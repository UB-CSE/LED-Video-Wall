# CSE LED Video Wall Check-In Server

## About

When the microcontroller client boots, it sends its MAC address to this server (hosted at ledvwci.cse.buffalo.edu) to
determine which LED Video Wall server instance to connect to. This service prevents needing to hard-code production vs
development hosts into the client's source code, which prevents having to re-flash the microcontrollers when switching
LED Video Wall servers.

## Setup

Create `hosts.csv` based on `hosts.csv.example`.

Run `docker compose up --build -d`

The web server will run on port 5513. You should configure a reverse proxy to direct traffic here.

## Usage

Add each microcontroller's MAC address and LED Video Wall server host pair to `hosts.csv`. Changes are applied
immediately, as the file is read on each request. The MAC address can be formatted in any way. Capitalization and
delimiters do not matter. See `hosts.csv.example` for formatting.

## API

Make a request to `https://ledvwci.cse.buffalo.edu/client/get-server/<MAC Address>`, replacing `<MAC Address>` with the
microcontrollers MAC Address in any format to retrieve the corresponding hostname as plain text.