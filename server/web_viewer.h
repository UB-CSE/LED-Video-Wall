#ifndef WEB_VIEWER_H
#define WEB_VIEWER_H

#include "canvas.h"
#include "httplib.h"
#include <mutex>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>

class WebViewer {
private:
  httplib::Server svr;
  std::thread server_thread;
  std::mutex canvas_mutex;
  cv::Mat current_frame;
  bool running;

  // Convert OpenCV Mat to JPEG binary data
  std::vector<uchar> matToJpeg(const cv::Mat &image) {
    // Scale up the image for better visibility (5x)
    cv::Mat scaled;
    cv::resize(image, scaled, cv::Size(), 5.0, 5.0, cv::INTER_NEAREST);

    // Convert to JPEG
    std::vector<uchar> buf;
    cv::imencode(".jpg", scaled, buf);
    return buf;
  }

public:
  WebViewer() : running(false) {}

  void start(int port = 8080) {
    if (running)
      return;
    running = true;

    // Serve static HTML
    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
      res.set_content(R"(
<!DOCTYPE html>
<html>
<head>
    <title>LED Matrix Viewer</title>
    <style>
        body { 
            background: #1a1a1a; 
            color: #fff; 
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        #canvas-container {
            border: 2px solid #333;
            margin: 20px;
            padding: 10px;
            background: #000;
        }
        #matrix-canvas {
            max-width: 100%;
            height: auto;
        }
    </style>
</head>
<body>
    <h1>LED Matrix Viewer</h1>
    <div id="canvas-container">
        <img id="matrix-canvas" src="/canvas" alt="LED Matrix">
    </div>
    <script>
        function updateCanvas() {
            const img = document.getElementById('matrix-canvas');
            img.src = '/canvas?' + new Date().getTime();
        }
        // Update every 100ms
        setInterval(updateCanvas, 100);
    </script>
</body>
</html>
            )",
                      "text/html");
    });

    // Serve canvas image
    svr.Get("/canvas", [this](const httplib::Request &,
                              httplib::Response &res) {
      std::vector<uchar> jpg_data;
      {
        std::lock_guard<std::mutex> lock(canvas_mutex);
        jpg_data = matToJpeg(current_frame);
      }
      res.set_content((char *)jpg_data.data(), jpg_data.size(), "image/jpeg");
    });

    // Start server in separate thread
    server_thread =
        std::thread([this, port]() { svr.listen("localhost", port); });
  }

  void updateCanvas(const cv::Mat &frame) {
    std::lock_guard<std::mutex> lock(canvas_mutex);
    frame.copyTo(current_frame);
  }

  void stop() {
    if (!running)
      return;
    running = false;
    svr.stop();
    if (server_thread.joinable()) {
      server_thread.join();
    }
  }

  ~WebViewer() { stop(); }
};

#endif
