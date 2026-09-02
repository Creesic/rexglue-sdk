# fm4 - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(fm4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(FM4_SOURCES
    src/main.cpp
    src/fm4_guest_pc_fiber.cpp
    src/fm4_debug_media.cpp
    src/fm4_debug_audio_mix.cpp
    src/fm4_midasm_hooks.cpp
    src/gpu/fm4_d3d_trace.cpp
)

if(WIN32)
    add_executable(fm4 WIN32 ${FM4_SOURCES})
else()
    add_executable(fm4 ${FM4_SOURCES})
endif()

rexglue_setup_target(fm4 GPU_PLUGINS xenos)

add_custom_command(TARGET fm4 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/fm4.toml"
    "$<TARGET_FILE_DIR:fm4>/fm4.toml"
  COMMENT "Copy fm4.toml beside fm4.exe")

target_include_directories(fm4 PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
