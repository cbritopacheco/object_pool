# ---- Set cmake minimum version ---------------
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)

project(object_pool VERSION 0.0.1 LANGUAGES CXX)

# ---- Configuration ---------------------------
include("${PROJECT_SOURCE_DIR}/cmake/configuration.cmake")

# ---- Set the module path ---------------------
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

# ---- Print copyright message -----------------
message("${blue} ----------------------------------------------------------- ${reset}")
message("${blue} --- Copyright (C) Carlos Brito 2017 All rights reserved --- ${reset}")
message("${blue} ----------- Boost Software License Version 1.0 ------------ ${reset}")
message("${blue} --------------${green} THIS SOFTWARE IS OPEN SOURCE ${blue}--------------- ${reset}")
message("${blue} ----------------------------------------------------------- ${reset}")

add_subdirectory(tests)