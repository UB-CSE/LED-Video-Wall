#include <cstdio>
#include <exception>
#include <iostream>
#include <iterator>
#include <string.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // for close
#include "tcp.hpp"
#include "client.hpp"
#include "config-parser.hpp"
#include <vector>
#include "canvas.h"
#include "input-parser.hpp"
#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>



#include <mpi.h>
#define MASTER_PROCESSOR 0
#define CANVAS_PROCESSOR 1


/*

=====================================================================================
             Welcome to Harvey's Virtual Canvas and Element tutorial
=====================================================================================


This file aims to demo the relationship between the master processor 0, and canvas 
processor 1

*/


int main(int argc, char* argv[]) {


    //MPI Boiler Plate init
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    VirtualCanvas vCanvas;
    cv::Size dim(2000,2000); 

    //MPI 3's RMA setup. One window object and one pointer to walk through the buffer
    MPI_Win win;
    uchar* win_base_ptr = nullptr;


    /*

    =====================================================================================
                                        Initialization 
    =====================================================================================


    The canvas processor owns the virtual canvas. Thus, the canvas processor will read
    the input file and construct the virtual canvas.

    We use MPI windows to expose a region of memory (Virtual Canvas' PixelMatrix's buffer)
    to all available processors. As such, the master processor can reach into this
    stretch of canvas processor's memory and pluck whichever submatrix it needs

    */



    if(rank == CANVAS_PROCESSOR){

        //Create Virtual Canvas and load input file
        std::string inputFilePath = "input.yaml";
        
        vCanvas.pixelMatrix = cv::Mat::zeros(dim, CV_8UC3);
        vCanvas.dim = dim;
        parseInput(vCanvas, inputFilePath);

        //Expose the raw buffer as a window
        win_base_ptr = vCanvas.pixelMatrix.data;

        MPI_Win_create(win_base_ptr, vCanvas.pixelMatrix.total() * vCanvas.pixelMatrix.elemSize(), sizeof(uchar), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    }else{
        //Other processors don't have to bind a buffer to the window but have to open the window too
        MPI_Win_create(nullptr, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    }

    MPI_Barrier(MPI_COMM_WORLD);


    /*

    =====================================================================================
                                        READING
    =====================================================================================


    The master processor is in charge of reading the virtual canvas. Because MPI
    windows are one sided commuication tools, we need some sync mechanism.
    In this Case, we use window locking and unlocking. More specifically, when we do:

    MPI_Win_lock(MPI_LOCK_SHARED, CANVAS_PROCESSOR, 0, win)

    We are 
    a) utilizing a shared lock - multiple processors can read but not write to the window
    b) targetting the buffer through the window on CANVAS_PROCESSOR
    c) I don't actually know what 0 does, its a default
    d) Reference to the window object itself

    Unlocks are quite self explanatory...

    https://www.mpich.org/static/docs/v3.3/www3/MPI_Win_lock.html
    https://www.mpich.org/static/docs/v3.1/www3/MPI_Get.html
    
    */
    if (rank == MASTER_PROCESSOR) {

        while(true){

            //Here we define the size and location of the submatrix we want to extract from the vCanvas. 

            int roi_rows = 800, roi_cols = 800; //Size of submatrix
            int channels = 3;
            int start_y = 500; //row offset (LOCATION y)
            int start_x = 600; //col offset (LOCATION x)
        
            //This is the virtual canvas' dims
            int canvas_width = dim.width;
            int canvas_height = dim.height;
        
            std::vector<uchar> local_buffer(roi_rows * roi_cols * channels);
        
            MPI_Win_lock(MPI_LOCK_SHARED, CANVAS_PROCESSOR, 0, win);
        
            for (int r = 0; r < roi_rows; ++r) {
                MPI_Aint displacement = ((start_y + r) * canvas_width + start_x) * channels;
        
                //We use MPI GET and PUT instead of send and recv like in zola's class
                MPI_Get(
                    local_buffer.data() + r * roi_cols * channels,   //Destination buffer with ptr math
                    roi_cols * channels,                             //Total items to get
                    MPI_UNSIGNED_CHAR,
                    CANVAS_PROCESSOR,
                    displacement,                                    //Also known as offset
                    roi_cols * channels,
                    MPI_UNSIGNED_CHAR,
                    win
                );
            }
        
            MPI_Win_unlock(CANVAS_PROCESSOR, win);
        
            //Local display for demo
            cv::Mat receivedMat(roi_rows, roi_cols, CV_8UC3, local_buffer.data());
            cv::imshow("Extracted submatrix", receivedMat);
            cv::waitKey(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
 

        }
        
    }

   /*

    =====================================================================================
                                     UPDATE
    =====================================================================================

    Canvas processor updates the vCanvas' pixel matrix asynchonously from everything else.

    Because it is writing, it utilizes an exclusive lock to prevent shenanigans.

    Keep in mind that the code has changed dramatically. Individual elements now have a
    nextFrame method that updates their local pixelMatrix.

            Image Elements -> nextFrame() = does nothing
            Carousel Elements-> -> nextFrame() = advances frame and loops
            video Elements-> -> nextFrame() = advances frame and loops

    Changes to local pixelMatrices do not propogate to the virtual canvas' master pixel
    matrix until pushToCanvas is called.

    */

    if (rank == CANVAS_PROCESSOR) {
        while (true) {
            
            //Loops through all the element pointers held by the virtual canvas and update each one separtely
            //To implement variable framerates, we just call nextFrame on specific elements at different times
            for (int i = 0; i < vCanvas.getElementList().size(); i++) {
                cv::Mat frame;
                vCanvas.getElementList().at(i)->nextFrame(frame);
            }

            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, CANVAS_PROCESSOR, 0, win);
            std::cout << "[CANVAS_PROCESSOR] Updating frame...\n";
            vCanvas.pushToCanvas();
            MPI_Win_unlock(CANVAS_PROCESSOR, win);

            cv::imshow("Display Cats", vCanvas.getPixelMatrix());
            cv::waitKey(1);

            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
    

    

    
    MPI_Win_free(&win);
    MPI_Finalize();



    return 0;
}


