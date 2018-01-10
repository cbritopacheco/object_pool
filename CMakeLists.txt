# ---- Set cmake minimum version ---------------
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
cmake_policy(SET CMP0048 NEW)

project(object_pool VERSION 0.0.1 LANGUAGES CXX)

# ---- Build options ---------------------------
option(BUILD_TESTS          "Builds unit tests"     ON)
option(BUILD_EXAMPLES       "Builds examples"       OFF )
option(INCLUDE_COVERAGE     "Builds examples"       ON )

# ---- Configuration ---------------------------
include("${PROJECT_SOURCE_DIR}/cmake/configuration.cmake")

# ---- Set the module path ---------------------
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

# ---- Print copyright message -----------------
message("${blue} ----------------------------------------------------------- ${reset}")
message("${blue} --- Copyright (C) Carlos Brito 2017 All rights reserved --- ${reset}")
message("${blue} ----------- Boost Software License Version 1.0 ------------ ${reset}")
message("${green} -------------- THIS SOFTWARE IS OPEN SOURCE --------------- ${reset}")
message("${blue} ----------------------------------------------------------- ${reset}")

# ---- Configure compiler ----------------------
set(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Catch dependency -------------------------
message("${gray}Catch2 is needed to perform the tests.${reset}")

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/catch/catch.hpp")
    message("${gray}We will attempt to download the latest version from the internet.${reset}")
    file(MAKE_DIRECTORY "catch")
    file(DOWNLOAD
         "https://github.com/catchorg/Catch2/releases/download/v2.0.1/catch.hpp"
         "${PROJECT_SOURCE_DIR}/catch/catch.hpp"
         TIMEOUT    10
         STATUS     DOWNLOAD_STATUS
         SHOW_PROGRESS
         )
    list(GET ${DOWNLOAD_STATUS} 0 IS_ERROR)
    
    if (${IS_ERROR})
        message(FATAL_ERROR "${green}Download completed succesfully!${reset}")
    endif()
endif()

# ---- Build -----------------------------------
message("${blue} Generating makefiles... ${reset}")

include_directories("catch/")
include_directories(".")

# --- Gather all tests ---------------
file(GLOB_RECURSE test_glob "test/*.cpp")
set(test_SRC ${test_glob})

# ---- Add an executable -----------------------
add_executable(unit_tests "${test_SRC}")

include(CodeCoverage)
message("${PROJECT_SOURCE_DIR}")
set(LCOV_REMOVE_EXTRA "'test/*'" "'catch/*'")
setup_target_for_coverage (coverage unit_tests coverage)