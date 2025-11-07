#include <iostream>
#include <unistd.h>
#include <sys/select.h>
#include <string>
#include "canvas.hpp"
#include "command.hpp"
#include "text-render.hpp"

/*
inputAvailable is boilerplate code from my past projects. This is intended to allow
stdin to be read afterwards without blocking from waiting for input.

This checks to see if stdin has an input waiting. If there is, this is true. If
there is not, this resolves to false

*/

bool inputAvailable() {
    struct timeval tv = {0, 0};  // zero timeout = check instantly
    fd_set fds;
    FD_ZERO(&fds);               // Clear the set
    FD_SET(STDIN_FILENO, &fds);  // Add stdin (fd 0)

    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}


int processCommand(VirtualCanvas& vCanvas, const std::string& line, bool& isPaused) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "quit") {
        return 1;
    }
    if (cmd == "pause") {
        isPaused = true;
        std::cout << "Paused.  Send <resume> to continue.\n";
        return 0;
    }
    if (cmd == "resume") {
        isPaused = false;
        std::cout << "Resumed.\n";
        return 0;
    }
    if (cmd == "move") {
        int id, x, y;
        if (!(iss >> id >> x >> y) || x < 0 || y < 0) {
            std::cerr << "Invalid move. Usage:\n  move <ElementID> <x> <y>\n";
        } else {
            vCanvas.moveElement(id, cv::Point(x, y));
        }
        return 0;
    }
    if (cmd == "add") {
        std::string type, filepath;
        int id, x, y;
        double scale;
        if (!(iss >> type >> id >> filepath >> x >> y >> scale)|| x < 0 || y < 0) {
            std::cerr << "Invalid add. Usage:\n add <type> <ElementID> <filepath> <x> <y>\n";
        } else {
            Element* newelement=new ImageElement(filepath,id,cv::Point(x,y),-1, scale);
            vCanvas.addElementToCanvas(newelement);
        }
        return 0;
    }
    if (cmd == "remove") {
        int id;
        if (!(iss >> id)) {
            std::cerr << "Invalid remove. Usage:\n  remove <ElementID>\n";
        } else {
           vCanvas.removeElementFromCanvas(id);
        return 0;
        }
    }


    //TEXT COMMANDS/////
    if (cmd == "set_text") {
        int id;
        std::string newText;
        if (!(iss >> id) || !std::getline(iss >> std::ws, newText)) {
            std::cerr << "Invalid set_text. Usage: set_text <ElementID> <text>\n";
        } else {
            Element* oldElem = nullptr;
            for (Element* e : vCanvas.getElementList()) {
                if (e && e->getId() == id) { oldElem = e; break; }
            }

            if (!oldElem) {
                std::cerr << "No element with id " << id << "\n";
                return 0;
            }

            TextElement* textElem = dynamic_cast<TextElement*>(oldElem);
            if (!textElem) {
                std::cerr << "Element " << id << " is not a text element.\n";
                return 0;
            }
            std::string fontPath = textElem->fontPath;
            int fontSize = textElem->fontSize;
            cv::Scalar color = textElem->color;
            cv::Point loc = textElem->getLocation();

            TextElement* newElem = dynamic_cast<TextElement*>(
                renderTextToElement(newText, fontPath, fontSize, color, id, loc)
            );

            vCanvas.removeElementFromCanvas(id);
            vCanvas.addElementToCanvas(newElem);
            vCanvas.pushToCanvas();

        }
        return 0;
    }
    if (cmd == "set_font_size") {
        int id;
        int newSize;
        if (!(iss >> id >> newSize)) {
            std::cerr << "Invalid set_font_size. Usage:\n set_font_size <ElementID> <size>\n";
        } else {
            // Find the element
            Element* oldElem = nullptr;
            for (Element* e : vCanvas.getElementList()) {
                if (e && e->getId() == id) { oldElem = e; break; }
            }

            if (!oldElem) {
                std::cerr << "No element with id " << id << "\n";
                return 0;
            }

            TextElement* textElem = dynamic_cast<TextElement*>(oldElem);
            if (!textElem) {
                std::cerr << "Element " << id << " is not a text element.\n";
                return 0;
            }
            std::string text = textElem->content;  
            std::string fontPath = textElem->fontPath;
            cv::Scalar color = textElem->color;
            cv::Point loc = textElem->getLocation();

            TextElement* newElem = dynamic_cast<TextElement*>(
                renderTextToElement(text, fontPath, newSize, color, id, loc)
            );

            vCanvas.removeElementFromCanvas(id);
            vCanvas.addElementToCanvas(newElem);
            vCanvas.pushToCanvas();

        }
        return 0;
    }
    if (cmd == "set_font") {
        int id;
        std::string newFontPath;
        if (!(iss >> id >> std::ws) || !std::getline(iss, newFontPath)) {
            std::cerr << "Invalid set_font. Usage:\n set_font <ElementID> <fontPath>\n";
            return 0;
        }

        Element* oldElem = nullptr;
        for (Element* e : vCanvas.getElementList()) {
            if (e && e->getId() == id) { oldElem = e; break; }
        }

        if (!oldElem) {
            std::cerr << "No element with id " << id << "\n";
            return 0;
        }

        TextElement* textElem = dynamic_cast<TextElement*>(oldElem);
        if (!textElem) {
            std::cerr << "Element " << id << " is not a text element.\n";
            return 0;
        }

        std::string currentText = textElem->content;
        int fontSize = textElem->fontSize;
        cv::Scalar color = textElem->color;
        cv::Point loc = textElem->getLocation();

        TextElement* newElem = dynamic_cast<TextElement*>(
            renderTextToElement(currentText, newFontPath, fontSize, color, id, loc)
        );
        if (!newElem) {
        std::cerr << "Failed to render new text with font: " << newFontPath << "\n";
        return 0; 
        }

        vCanvas.removeElementFromCanvas(id);
        vCanvas.addElementToCanvas(newElem);
        vCanvas.pushToCanvas();
        return 0;
    }
    if (cmd == "set_font_color") {
        int id;
        int b, g, r;
        if (!(iss >> id >> b >> g >> r)) {
            std::cerr << "Invalid set_font_color. Usage:\n set_font_color <ElementID> <B> <G> <R>\n";
        } else {
            Element* oldElem = nullptr;
            for (Element* e : vCanvas.getElementList()) {
                if (e && e->getId() == id) { oldElem = e; break; }
            }

            if (!oldElem) {
                std::cerr << "No element with id " << id << "\n";
                return 0;
            }
            TextElement* textElem = dynamic_cast<TextElement*>(oldElem);
            if (!textElem) {
                std::cerr << "Element " << id << " is not a text element.\n";
                return 0;
            }
            std::string text = textElem->content;
            std::string fontPath = textElem->fontPath;
            int fontSize = textElem->fontSize;
            cv::Point loc = textElem->getLocation();
            cv::Scalar newColor(b, g, r); 

            TextElement* newElem = dynamic_cast<TextElement*>(
                renderTextToElement(text, fontPath, fontSize, newColor, id, loc)
            );

            vCanvas.removeElementFromCanvas(id);
            vCanvas.addElementToCanvas(newElem);
            vCanvas.pushToCanvas();

            return 0;
        }
    }
    if (cmd == "scale") {
        int id;
        double factor;
        if (!(iss >> id >> factor) || factor <= 0.0) {
            std::cerr << "Invalid scale. Usage:\n  scale <ElementID> <factor> (e.g., 0.5)\n";
            return 0;
        }

        Element* oldElem = nullptr;
        for (Element* e : vCanvas.getElementList()) {
            if (e && e->getId() == id) { oldElem = e; break; }
        }

        if (!oldElem) {
            std::cerr << "No element with id " << id << "\n";
            return 0;
        }

        ImageElement* imgElem = dynamic_cast<ImageElement*>(oldElem);
        if (!imgElem) {
            std::cerr << "Element " << id << " is not an image element.\n";
            return 0;
        }

        std::string path   = imgElem->getFilePath();
        cv::Point   loc    = imgElem->getLocation();
        int         fps    = oldElem->getFrameRate();
        double      scale  = imgElem->getScale();

        vCanvas.removeElementFromCanvas(id);
        Element* newElem = nullptr;
        try {
            newElem = new ImageElement(path, id, loc, fps, scale);
            static_cast<ImageElement*>(newElem)->setScale(factor);  
            vCanvas.addElementToCanvas(newElem);
            vCanvas.pushToCanvas();
        } catch (const std::exception& e) {
            std::cerr << "[scale] Failed to reload image: " << e.what() << "\n";
            if (newElem) { delete newElem; }
            return 0;
        }

        std::cout << "Scaled image " << id << " by factor " << factor << "\n";
        return 0;
    }
    std::cout << "Unknown command: " << cmd << "\n"
                    "Available: pause, resume, quit, move <id> <x> <y>\n";
        return 0;
    }