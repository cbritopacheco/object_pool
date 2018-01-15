# ---- Build options ---------------------------
option(BUILD_TESTS          "Builds unit tests"     ON)
option(BUILD_EXAMPLES       "Builds examples"       OFF )
option(INCLUDE_COVERAGE     "Builds examples"       ON )

include(CodeCoverage)
set(LCOV_REMOVE_EXTRA "'catch/*'" "'examples/*'" "'cmake/*'" "'build/*'" "'tests/*'")
setup_target_for_coverage (coverage unit_tests coverage)

# ---- Configure compiler ----------------------
set(CMAKE_CXX_FLAGS "-g -O0 --coverage -fno-inline -fno-inline-small-functions -fno-default-inline -fprofile-arcs -ftest-coverage -pthread")
set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Catch dependency -------------------------
message("${gray}Catch2 is needed to perform the tests.${reset}")

if (NOT EXISTS "${PROJECT_SOURCE_DIR}/tests/catch/catch.hpp")
    message("${gray}We will attempt to download the latest version from the internet.${reset}")
    file(MAKE_DIRECTORY "catch")
    file(DOWNLOAD
         "https://github.com/catchorg/Catch2/releases/download/v2.0.1/catch.hpp"
         "${PROJECT_SOURCE_DIR}/tests/catch/catch.hpp"
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

include_directories("${PROJECT_SOURCE_DIR}/tests/catch/")
include_directories("${PROJECT_SOURCE_DIR}/src/")

# --- Gather all tests ---------------
file(GLOB_RECURSE test_glob "*.cpp")
set(test_SRC ${test_glob})

# ---- Add an executable -----------------------
add_executable(unit_tests "${test_SRC}")
target_link_libraries(unit_tests)
