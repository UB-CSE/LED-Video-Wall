cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS "./src" "../protocol")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(client)

