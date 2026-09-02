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
    src/render/video.cpp
    src/render/render_queue.cpp
    src/render/render_state.cpp
    src/render/d3d_resources.cpp
    src/render/pipeline.cpp
    generated/shader_cache.cpp
)

if(WIN32)
    add_executable(fm4 WIN32 ${FM4_SOURCES})
else()
    add_executable(fm4 ${FM4_SOURCES})
endif()

# Plume render abstraction, vendored in the SDK tree at thirdparty/plume and
# built here because this tree consumes the SDK as an installed package.
# Plume at the pinned commit has no per-backend options; the Agility SDK is
# off because this tree carries no directx12-agility package.
set(PLUME_D3D12_AGILITY_SDK_ENABLED OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/plume plume EXCLUDE_FROM_ALL)

rexglue_setup_target(fm4 GPU_PLUGINS xenos)

# Plume is linked, not runtime-loaded; it does not propagate d3d12/dxgi.
target_link_libraries(fm4 PRIVATE plume)
if(WIN32)
    # Plume's own CMakeLists.txt does not link d3d12/dxgi to its target (see
    # thirdparty/plume/examples/*/CMakeLists.txt, which link these on the
    # executable the same way); without them, fm4.exe fails to link with
    # unresolved D3D12CreateDevice / D3D12SerializeRootSignature.
    target_link_libraries(fm4 PRIVATE d3d12 dxgi)
endif()

#=============================================================================
# FM2P-derived native renderer: pinned DXC for the blit shaders and the
# runtime spec-constant link, zstd for the shader-cache blobs, xxHash for the
# shader/declaration cache keys, RenderDoc's header-only in-app capture API.
#=============================================================================
set(FM4_DXC_EXECUTABLE
    "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/dxc-bin/bin/x64/dxc.exe")
if(NOT EXISTS "${FM4_DXC_EXECUTABLE}")
    message(FATAL_ERROR
        "thirdparty/dxc-bin is missing: git submodule update --init thirdparty/dxc-bin. "
        "The system DXC cannot be substituted -- IDxcLinker refuses cross-version DXIL.")
endif()

set(FM4_GENERATED_SHADER_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(FM4_GENERATED_SHADER_BINARY_DIR  "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")

function(fm4_compile_dxil_shader OUT_VAR SOURCE_FILE PROFILE ENTRY_POINT
         ARRAY_NAME HEADER_SUBDIR HEADER_BASENAME)
    get_filename_component(shader_name "${SOURCE_FILE}" NAME)
    set(dxil_output "${FM4_GENERATED_SHADER_BINARY_DIR}/${shader_name}.dxil")
    set(header_output
        "${FM4_GENERATED_SHADER_INCLUDE_DIR}/${HEADER_SUBDIR}/${HEADER_BASENAME}.dxil.h")
    add_custom_command(
        OUTPUT "${dxil_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${FM4_GENERATED_SHADER_BINARY_DIR}"
        COMMAND "${FM4_DXC_EXECUTABLE}" -E "${ENTRY_POINT}" -T "${PROFILE}"
                -Fo "${dxil_output}" "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}"
        COMMENT "DXC ${SOURCE_FILE}" VERBATIM)
    add_custom_command(
        OUTPUT "${header_output}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT_FILE=${dxil_output}"
                "-DOUTPUT_FILE=${header_output}" "-DARRAY_NAME=${ARRAY_NAME}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinaryHeader.cmake"
        DEPENDS "${dxil_output}"
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinaryHeader.cmake"
        COMMENT "Embedding ${shader_name}.dxil" VERBATIM)
    set(${OUT_VAR} ${${OUT_VAR}} "${header_output}" PARENT_SCOPE)
endfunction()

set(FM4_BLIT_SHADER_HEADERS)
fm4_compile_dxil_shader(FM4_BLIT_SHADER_HEADERS
    src/render/shaders/copy_vs.hlsl vs_6_0 main g_copy_vs_dxil render/shaders copy_vs.hlsl)
fm4_compile_dxil_shader(FM4_BLIT_SHADER_HEADERS
    src/render/shaders/copy_ps.hlsl ps_6_0 main g_copy_ps_dxil render/shaders copy_ps.hlsl)
add_custom_target(fm4_blit_shaders DEPENDS ${FM4_BLIT_SHADER_HEADERS})
add_dependencies(fm4 fm4_blit_shaders)
target_sources(fm4 PRIVATE ${FM4_BLIT_SHADER_HEADERS})
target_include_directories(fm4 BEFORE PRIVATE "${FM4_GENERATED_SHADER_INCLUDE_DIR}")

# The runtime LoadLibraryW for DXC must find the SAME pair dxc.exe came from.
get_filename_component(FM4_DXC_DIR "${FM4_DXC_EXECUTABLE}" DIRECTORY)
foreach(dxc_runtime_dll dxcompiler.dll dxil.dll)
    add_custom_command(TARGET fm4 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${FM4_DXC_DIR}/${dxc_runtime_dll}" "$<TARGET_FILE_DIR:fm4>/${dxc_runtime_dll}"
        COMMENT "Staging ${dxc_runtime_dll} next to fm4")
endforeach()

if(NOT TARGET libzstd_static)
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/zstd/build/cmake"
                     "${CMAKE_CURRENT_BINARY_DIR}/zstd" EXCLUDE_FROM_ALL)
