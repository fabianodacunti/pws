# Proof of concept: I2C sensors on an ESP32-S3, read in a single loop.
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pws_poc)
