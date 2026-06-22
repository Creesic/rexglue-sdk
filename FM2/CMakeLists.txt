
# fm2 - ReXGlue Recompiled Project
#
# This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(fm2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(FM2_SOURCES
    src/main.cpp
    src/fm2_hooks.cpp
    src/native_renderer/fm2_native_state.cpp
    src/native_renderer/fm2_native_renderer.cpp
)

if(WIN32)
    add_executable(fm2 WIN32 ${FM2_SOURCES})
else()
    add_executable(fm2 ${FM2_SOURCES})
endif()

rexglue_setup_target(fm2)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../plume/CMakeLists.txt")
    set(PLUME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    if(NOT TARGET plume)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../plume" "${CMAKE_CURRENT_BINARY_DIR}/plume")
    endif()
    file(GLOB FM2_WINDOWS_KIT_DXC_DIRS
        "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
    list(SORT FM2_WINDOWS_KIT_DXC_DIRS)
    list(REVERSE FM2_WINDOWS_KIT_DXC_DIRS)
    find_program(PLUME_DXC_EXECUTABLE
        NAMES dxc.exe
        PATHS
            ${FM2_WINDOWS_KIT_DXC_DIRS}
            "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64"
        DOC "DXC executable used for FM2 Plume debug shaders")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../plume/examples/cmake/PlumeShaders.cmake")
    plume_shaders_init()
    set(FM2_PLUME_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    plume_compile_vertex_shader(
        fm2
        "${CMAKE_CURRENT_SOURCE_DIR}/src/native_renderer/shaders/fm2_debug_replay.vert.hlsl"
        "fm2DebugReplayVert"
        "VSMain"
        OUTPUT_DIR "${FM2_PLUME_SHADER_OUTPUT_DIR}")
    plume_compile_pixel_shader(
        fm2
        "${CMAKE_CURRENT_SOURCE_DIR}/src/native_renderer/shaders/fm2_debug_replay.frag.hlsl"
        "fm2DebugReplayFrag"
        "PSMain"
        OUTPUT_DIR "${FM2_PLUME_SHADER_OUTPUT_DIR}")
    add_custom_target(fm2_plume_debug_replay_shaders
        DEPENDS
            "${FM2_PLUME_SHADER_OUTPUT_DIR}/fm2DebugReplayVert.hlsl.dxil.h"
            "${FM2_PLUME_SHADER_OUTPUT_DIR}/fm2DebugReplayFrag.hlsl.dxil.h")
    add_dependencies(fm2 fm2_plume_debug_replay_shaders)
    target_link_libraries(fm2 PRIVATE plume d3d12 dxgi)
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=1)
else()
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=0)
endif()

# Ensure FM2 sees generated SDK headers (for example rex/version.h) when
# building against an in-tree SDK via generated/rexglue.cmake.
target_include_directories(fm2 PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/xxHash
)

# Enable Tracy profiling for guest functions (ZoneNamed per recompiled function)
target_compile_definitions(fm2 PRIVATE REXGLUE_PROFILE_GUEST_FUNCTIONS)
