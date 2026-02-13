# LED Video Wall (LEDVW) Server

* Maintains a virtual canvas that can be configured via a YAML file and updated in real-time with UNIX socket commands to `/tmp/led-cmd`
* Supports images, carousels (slideshows), videos, live streams (via webcam), and text
* Physical wall layout configured in `config.yaml`
* Specifies the location of each LED matrix on the virtual canvas, client MAC addresses, GPIO pins, system framerate, etc.
* Encodes and sends frames to appropriate microcontrollers for each portion of the canvas

## Local Installation

* If you're using Windows, install this in WSL
* Install dependencies with `sudo apt install -y g++ make libopencv-dev libyaml-cpp-dev`
* Navigate to the `server` directory in this repository (the parent of this README file)
* Run `make` to compile the server application
* Start the server with `./led-wall-server <configuration file>`, e.g., `./led-wall-server input-text.yaml`