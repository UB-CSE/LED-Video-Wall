#ifndef CANVAS_H
#define CANVAS_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <algorithm>
#include "input-parser.hpp"


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
        Element(const std::string& path, int elementId, cv::Point loc = cv::Point(0, 0));
    
        std::string getFilePath() const { return filePath; }
        cv::Point getLocation() const { return location; }
        int getId() const { return id; }
    
        void setLocation(const cv::Point& loc) { location = loc; }
        virtual void clear() override;
    };


class VirtualCanvas : public AbstractCanvas {
    private:
        int elementCount;
        std::vector<ElemTuple>elementList;
    
    public:
        VirtualCanvas(const cv::Size& size);
        virtual void clear() override;
        
        void addElementToCanvas(const ElemTuple & element);
        void addPayloadToCanvas(Payload & elementsPayload);
        void removeElementFromCanvas(int elementId);
        void pushToCanvas();
        void updateCanvas();
        
        int getElementCount() const { return elementCount; }
        const std::vector<ElemTuple>& getElementList() const { return elementList; }
    };
        
    

#endif