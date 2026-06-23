# doax - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(doax LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(DOAX_SOURCES
    src/main.cpp
    src/doax_hooks.cpp
)

if(WIN32)
    add_executable(doax WIN32 ${DOAX_SOURCES})
else()
    add_executable(doax ${DOAX_SOURCES})
endif()

rexglue_setup_target(doax)

# In-tree SDK builds emit generated headers under the rexglue-sdk binary dir.
target_include_directories(doax PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui
)
