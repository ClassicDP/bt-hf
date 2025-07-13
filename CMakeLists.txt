cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bt-hf)

set(CONFIG_PARTITION_TABLE_SINGLE_APP "y")
