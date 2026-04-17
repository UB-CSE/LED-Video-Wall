#!/bin/bash

function check_result() {
    if [ $? -ne 0 ]; then
        exit 1
    fi
}

if [ ! -f led-wall-server ] || [ -L led-wall-server ]; then
    echo "Error: led-wall-server executable not found or is a symlink. Please build the project first."
    exit 1
fi

mkdir -p 'led-wall-server.app/Contents/MacOS' 'led-wall-server.app/Contents/Frameworks'

# Executable

mv led-wall-server led-wall-server.app/Contents/MacOS/
ln -sF 'led-wall-server.app/Contents/MacOS/led-wall-server' 'led-wall-server'

# CEF Framework

cp -R 'cef/lib/Chromium Embedded Framework.framework' 'led-wall-server.app/Contents/Frameworks'
check_result

# CEF Helper

cd cef
mkdir -p 'led-wall-server Helper.app/Contents/MacOS'

if [ ! -f process_helper_mac.cc ]; then
    echo "Downloading process_helper_mac.cc..."
    wget https://github.com/chromiumembedded/cef/raw/refs/heads/master/tests/cefsimple/process_helper_mac.cc
    check_result
fi

g++ -std=c++20 -Iinclude -I. process_helper_mac.cc -o 'led-wall-server Helper.app/Contents/MacOS/led-wall-server Helper' -Flib -framework 'Chromium Embedded Framework' lib/libcef_dll_wrapper.a -DCEF_USE_SANDBOX
check_result

install_name_tool -change '@executable_path/../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework' '@executable_path/../../../Chromium Embedded Framework.framework/Chromium Embedded Framework' 'led-wall-server Helper.app/Contents/MacOS/led-wall-server Helper'
check_result

cp -r 'led-wall-server Helper.app' '../led-wall-server.app/Contents/Frameworks'
check_result

