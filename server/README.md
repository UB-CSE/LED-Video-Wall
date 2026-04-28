# LED Video Wall (LEDVW) Server

* Maintains a virtual canvas that can be configured via a YAML file and updated in real-time with commands sent to a pipe at `/tmp/led-cmd-<port>`
* Supports images, carousels (slideshows), videos, live streams (via webcam), and text
* Physical wall layout configured in `config.yaml`
* Specifies the location of each LED matrix on the virtual canvas, client MAC addresses, GPIO pins, system framerate, etc.
* Encodes and sends frames to appropriate microcontrollers for each portion of the canvas

## Local Installation

* If you're using Windows, install this in WSL
* Install dependencies with `sudo apt install -y g++ make cmake libopencv-dev libyaml-cpp-dev libasound2t64`
* Navigate to the `server` directory in this repository (the parent of this README file)
* Run `make` to compile the server application
    * Note that the first time you run `make`, it will also download and build CEF, which may take a while. Future builds will be considerably faster
* Start the server with `./led-wall-server <configuration file>`, e.g., `./led-wall-server input-text.yaml`
    * The following command-line arguments are also supported:
        * `--ledvw-port=<port>` : The port which the microcontrollers will connect to for communication (default: 7070)
            * This port number is also used as a kind of "id" for the server. The command pipe will be named `/tmp/led-cmd-<ledvw-port>`, and CEF caches will be stored in `./cef-caches/cef-cache-<ledvw-port>`.
        * `--rtmp-port=<port>` : The port for the RTMP server (default: 1935)
        * `--prod` : When present, the video wall preview window is not shown.
* If you see the following error message on startup:
    ```
    The SUID sandbox helper binary was found, but is not configured correctly. Rather than run without sandboxing I'm aborting now. You need to make sure that /path/to/LED-Video-Wall/server/cef/lib/chrome-sandbox is owned by root and has mode 4755.
    ```
    Then run the following script as root to fix the permissions of the sandbox binary:
    ```bash
    sudo ./scripts/configure-cef-sandbox.bash ./cef/lib/chrome-sandbox
    ```
