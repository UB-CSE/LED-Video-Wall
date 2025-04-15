#ifndef CANVAS_H
#define CANVAS_H

// clang-format off
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include "input-parser.hpp"




class Element {

    private:
        int id;
        cv::Point location;
        int frameRate;
        
        
    public:

        int getId() const { return id;};
        cv::Point& getLocation() { return location;};
        int getFrameRate() {return frameRate;};
        cv::Mat getPixelMatrix() {return pixelMatrix;};

        virtual bool nextFrame(cv::Mat& frame) = 0;
        virtual void reset() = 0;
        virtual ~Element() {}

    protected:
        
        cv::Mat pixelMatrix;
        Element(int id, cv::Point loc, int frameRate) : id(id), location(loc), frameRate(frameRate) {}
    };
    
class ImageElement : public Element {
    private:
        bool provided;
    
    public:
        ImageElement(const std::string& filepath, int id, cv::Point loc, int frameRate);
        bool nextFrame(cv::Mat& frame) override;
        void reset() override;
    };
    
class CarouselElement : public Element {
    private:
        std::vector<cv::Mat> pixelMatrices;
        size_t current; //This is the internal counter for carousel objects to remember which frame they are on
    
    public:
        CarouselElement(const std::vector<std::string>& filepaths, int id, cv::Point loc, int frameRate);
        bool nextFrame(cv::Mat& frame) override;
        void reset() override;
    };
    
class VideoElement : public Element {
    private:
        cv::VideoCapture cap;
    
    public:
        VideoElement(const std::string& filepath, int id, cv::Point loc, int frameRate);
        bool nextFrame(cv::Mat& frame) override;
        void reset() override;
    };


//Originally, elementCount and PixelMatrix and the rest were in private, but for my MPI implementation, i needed to access them directly to init vCanvas with a default constructer
class VirtualCanvas{        
    
    public:
        int elementCount;


        cv::Mat pixelMatrix;
        cv::Mat canvasLut;
        cv::Size dim;
        std::vector<Element *> elementPtrList;

        //Default Constructor
        VirtualCanvas(){}

        VirtualCanvas(const cv::Size& size) : dim(size) {
            pixelMatrix = cv::Mat::zeros(dim, CV_8UC3);
        }
        
        //Keeping getPixelMatrix's matrix return instead of what I'm doing for nextFrame in order to cut down on code merging time 
        cv::Mat getPixelMatrix() const { return pixelMatrix; }

        cv::Size getDimensions() const { return dim; }
        int getElementCount() const { return elementCount; }
        const std::vector<Element *>& getElementList() const { return elementPtrList; }
        void clear() {pixelMatrix = cv::Mat::zeros(dim, CV_8UC3);}
        bool moveElement(int elementId, cv::Point loc);
        void addElementToCanvas(Element* element);
        bool removeElementFromCanvas(int elementId);
        void pushToCanvas();
    };


/*
class AbstractCanvas {
protected:
    cv::Mat pixelMatrix;
    cv::Size dim;

public:
    AbstractCanvas() {}
    AbstractCanvas(const cv::Size& size) : dim(size) {
        pixelMatrix = cv::Mat::zeros(dim, CV_8UC3);
    }
    virtual ~AbstractCanvas() {}

    cv::Mat getPixelMatrix() const { return pixelMatrix; }
    cv::Size getDimensions() const { return dim; }

    virtual void clear() = 0;  // Pure virtual function
};



class Element : public AbstractCanvas {
    private:
        std::string filePath;
        cv::Point location;
        int id;

       

    
    public:
        std::tuple<int,int> frameRateData;
        Element(const std::string& path, int elementId, cv::Point loc = cv::Point(0, 0), std::tuple<int, int> frameRateData = std::make_tuple(-1,0));
        Element(const cv::Mat, int elementId, cv::Point loc = cv::Point(0, 0), std::tuple<int, int> frameRateData = std::make_tuple(-1,0));
    
        std::string getFilePath() const { return filePath; }
        cv::Point getLocation() const { return location; }
        std::tuple<int,int> getFrameRateData() {return frameRateData;}
        int getId() const { return id; }
    
        void setLocation(const cv::Point& loc) { location = loc; }
        virtual void clear() override;
    };


*/






        
    

#endif
