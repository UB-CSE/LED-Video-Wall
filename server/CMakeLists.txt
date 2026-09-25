cmake_minimum_required(VERSION 3.25)

if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif ()

enable_testing()

### Options ###

option(BUILD_TESTS "Build unit tests" ON)

### Build type ###

# Default to debug build
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

### Project ###

project(LEDWallServer)
add_library(${PROJECT_NAME})

# compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Platform ###

if (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Unsupported platform ${CMAKE_SYSTEM_NAME}")
endif ()

### Languages ###

enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

### Sources ###

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../protocol/src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC inc inc_dummy ../protocol/src)

### Libraries ###

include(FetchContent)
find_package(PkgConfig REQUIRED)

# opencv4
pkg_check_modules(OpenCV REQUIRED opencv4)
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${OpenCV_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${OpenCV_DEFINITIONS})

# yaml-cpp
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${YAML_CPP_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${YAML_CPP_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${YAML_CPP_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${YAML_CPP_DEFINITIONS})

# freetype2
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
target_include_directories(${PROJECT_NAME} PUBLIC ${FREETYPE2_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${FREETYPE2_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${FREETYPE2_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${FREETYPE2_DEFINITIONS})

# librtmp
pkg_check_modules(LIBRTMP REQUIRED librtmp)
target_include_directories(${PROJECT_NAME} PUBLIC ${LIBRTMP_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${LIBRTMP_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${LIBRTMP_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${LIBRTMP_DEFINITIONS})

# ffmpeg libraries
pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libswscale)
target_include_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${FFMPEG_DEFINITIONS})

# spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
)
set(SPDLOG_USE_STD_FORMAT ON)
set(SPDLOG_ENABLE_PCH ON)
FetchContent_MakeAvailable(spdlog)

target_link_libraries(${PROJECT_NAME} spdlog::spdlog)

# CEF

set(CEF_VERSION "146.0.9+g3ca6a87+chromium-146.0.7680.165")

set(CEF_DOWNLOAD_PLATFORM "linux")

# CEF has a bug where it improperly sets PROJECT_ARCH on arm, so set it correctly beforehand here.
if (("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64") OR
    ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64") OR
    ("${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64"))
    set(PROJECT_ARCH "arm64")
    set(CEF_DOWNLOAD_ARCH "arm64")
elseif (CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(PROJECT_ARCH "x86_64")
    set(CEF_DOWNLOAD_ARCH "64")
else ()
    message(FATAL_ERROR "Unsupported architecture ${CMAKE_SYSTEM_PROCESSOR}")
endif ()

set(CEF_BUILD_NAME "cef_binary_${CEF_VERSION}_${CEF_DOWNLOAD_PLATFORM}${CEF_DOWNLOAD_ARCH}_minimal")
set(CEF_DOWNLOAD_FILE "${CEF_BUILD_NAME}.tar.bz2")
set(CEF_DOWNLOAD_URL "https://cef-builds.spotifycdn.com/${CEF_DOWNLOAD_FILE}")

# Download CEF binaries. Could take a while...
FetchContent_Declare(
        cef
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
target_link_directories(${PROJECT_NAME} PUBLIC ${CEF_RELEASE_DIR})
target_link_libraries(${PROJECT_NAME} cef libcef_dll_wrapper)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CEF_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CEF_DIR}) # INSANE decision by CEF authors to include headers relative to base dir.

### Compiler Options ###

# Treat warnings as errors
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Werror)

# Turn off some warnings
target_compile_options(${PROJECT_NAME} PUBLIC
        # General
        -Wno-unused-parameter -Wno-unused-variable

        # Work around librtmp silliness
        -Wno-write-strings
)

### Build executable ###

add_executable(led-wall-server src/main.cpp)
target_link_libraries(led-wall-server ${PROJECT_NAME})

### Tests ###
if (BUILD_TESTS)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.18.0
    )
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)

    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

    set(TESTS_NAME led-wall-server-tests)

    add_executable(${TESTS_NAME} ${TEST_SOURCES})
    target_link_libraries(${TESTS_NAME} ${PROJECT_NAME})

    target_compile_definitions(${TESTS_NAME} PUBLIC "IMAGES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/images\"")

    target_link_libraries(${TESTS_NAME} GTest::gtest_main GTest::gmock_main)
    gtest_discover_tests(${TESTS_NAME})
endif ()