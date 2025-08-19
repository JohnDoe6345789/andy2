
cmake_minimum_required(VERSION 3.22.1)

project("iotcapis")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/../../../../)

# Add the main library
add_library(IOTCAPIsT SHARED
    iotc_jni.c
    ../../../../libIOTCAPIsT.c
)

# Find required packages
find_library(log-lib log)
find_library(android-lib android)

# Link libraries
target_link_libraries(IOTCAPIsT
    ${log-lib}
    ${android-lib}
)

# Compiler flags
target_compile_options(IOTCAPIsT PRIVATE
    -Wall
    -Wextra
    -std=c99
    -fPIC
    -D_GNU_SOURCE
    -DANDROID
)
