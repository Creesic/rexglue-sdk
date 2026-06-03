
# fh1 - ReXGlue Recompiled Project
#
# This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(fh1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

include(generated/rexglue.cmake)

# Sources
set(FH1_SOURCES
    src/main.cpp
    src/roundevenf_compat.cpp
)

if(WIN32)
    add_executable(fh1 WIN32 ${FH1_SOURCES})
else()
    add_executable(fh1 ${FH1_SOURCES})
endif()

rexglue_setup_target(fh1)

if(WIN32)
    target_link_options(fh1 PRIVATE /INCREMENTAL:NO)
endif()
