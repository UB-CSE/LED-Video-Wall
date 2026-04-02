#!/bin/bash

function check_result() {
    if [ $? -ne 0 ]; then
        exit 1
    fi
}

BUILD_TYPE="Release" # Debug or Release
CEF_VERSION="146.0.9+g3ca6a87+chromium-146.0.7680.165"

if [[ $(uname -s) == "Darwin" ]]; then
    PLATFORM="macos"
elif [[ $(uname -s) == "Linux" ]]; then
    PLATFORM="linux"
else
    echo "Unsupported OS: $(uname -s)"
    exit 1
fi

if [[ $(uname -m) == "x86_64" ]]; then
  if [[ $PLATFORM == "macos" ]]; then
    ARCH="x64"
  else
    ARCH="64"
  fi
elif [[ $(uname -m) == "aarch64" || $(uname -m) == "arm64" ]]; then
    ARCH="arm64"
else
    echo "Unsupported architecture: $(uname -m)"
    exit 1
fi

CEF_BUILD_NAME="cef_binary_${CEF_VERSION}_${PLATFORM}${ARCH}_minimal"
CEF_DOWNLOAD_FILE="${CEF_BUILD_NAME}.tar.bz2"
CEF_DOWNLOAD_URL="https://cef-builds.spotifycdn.com/${CEF_DOWNLOAD_FILE}"

if [ ! -d "cef" ]; then
    echo "Creating cef directory..."
    mkdir cef
fi

cd cef
check_result

if [ ! -f "${CEF_DOWNLOAD_FILE}" ]; then
    echo "Downloading CEF from ${CEF_DOWNLOAD_URL}..."
    wget "${CEF_DOWNLOAD_URL}" -O "${CEF_DOWNLOAD_FILE}"
    check_result
else
    echo "CEF archive already exists, skipping download."
fi

if [ ! -d "${CEF_BUILD_NAME}" ]; then
    echo "Extracting CEF archive..."
    tar -xjf "${CEF_DOWNLOAD_FILE}"
    check_result
else
    echo "CEF directory already exists, skipping extraction."
fi

cd "${CEF_BUILD_NAME}"
check_result

echo "Configuring CEF build with CMake..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
check_result
echo "Building CEF with CMake..."
cmake --build build --config ${BUILD_TYPE} -j$(nproc)
check_result
echo "CEF build completed successfully."

if [ ! -d "../lib" ]; then
    echo "Creating lib directory..."
    mkdir ../lib
fi

echo "Copying CEF libraries and resources..."
cp build/libcef_dll_wrapper/libcef_dll_wrapper.a ../lib
check_result

cp -r Release/* ../lib
check_result

cp -r Resources/* ../lib
check_result
echo "CEF libraries and resources copied to lib directory."

echo "Creating include directory symlink..."
if [ -d "../include" ]; then
  rm -rf ../include
fi

ln -sF $(pwd)/include ../include
check_result
echo "Include directory symlink created."

echo "CEF setup completed successfully."

