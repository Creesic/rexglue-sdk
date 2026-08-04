
# fm2 - ReXGlue Recompiled Project
#
# This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(fm2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

set(FM2_SOURCES
    src/main.cpp
    src/fm2_hooks.cpp
    src/render/render_patches.cpp
    src/render/fm2_device.cpp
    src/render/video.cpp
    src/render/d3d_hooks.cpp
    src/render/d3d_resource_hooks.cpp
    src/render/pipeline.cpp
    src/render/render_state.cpp
    src/render/shader_probe_window.cpp
    src/native_renderer/fm2_native_renderer.cpp
    src/native_renderer/fm2_native_state.cpp
    generated/shader_cache.cpp
)

if(WIN32)
    add_executable(fm2 WIN32 ${FM2_SOURCES})
else()
    add_executable(fm2 ${FM2_SOURCES})
endif()

rexglue_setup_target(fm2 GPU_PLUGINS xenos)

# pipeline.cpp wraps DXC/dxcompiler.dll calls in __try/__except to survive crashes
# from malformed regenerated shader blobs. clang-cl's default /EHsc does NOT let
# __except catch asynchronous hardware exceptions (access violations) -- the guard
# silently does nothing. /EHa (async EH) is required for the catch to actually fire.
if(WIN32)
    set_source_files_properties(src/render/pipeline.cpp PROPERTIES COMPILE_OPTIONS "/EHa")
endif()

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../plume/CMakeLists.txt")
    set(PLUME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    if(NOT TARGET plume)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../plume" "${CMAKE_CURRENT_BINARY_DIR}/plume")
    endif()

    set(FM2_DEFAULT_ZSTD_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../../ReOdyssey/thirdparty/zstd")
    set(FM2_ZSTD_DIR
        "${FM2_DEFAULT_ZSTD_DIR}"
        CACHE PATH "zstd source for FM2 shader cache decompression")
    if(NOT EXISTS "${FM2_ZSTD_DIR}/build/cmake/CMakeLists.txt"
       AND EXISTS "${FM2_DEFAULT_ZSTD_DIR}/build/cmake/CMakeLists.txt")
        set(FM2_ZSTD_DIR "${FM2_DEFAULT_ZSTD_DIR}" CACHE PATH
            "zstd source for FM2 shader cache decompression" FORCE)
    endif()
    if(EXISTS "${FM2_ZSTD_DIR}/build/cmake/CMakeLists.txt")
        set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
        set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
        set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        add_subdirectory("${FM2_ZSTD_DIR}/build/cmake" "${CMAKE_CURRENT_BINARY_DIR}/zstd_build")
    endif()

    file(GLOB FM2_WINDOWS_KIT_DXC_DIRS
        "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
    list(SORT FM2_WINDOWS_KIT_DXC_DIRS)
    list(REVERSE FM2_WINDOWS_KIT_DXC_DIRS)
    find_program(FM2_DXC_EXECUTABLE
        NAMES dxc.exe
        PATHS
            ${FM2_WINDOWS_KIT_DXC_DIRS}
            "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64"
        DOC "DXC executable used for FM2 Plume blit shaders")

    set(FM2_GENERATED_SHADER_INCLUDE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
    set(FM2_GENERATED_SHADER_BINARY_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")

    function(fm2_compile_dxil_shader OUT_VAR SOURCE_FILE PROFILE ENTRY_POINT
             ARRAY_NAME HEADER_SUBDIR HEADER_BASENAME)
        get_filename_component(shader_name "${SOURCE_FILE}" NAME)
        set(dxil_output
            "${FM2_GENERATED_SHADER_BINARY_DIR}/${shader_name}.dxil")
        set(header_output
            "${FM2_GENERATED_SHADER_INCLUDE_DIR}/${HEADER_SUBDIR}/${HEADER_BASENAME}.dxil.h")

        add_custom_command(
            OUTPUT "${dxil_output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${FM2_GENERATED_SHADER_BINARY_DIR}"
            COMMAND "${FM2_DXC_EXECUTABLE}"
                -E "${ENTRY_POINT}"
                -T "${PROFILE}"
                -Fo "${dxil_output}"
                "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}"
            COMMENT "DXC ${SOURCE_FILE}"
            VERBATIM
        )

        add_custom_command(
            OUTPUT "${header_output}"
            COMMAND "${CMAKE_COMMAND}"
                "-DINPUT_FILE=${dxil_output}"
                "-DOUTPUT_FILE=${header_output}"
                "-DARRAY_NAME=${ARRAY_NAME}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinaryHeader.cmake"
            DEPENDS
                "${dxil_output}"
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinaryHeader.cmake"
            COMMENT "Embedding ${shader_name}.dxil"
            VERBATIM
        )

        set(${OUT_VAR} ${${OUT_VAR}} "${header_output}" PARENT_SCOPE)
    endfunction()

    set(FM2_BLIT_SHADER_HEADERS)
    if(FM2_DXC_EXECUTABLE)
        fm2_compile_dxil_shader(FM2_BLIT_SHADER_HEADERS
            src/render/shaders/copy_vs.hlsl vs_6_0 main g_copy_vs_dxil
            render/shaders copy_vs.hlsl)
        fm2_compile_dxil_shader(FM2_BLIT_SHADER_HEADERS
            src/render/shaders/copy_ps.hlsl ps_6_0 main g_copy_ps_dxil
            render/shaders copy_ps.hlsl)
        fm2_compile_dxil_shader(FM2_BLIT_SHADER_HEADERS
            src/native_renderer/shaders/fm2_debug_replay.vert.hlsl vs_6_0
            VSMain fm2DebugReplayVertBlobDXIL
            shaders fm2DebugReplayVert.hlsl)
        fm2_compile_dxil_shader(FM2_BLIT_SHADER_HEADERS
            src/native_renderer/shaders/fm2_debug_replay_native28.vert.hlsl
            vs_6_0 VSMain fm2DebugReplayNative28VertBlobDXIL
            shaders fm2DebugReplayNative28Vert.hlsl)
        fm2_compile_dxil_shader(FM2_BLIT_SHADER_HEADERS
            src/native_renderer/shaders/fm2_debug_replay.frag.hlsl ps_6_0
            PSMain fm2DebugReplayFragBlobDXIL
            shaders fm2DebugReplayFrag.hlsl)
        add_custom_target(fm2_blit_shaders DEPENDS ${FM2_BLIT_SHADER_HEADERS})
        add_dependencies(fm2 fm2_blit_shaders)
        target_sources(fm2 PRIVATE ${FM2_BLIT_SHADER_HEADERS})
        target_include_directories(fm2 BEFORE PRIVATE
            "${FM2_GENERATED_SHADER_INCLUDE_DIR}")
    else()
        message(WARNING "DXC not found; FM2 native renderer blit shaders will be missing")
    endif()

    target_link_libraries(fm2 PRIVATE plume d3d12 dxgi)
    if(TARGET libzstd_static)
        target_link_libraries(fm2 PRIVATE libzstd_static)
        target_include_directories(fm2 PRIVATE "${FM2_ZSTD_DIR}/lib")
    endif()
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=1 NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=0)
endif()

target_include_directories(fm2 PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/rexglue-sdk/include
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/xxHash
)

target_compile_definitions(fm2 PRIVATE REXGLUE_PROFILE_GUEST_FUNCTIONS)
