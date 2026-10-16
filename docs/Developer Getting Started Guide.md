# Developer Getting Started Guide

The LED Video Wall project is divided into four major components:
- [LEDVW Server](#ledvw+server)
- [Microcontroller Client](#microcontroller+client)
- [Web Server](#web+server)
- [Web Client](#web+client)

## LEDVW Server

### Features
- Maintains a virtual canvas that can be configured via a YAML file and updated in real-time with commands sent to a Unix socket located at `/tmp/ledvw-cmd-<wall-name>`
- Supports images, carousels (slideshows), videos, RTMP live streams, web browsers, and text elements
- Physical wall layout configuration is described in a YAML file provided at server start-up
	- Specifies the location of each LED matrix on the virtual canvas, MCU client MAC addresses, GPIO pins, system framerate, image encoding, etc.
- Encodes and sends frames to appropriate microcontrollers for each portion of the canvas

### Local Installation

Linux is the only supported OS for the server.  If you are using Windows, install and use [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) for development work. If you are using macOS, set up a Linux VM with VMWare Fusion (or other) or install [Asahi](https://asahilinux.org/) on your machine (if supported).

#### 1. Install dependencies

Ubuntu/Debian:

```bash
sudo apt install -y git \
					build-essential \
                    cmake \
                    pkg-config \
                    libopencv-dev \
                    libfreetype6-dev \
                    librtmp-dev \
                    libreadline-dev
```

Note: Verify that your C++ compiler supports C++20 by running `g++ --version`. If it is lower than version 16, install the `g++-16` package and use that compiler instead.
#### 2. Navigate to the `server` directory

#### 3. Configure the CMake project.

```bash
cmake -S . -B build/
```

Be patient. This step may take a few minutes to download a few large third-party libraries. You will not need to re-configure after this unless you make changes to the CMake project or add/remove source files.

<details>
	<summary>Advanced Options</summary>
	<ul>
		<li>Add `-DCMAKE_BUILD_TYPE=Release` for release optimizations</li>
		<li>To disable tests from being built add `-DBUILD_TESTS=OFF</li>
		<li>To disable the command tester program from being built add `-DBUILD_CMD_TESTER=OFF`</li>
	</ul>
</details>

#### 4. Compile the program

```bash
cmake --build build/
```

The first time you compile will take a few minutes to build all the third-party libraries and source files for the project. Subsequent builds will only re-compile modified source files and re-link the program.

The resulting server program executable will be located at `build/led-wall-server`

### Running the Server

Start the server like so:
```bash
./build/led-wall-server <matrix-config-file>
```

See the `matrix-configs` folder for matrix layout configuration files.

The following command-line arguments are also supported:
- `--canvas-config=<file>` : Specify a canvas config YAML file to load at start-up (see `canvas-configs` folder for examples).
- `--prod` : When present, the video wall preview window is not shown.
- `--interactive` : Show a command prompt for interacting with the canvas.
- `--rtmp-tls-cert` and `--rtmp-tls-key` : Enable RTMP TLS with a cert and key file.

##### CEF Permissions Issue

If you see the following error on start up:
```
The SUID sandbox helper binary was found, but is not configured correctly. Rather than run without sandboxing I'm aborting now. You need to make sure that /path/to/LED-Video-Wall/server/build/_deps/cef-src/Release/chrome-sandbox is owned by root and has mode 4755.
```

Modify the permissions of the `chrome-sandbox` program so that it gets run as root
```bash
# Owner is root
chown root ./build/_deps/cef-src/Release/chrome-sandbox

# All users can execute. SUID bit makes it execute as owner (root)
chmod 4755 ./build/_deps/cef-src/Release/chrome-sandbox
```

Or use the `scripts/configure-cef-sandbox.bash` script.

### Unit Testing

[GoogleTest](https://google.github.io/googletest/primer.html) is used for unit testing. Unit test code is located in the `tests/` directory.

To run tests:
- Compile the server (see above).
    - Make sure that the `BUILD_TESTS` configuration option set to `ON` (default).
- Run the tests with `./build/led-wall-server-tests`
  - Some tests will write images/files to disk that you can manually inspect at `./build/test-output`

## Microcontroller Client

### Features
- Lightweight software that runs on each ESP32 microcontroller
- First, connects to check-in server, then connects to LEDVW server with TCP socket
- Receives encoded images from the server and decodes them to RGB, then sets the matrix LEDs
- Gracefully handles network disruptions and reconnects automatically

### Installation

On Windows, do this in [WSL](https://learn.microsoft.com/en-us/windows/wsl/install).
#### 1. Install Dependencies
Required:
* [`idf.py`](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html#installation) (Tested with `v6.0`)

Optional:
* [`idf-eclipse-plugin`](https://github.com/espressif/idf-eclipse-plugin/blob/master/README.md) (Eclipse)
* [`vscode-esp-idf-extension`](https://github.com/espressif/vscode-esp-idf-extension/blob/master/README.md) (VS Code)
* [`clangd`](https://clangd.llvm.org/installation) (LSP)
* [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) (Formatter)
* [`espflash`](https://github.com/esp-rs/espflash) (Alternative to `esptool.py`)
* [`WSL-install`](https://learn.microsoft.com/en-us/windows/wsl/install) (WSL)

#### 2. For Developers using WSL
You need to attach the ESP32 to the WSL

*[`USPID`](https://learn.microsoft.com/en-us/windows/wsl/connect-usb) Install USBIPD on WSL

The following commands must be done in Administrator Mode in PowerShell

Find and copy the bus ID of the ESP32 through:
```PowerShell
usbipd list
```
Using this bus ID share the ESP32:
```PowerShell
usbipd bind --busid <bus ID>
```
To attach the ESP32:
```PowerShell
usbipd attach --wsl --busid <bus ID>
```
*[`Attaching USB Device to WSL`](https://learn.microsoft.com/en-us/windows/wsl/connect-usb) For more information

#### 3. Configure "wifi_credentials.hpp"

There should be a separate header file, "wifi_credentials.hpp", that defines

preprocessor constants, WIFI_SSID and WIFI_PASSWORD, that are used by the

clients to connect to wifi.

!!! THE CREDENTIALS FILE MUST NOT BE ADDDED TO GIT/SOURCE CONTROL !!!

Here is an example of what "wifi_credentials.hpp" should look like:

```
#ifndef CREDENTIALS_HPP
#define CREDENTIALS_HPP
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif
```
WIFI_SSID and WIFIF_PASSWORD will be provided to you

#### 4. Load the Environment
Before running any commands, be sure to load the environment via:
```bash
$ . $HOME/esp/esp-idf/export.sh
```

#### 5. Build
```bash
$ idf.py build
```

### Flashing & Monitoring
To flash the built program to the connected microcontroller:
```bash
$ idf.py flash
```

To monitor the logs program logs:
```bash
$ idf.py monitor
```
To quit the logs press `Ctrl+]`

Tip: idf.py allows you to combine commands like so:
```bash
$ idf.py build flash monitor
```

## Web Server
### Features
- Provides a web API for controlling video walls
- Websocket support for live-editing
- Starts and keep track of multiple instances of the LEDVW server. Sends each server commands over its associated Unix socket.

### Local Installation

On Windows, do this in [WSL](https://learn.microsoft.com/en-us/windows/wsl/install).
#### 1. Navigate to the `web/web-server` directory
#### 2. Download Dependencies
```
go mod download
```

### Running the Server
```
go run .
```
The web server will run on port 5000

Or build, then run it:
```
go build
```

```
./LEDVW_WebServer
```

## Web Client

### Features
- Provides a user-friendly GUI for controlling video walls.
- Communicates with the web server to control the video walls 

### Local Installation

On Windows, do this in [WSL](https://learn.microsoft.com/en-us/windows/wsl/install).

#### 1. Navigate to the `web/LED-Wall-Website` directory
#### 2. Install Node Version Manager

```bash
wget -qO- https://raw.githubusercontent.com/nvm-sh/nvm/v0.40.4/install.sh | bash
```

Refresh your path:
```
source ~/.bashrc
```
#### 3. Install Node
```
nvm install --lts
```

#### 4. Install Dependencies
```
npm i
```
If there are problems try
```
npm audit fix
```

#### 5. Run the development server
```
npm run dev
```
Visit the URL printed in the terminal