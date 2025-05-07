cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "./src" "../protocol")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# adds a custom command to generate build/version.txt 
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/version.txt
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}
    COMMAND bash -c "printf \"%s\" \"$(grep -E \"#define CURRENT_FW_VERSION\" ${CMAKE_SOURCE_DIR}/src/ota.hpp | sed 's/#define CURRENT_FW_VERSION \"\\(.*\\)\"/\\1/')\" > ${CMAKE_BINARY_DIR}/version.txt"
    DEPENDS ${CMAKE_SOURCE_DIR}/src/ota.hpp
    COMMENT "Generating version.txt from ota.hpp"
    VERBATIM
)

# ensure build/version.txt is built before the client binary
add_custom_target(generate_version_txt ALL
    DEPENDS ${CMAKE_BINARY_DIR}/version.txt
)

project(client)
