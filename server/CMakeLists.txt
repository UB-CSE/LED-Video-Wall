cmake_minimum_required(VERSION 3.10)
project(virtual_canvas)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(yaml-cpp REQUIRED)

# Add include directories
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${YAML_CPP_INCLUDE_DIR})

# Require GUI support 
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)

# Create library target
add_library(canvas_lib
    canvas.cpp
)

# Link libraries
target_link_libraries(canvas_lib
    ${OpenCV_LIBS}
    yaml-cpp
)

# Create executable target
add_executable(canvas_test
    main.cpp
)

# Link executable with our library
target_link_libraries(canvas_test
    canvas_lib
)