endif()
target_link_libraries(fm4 PRIVATE libzstd_static)
target_include_directories(fm4 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/zstd/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/xxHash"
    "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/renderdoc")
target_compile_definitions(fm4 PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

#=============================================================================
# Offline Xenos -> DXIL/SPIR-V shader cache (opt-in, needs tools/XenosRecomp).
#=============================================================================
set(FM4_XENOS_RECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../tools/XenosRecomp")
if(EXISTS "${FM4_XENOS_RECOMP_ROOT}/XenosRecomp/CMakeLists.txt")
    # XenosRecomp's thirdparty/CMakeLists.txt resolves every nested dependency
    # against this variable and its own CMakeLists never sets it; without both
    # of these (copied from ReXFM2P/FM2/CMakeLists.txt:150-157) configure fails
    # on "add_subdirectory given source /dxc-bin".
    set(XENOS_RECOMP_THIRDPARTY_ROOT "${FM4_XENOS_RECOMP_ROOT}/thirdparty")
    if(WIN32 AND NOT CMAKE_SYSTEM_PROCESSOR)
        if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
            set(CMAKE_SYSTEM_PROCESSOR AMD64)
        elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
            set(CMAKE_SYSTEM_PROCESSOR ARM64)
        endif()
    endif()
    add_subdirectory("${FM4_XENOS_RECOMP_ROOT}/thirdparty"
        "${CMAKE_CURRENT_BINARY_DIR}/XenosRecomp/thirdparty" EXCLUDE_FROM_ALL)
    add_subdirectory("${FM4_XENOS_RECOMP_ROOT}/XenosRecomp"
        "${CMAKE_CURRENT_BINARY_DIR}/XenosRecomp/tool" EXCLUDE_FROM_ALL)
    add_custom_target(fm4_regenerate_shader_cache
        COMMAND "${CMAKE_COMMAND}"
            "-DXENOS_RECOMP=$<TARGET_FILE:XenosRecomp>"
            "-DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/assets/missed_shaders"
            "-DSHADER_COMMON=${FM4_XENOS_RECOMP_ROOT}/XenosRecomp/shader_common.h"
            "-DOUTPUT=${CMAKE_CURRENT_SOURCE_DIR}/generated/shader_cache.cpp"
            "-DVERIFY_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyShaderCache.cmake"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateShaderCache.cmake"
        DEPENDS XenosRecomp
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateShaderCache.cmake"
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyShaderCache.cmake"
        COMMENT "Regenerating the FM4 shader cache" VERBATIM USES_TERMINAL)
else()
    message(WARNING "tools/XenosRecomp is not initialized; shader-cache regeneration unavailable")
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
    ${CMAKE_CURRENT_SOURCE_DIR}
)
