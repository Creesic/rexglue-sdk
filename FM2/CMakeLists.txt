
# fm2 - ReXGlue Recompiled Project
#
# This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(fm2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(FM2_SOURCES
    src/main.cpp
    src/fm2_hooks.cpp
)

if(WIN32)
    add_executable(fm2 WIN32 ${FM2_SOURCES})
else()
    add_executable(fm2 ${FM2_SOURCES})
endif()

rexglue_setup_target(fm2)

# Ensure FM2 sees generated SDK headers (for example rex/version.h) when
# building against an in-tree SDK via generated/rexglue.cmake.
target_include_directories(fm2 PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui
)

# Enable Tracy profiling for guest functions (ZoneNamed per recompiled function)
target_compile_definitions(fm2 PRIVATE REXGLUE_PROFILE_GUEST_FUNCTIONS)
