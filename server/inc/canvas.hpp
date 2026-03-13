#ifndef CANVAS_H
#define CANVAS_H

// clang-format off
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include "input-parser.hpp"
#include "rtmp.hpp"


class Element {

    private:
        int id;
        cv::Point location;
        cv::Point adjustedLocation;
        int frameRate;
        double angleDegrees = 0.0;
        
    public:

        int getId() const { return id;};
        cv::Point& getLocation() { return adjustedLocation;};
        int getFrameRate() {return frameRate;};
        cv::Mat getPixelMatrix() {return pixelMatrix;};

        // Set pixelMatrix to the next frame
        virtual bool nextFrame() { return false; }
        virtual void reset() {}
        virtual ~Element() {}

    protected:
        
        cv::Mat pixelMatrix;
        Element(int id, cv::Point loc, int frameRate, double rotationDegrees = 0.0) : id(id), location(loc), adjustedLocation(loc), frameRate(frameRate), angleDegrees(rotationDegrees) {}

        // Rotates pixelMatrix by rotationDegrees
        void rotateFrame();
    };
    
class ImageElement : public Element {
    private:
        bool provided;
        cv::Mat original_;   
        double  scale_;
        std::string filePath_;
    
    public:
        ImageElement(const std::string& filepath, int id, cv::Point loc, double scale, double rotationDegrees = 0.0);

        const std::string& getFilePath() const { return filePath_; }
        const double getScale() const {return scale_; }

        void setScale(double s) {
            if (s <= 0.0) return;
            scale_ = s;
            if (original_.empty()) return;

            if (std::abs(scale_ - 1.0) < 1e-6) {
                pixelMatrix = original_.clone();
            } else {
                cv::resize(
                    original_, pixelMatrix, cv::Size(),
                    scale_, scale_,
                    (scale_ < 1.0) ? cv::INTER_AREA : cv::INTER_LINEAR
                );
            }
        }
        void reset() override;
    };
    
class CarouselElement : public Element {
    private:
        std::vector<cv::Mat> pixelMatrices;
        size_t current; //This is the internal counter for carousel objects to remember which frame they are on
    
    public:
        CarouselElement(const std::vector<std::string>& filepaths, int id, cv::Point loc, int frameRate, double rotationDegrees = 0.0);
        bool nextFrame() override;
        void reset() override;
    };
    
class VideoElement : public Element {
    private:
        cv::VideoCapture cap;
    
    public:
        VideoElement(const std::string& filepath, int id, cv::Point loc, int frameRate, double rotationDegrees = 0.0);
        VideoElement(int webcamNum, int id, cv::Point loc, int frameRate, double rotationDegrees = 0.0);
        bool nextFrame() override;
        void reset() override;
    };

class RTMPStreamElement : public Element {
    private:
        RTMPServer& rtmpServer;
        std::string streamName;
        cv::Size size;

        const cv::Mat noFrameMat = cv::Mat(100, 100, CV_8UC3, cv::Scalar(0, 255, 0)); // green
    
    public:
        RTMPStreamElement(RTMPServer& rtmpServer, const std::string& streamName, int id, cv::Point loc, int frameRate, cv::Size size = cv::Size(0, 0), double rotationDegrees = 0.0);
        bool nextFrame() override;
        void reset() override;
    };

class TextElement : public Element {

    public:
        std::string content;   
        std::string fontPath;  
        int fontSize;         
        cv::Scalar color;      

   
        TextElement(const cv::Mat& imgBGR, int id, cv::Point loc, const std::string& text, const std::string& font, int size, cv::Scalar col, double rotationDegrees = 0.0);
};
  

//Originally, elementCount and PixelMatrix and the rest were in private, but for my MPI implementation, i needed to access them directly to init vCanvas with a default constructer
class VirtualCanvas{        
    
    public:
        int elementCount = 0;

        cv::Mat pixelMatrix;
        cv::Mat canvasLut;
        cv::Size dim;
        std::vector<Element *> elementPtrList;

        explicit VirtualCanvas(const cv::Size& size) : dim(size) {
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

    private:
        void overlayImage(const cv::Mat& overlay, cv::Rect roi);
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
