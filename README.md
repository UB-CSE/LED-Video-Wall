# LED Video Wall

**A student-developed large-scale, modular, and affordable LED video wall**

Development occurs on the `dev` branch. Contributors should open pull requests into that branch.

## Overview

This project is composed of four main parts (and a bonus):

* **LED Video Wall** (LEDVW) **server** (C++, located in `server` directory)
* **Microcontroller client** (C++, `client`)
* **Web server** (Python, Flask, `web/web-server`)
* **Web client** (TypeScript, React, `web/LED-Wall-Website`)
* (Bonus) **Check-in server** (Python, Flask, `check-in`)

At a high level, the LED Video Wall server application maintains a virtual canvas, where images, videos, text, and other sources can be layered. This can be configured via YAML files and manipulated in real-time via socket commands.

Each microcontroller is responsible for a small segment of the entire wall, generally 32x32 pixels, but this is flexible. The LEDVW server is configured to know which microcontrollers are responsible for each part of the canvas. It sends only the required pixel data for each segment to the appropriate microcontroller using a **custom high-efficiency protocol** over TCP. Each microcontroller decodes these messages and updates their LED matrices to display their portion of the canvas.

We use commodity hardware, including ESP32 microcontrollers and WS2812B LED matrices, to keep costs low and make building this accessible to everyone.

The web server acts as a bridge between the LEDVW server and the web client, allowing non-technical users to manipulate the canvas in a user-friendly way. It supports configuration file updates and real-time commands. The web client offers a convenient layer-based editor for creating canvas input configurations.

The check-in server speeds up development by allowing the microcontroller clients to quickly change which LEDVW server they connect to without recompiling and flashing the software.

Each part of the project has a more detailed README.md with information and installation instructions in its directory.

## Contributors

**Spring 2025**: Alex Bohosian, Michael Beards, Harvey Kwong, Nicholas Salvemini

**Fall 2025**: Jacob Bemis, Isaac Loc, Jason Lu, Torin Sheahen

**Spring 2026**: Connor Foster, Kevin Greegary, Md Hussain, Peter Lilley, Yasen Yanchev

**Project Manager**: Nicholas Myers
