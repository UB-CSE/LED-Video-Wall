cmake_minimum_required(VERSION 3.25)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

### Build type ###

# Default to debug build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

### Project ###

project(LEDWallServer)
add_executable(${PROJECT_NAME})

# compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Platform ###

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}")
endif()

### Languages ###

enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

### Sources ###

file(GLOB_RECURSE SOURCES src/*.cpp ../protocol/src/*.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE inc inc_dummy ../protocol/src)

### Libraries ###

include(FetchContent)
find_package(PkgConfig REQUIRED)

# opencv4
pkg_check_modules(OpenCV REQUIRED opencv4)
target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${OpenCV_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${OpenCV_DEFINITIONS})

# yaml-cpp
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${YAML_CPP_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${YAML_CPP_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${YAML_CPP_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${YAML_CPP_DEFINITIONS})

# freetype2
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
target_include_directories(${PROJECT_NAME} PRIVATE ${FREETYPE2_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${FREETYPE2_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${FREETYPE2_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${FREETYPE2_DEFINITIONS})

# librtmp
pkg_check_modules(LIBRTMP REQUIRED librtmp)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBRTMP_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LIBRTMP_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${LIBRTMP_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${LIBRTMP_DEFINITIONS})

# ffmpeg libraries
pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libswscale)
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${FFMPEG_DEFINITIONS})

# CEF

set(CEF_VERSION "146.0.9+g3ca6a87+chromium-146.0.7680.165")

set(CEF_DOWNLOAD_PLATFORM "linux")

# CEF has a bug where it improperly sets PROJECT_ARCH on arm, so set it correctly beforehand here.
if(("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64") OR
   ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64") OR
   ("${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64"))
    set(PROJECT_ARCH "arm64")
    set(CEF_DOWNLOAD_ARCH "arm64")
elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(PROJECT_ARCH "x86_64")
    set(CEF_DOWNLOAD_ARCH "64")
else()
    message(FATAL_ERROR "Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CEF_BUILD_NAME "cef_binary_${CEF_VERSION}_${CEF_DOWNLOAD_PLATFORM}${CEF_DOWNLOAD_ARCH}_minimal")
set(CEF_DOWNLOAD_FILE "${CEF_BUILD_NAME}.tar.bz2")
set(CEF_DOWNLOAD_URL "https://cef-builds.spotifycdn.com/${CEF_DOWNLOAD_FILE}")

# Download CEF binaries. Could take a while...
FetchContent_Declare(cef
  URL ${CEF_DOWNLOAD_URL}
  URL_HASH MD5=c4f5d1f90ba5292fa7208b87344f3912
)
# libcef.so is already built, so we only need to build libcef_dll_wrapper.a
FetchContent_MakeAvailable(cef)

set(CEF_DIR "${cef_SOURCE_DIR}/")
set(CEF_RESOURCES_DIR "${CEF_DIR}/Resources/")
set(CEF_RELEASE_DIR "${CEF_DIR}/Release/")
set(CEF_RELEASE_DIR "${CEF_DIR}/Release/")
set(CEF_INCLUDE_DIR "${CEF_DIR}/include/")

# !!Important!!
# Resources must be in the same directory as the CEF library or else you'll get this very criptic error followed by a hard crash:
# [0830/190409.395581:ERROR:base/i18n/icu_util.cc:232] Invalid file descriptor to ICU data received.
file(COPY ${CEF_RESOURCES_DIR} DESTINATION ${CEF_RELEASE_DIR})

# Prebuilt libs exist here
target_link_directories(${PROJECT_NAME} PRIVATE ${CEF_RELEASE_DIR})
target_link_libraries(${PROJECT_NAME} cef libcef_dll_wrapper)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CEF_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CEF_DIR}) # INSANE decision by CEF authors to include headers relative to base dir.

### Compiler Options ###

# Treat warnings as errors
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)

# Turn off some warnings
target_compile_options(${PROJECT_NAME} PRIVATE
  # General
  -Wno-unused-parameter -Wno-unused-variable

  # Work around librtmp silliness
  -Wno-write-strings
)

