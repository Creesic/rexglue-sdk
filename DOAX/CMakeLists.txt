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

# Visual Studio debugger hints (optional; doax_app.h resolves assets without this).
if(WIN32)
    set_target_properties(doax PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:doax>"
        VS_DEBUGGER_COMMAND_ARGUMENTS "--game_data_root=${CMAKE_CURRENT_SOURCE_DIR}/assets"
    )
endif()

# In-tree SDK builds emit generated headers under the rexglue-sdk binary dir.
target_include_directories(doax PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui
)
