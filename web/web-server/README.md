# Web Server

* Provides a convenient API for controlling the LEDVW Server without manually editing YAML files
* Reads and writes canvas configuration files to add and configure attributes for images, text, etc.
* Handles starting and stopping the LEDVW server
* Allows users to upload media and add it to the canvas
* Partial support for real-time updates via UNIX socket commands

## Local Installation

* If you're using Windows, install this in WSL
* Navigate to the `web/web-server` directory
* Run `sudo apt install python3-pip python3.12-venv` to install core Python dependencies
* Create a virtual environment with `python3 -m venv .venv` (ensure you're in the web-server directory first)
* Run `source .venv/bin/activate` to activate the virtual environment
* Run `pip install -r requirements.txt` to install the project dependencies
* Run the app in development mode with `flask --app main run --debug`