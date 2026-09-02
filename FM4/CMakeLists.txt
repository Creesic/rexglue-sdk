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
    src/gpu/fm4_d3d_hooks.cpp
    src/gpu/fm4_d3d_trace.cpp
    src/gpu/native_video.cpp
)

if(WIN32)
    add_executable(fm4 WIN32 ${FM4_SOURCES})
else()
    add_executable(fm4 ${FM4_SOURCES})
endif()

# Plume render abstraction, vendored in the SDK tree at thirdparty/plume. FM4
# consumes the SDK as an installed package, so the SDK's REXGLUE_ENABLE_PLUME
# option does not reach this tree; add the submodule directly. Task 4 links it.
# Plume at pin de0f70f has no PLUME_D3D12_ENABLED/PLUME_VULKAN_ENABLED options
# -- it builds D3D12 automatically on WIN32 and Vulkan unconditionally on every
# platform (see thirdparty/plume/CMakeLists.txt) -- so only the real Agility
# SDK toggle is set here.
set(PLUME_D3D12_AGILITY_SDK_ENABLED OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/plume plume EXCLUDE_FROM_ALL)

rexglue_setup_target(fm4 GPU_PLUGINS xenos)

# Plume is added by the add_subdirectory block above (Task 3). Linked, not
# runtime-loaded.
target_link_libraries(fm4 PRIVATE plume)
if(WIN32)
    # Plume's own CMakeLists.txt does not link d3d12/dxgi to its target (see
    # thirdparty/plume/examples/*/CMakeLists.txt, which link these on the
    # executable the same way); without them, fm4.exe fails to link with
    # unresolved D3D12CreateDevice / D3D12SerializeRootSignature.
    target_link_libraries(fm4 PRIVATE d3d12 dxgi)
endif()

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
