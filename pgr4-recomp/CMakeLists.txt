# pgr4_recompiled - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(pgr4_recompiled LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Native Plume renderer.
#
# OFF by default: with it off none of src/render/ is compiled, no D3D entry
# point is hooked, and the xenos GPU plugin path is bit-identical to a build
# without this option. It cannot be a runtime cvar -- REX_HOOK replaces a
# recompiled function at link time with no fall-through to the original body,
# so a hook disabled at runtime could not restore guest D3DDevice_Swap.
option(PGR4_ENABLE_PLUME "Build the native Plume renderer instead of the xenos GPU plugin" OFF)

# Sources
set(PGR4RECOMPILED_SOURCES
    src/main.cpp
    src/pad_script.cpp
)

if(PGR4_ENABLE_PLUME)
    list(APPEND PGR4RECOMPILED_SOURCES
        # Guest GPU: vblank + minimal PM4 executor (ours; FM2 has no equivalent)
        src/render/guest_gpu.cpp
        # Plume renderer, ported from FM2/src/render
        src/render/video.cpp
        src/render/render_queue.cpp
        src/render/d3d_hooks.cpp
        src/render/d3d_resource_hooks.cpp
        src/render/pipeline.cpp
        src/render/render_state.cpp
        # XenosRecomp-translated shader cache (regenerated from assets/missed_shaders)
        generated/shader_cache.cpp
    )
endif()

if(WIN32)
    add_executable(pgr4_recompiled WIN32 ${PGR4RECOMPILED_SOURCES})
else()
    add_executable(pgr4_recompiled ${PGR4RECOMPILED_SOURCES})
endif()

rexglue_setup_target(pgr4_recompiled)

if(PGR4_ENABLE_PLUME)
    set(PGR4_THIRDPARTY "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty")
    set(PGR4_PLUME_DIR "${PGR4_THIRDPARTY}/plume")

    # Checking only for plume/CMakeLists.txt is not enough: plume carries its
    # own submodules, and `git submodule add` does not recurse into them. That
    # file is present while contrib/ is still empty, and the failure surfaces
    # later as "Cannot find source file" / "No SOURCES given to target: plume".
    set(PGR4_PLUME_REQUIRED_SOURCES
        "${PGR4_PLUME_DIR}/CMakeLists.txt"
        "${PGR4_PLUME_DIR}/contrib/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"
        "${PGR4_PLUME_DIR}/contrib/volk/volk.c"
        "${PGR4_PLUME_DIR}/contrib/VulkanMemoryAllocator/include/vk_mem_alloc.h"
        "${PGR4_PLUME_DIR}/contrib/Vulkan-Headers/include/vulkan/vulkan.h"
        "${PGR4_THIRDPARTY}/zstd/build/cmake/CMakeLists.txt"
        "${PGR4_THIRDPARTY}/dxc-bin/bin/x64/dxc.exe"
    )
    set(PGR4_PLUME_MISSING "")
    foreach(required_source IN LISTS PGR4_PLUME_REQUIRED_SOURCES)
        if(NOT EXISTS "${required_source}")
            list(APPEND PGR4_PLUME_MISSING "${required_source}")
        endif()
    endforeach()
    if(PGR4_PLUME_MISSING)
        string(REPLACE ";" "\n    " PGR4_PLUME_MISSING_TEXT "${PGR4_PLUME_MISSING}")
        message(FATAL_ERROR
            "PGR4_ENABLE_PLUME is ON but the renderer's dependencies are not fully checked out. Missing:\n"
            "    ${PGR4_PLUME_MISSING_TEXT}\n\n"
            "Run:\n"
            "    git submodule update --init --recursive thirdparty/plume thirdparty/zstd thirdparty/dxc-bin tools/XenosRecomp")
    endif()

    set(PLUME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    if(NOT TARGET plume)
        add_subdirectory("${PGR4_PLUME_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/plume")
    endif()

    # XenosRecomp: regenerates generated/shader_cache.cpp from the shader
    # microcode the game dumped into assets/missed_shaders/*.bin on cache misses.
    set(PGR4_XENOS_RECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../tools/XenosRecomp")
    if(EXISTS "${PGR4_XENOS_RECOMP_ROOT}/XenosRecomp/CMakeLists.txt")
        set(XENOS_RECOMP_THIRDPARTY_ROOT "${PGR4_XENOS_RECOMP_ROOT}/thirdparty")
        if(WIN32 AND NOT CMAKE_SYSTEM_PROCESSOR)
            if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
                set(CMAKE_SYSTEM_PROCESSOR AMD64)
            elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
                set(CMAKE_SYSTEM_PROCESSOR ARM64)
            endif()
        endif()
        add_subdirectory("${PGR4_XENOS_RECOMP_ROOT}/thirdparty"
            "${CMAKE_CURRENT_BINARY_DIR}/XenosRecomp/thirdparty" EXCLUDE_FROM_ALL)
        add_subdirectory("${PGR4_XENOS_RECOMP_ROOT}/XenosRecomp"
            "${CMAKE_CURRENT_BINARY_DIR}/XenosRecomp/tool" EXCLUDE_FROM_ALL)
        add_custom_target(pgr4_regenerate_shader_cache
            COMMAND "${CMAKE_COMMAND}"
                "-DXENOS_RECOMP=$<TARGET_FILE:XenosRecomp>"
                "-DINPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/assets/missed_shaders"
                "-DSHADER_COMMON=${PGR4_XENOS_RECOMP_ROOT}/XenosRecomp/shader_common.h"
                "-DOUTPUT=${CMAKE_CURRENT_SOURCE_DIR}/generated/shader_cache.cpp"
                "-DVERIFY_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyShaderCache.cmake"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateShaderCache.cmake"
            DEPENDS XenosRecomp
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RegenerateShaderCache.cmake"
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyShaderCache.cmake"
            COMMENT "Regenerating the PGR4 shader cache from assets/missed_shaders"
            VERBATIM USES_TERMINAL)
    else()
        message(WARNING "tools/XenosRecomp is not initialized; shader-cache regeneration is unavailable")
    endif()

    # zstd: the shader cache is stored compressed.
    if(NOT TARGET libzstd_static)
        set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
        set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
        set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
        add_subdirectory("${PGR4_THIRDPARTY}/zstd/build/cmake" "${CMAKE_CURRENT_BINARY_DIR}/zstd")
        target_compile_options(libzstd_static PRIVATE -w)
    endif()

    # plume builds its D3D12 backend but leaves the system import libraries to
    # the consumer, so D3D12CreateDevice / D3D12SerializeRootSignature are
    # unresolved without these. Matches what FM2 links.
    target_link_libraries(pgr4_recompiled PRIVATE plume libzstd_static)
    if(WIN32)
        target_link_libraries(pgr4_recompiled PRIVATE d3d12 dxgi)
    endif()

    # Use the same pinned DXC build XenosRecomp vendors (thirdparty/dxc-bin). DXC
    # embeds a compiler-version tag in every DXIL library and its linker refuses
    # to link libraries built by different versions: the runtime spec-constant
    # link in pipeline.cpp must use the SAME dxcompiler.dll that produced the
    # shader cache, or every specialised shader fails to link.
    set(PGR4_DXC_DIR "${PGR4_THIRDPARTY}/dxc-bin")
    set(PGR4_DXC_EXECUTABLE "${PGR4_DXC_DIR}/bin/x64/dxc.exe")

    target_include_directories(pgr4_recompiled PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}"            # generated/shader_cache.h
        "${PGR4_PLUME_DIR}"
        "${PGR4_DXC_DIR}/inc"                    # dxcapi.h
        "${PGR4_THIRDPARTY}/zstd/lib"
        "${PGR4_THIRDPARTY}/xxHash"
        "${PGR4_THIRDPARTY}/renderdoc"           # renderdoc_app.h (header-only)
        "${CMAKE_CURRENT_BINARY_DIR}/generated/include"
    )
    target_compile_definitions(pgr4_recompiled PRIVATE PGR4_ENABLE_PLUME=1 NOMINMAX WIN32_LEAN_AND_MEAN)

    # Blit shaders: HLSL -> DXIL via DXC -> embedded byte-array headers.
    set(PGR4_GENERATED_SHADER_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
    set(PGR4_GENERATED_SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")

    function(pgr4_compile_dxil_shader OUT_VAR SOURCE_FILE PROFILE ENTRY_POINT
             ARRAY_NAME HEADER_SUBDIR HEADER_BASENAME)
        get_filename_component(shader_name "${SOURCE_FILE}" NAME)
        set(dxil_output "${PGR4_GENERATED_SHADER_BINARY_DIR}/${shader_name}.dxil")
        set(header_output "${PGR4_GENERATED_SHADER_INCLUDE_DIR}/${HEADER_SUBDIR}/${HEADER_BASENAME}.dxil.h")
        add_custom_command(
            OUTPUT "${dxil_output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${PGR4_GENERATED_SHADER_BINARY_DIR}"
            COMMAND "${PGR4_DXC_EXECUTABLE}" -E "${ENTRY_POINT}" -T "${PROFILE}"
                    -Fo "${dxil_output}" "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}"
            COMMENT "DXC ${SOURCE_FILE}"
            VERBATIM)
        add_custom_command(
            OUTPUT "${header_output}"
            COMMAND "${CMAKE_COMMAND}"
                    "-DINPUT_FILE=${dxil_output}" "-DOUTPUT_FILE=${header_output}"
                    "-DARRAY_NAME=${ARRAY_NAME}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinaryHeader.cmake"
            DEPENDS "${dxil_output}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedBinaryHeader.cmake"
            COMMENT "Embedding ${shader_name}.dxil"
            VERBATIM)
        set(${OUT_VAR} ${${OUT_VAR}} "${header_output}" PARENT_SCOPE)
    endfunction()

    set(PGR4_BLIT_SHADER_HEADERS)
    pgr4_compile_dxil_shader(PGR4_BLIT_SHADER_HEADERS
        src/render/shaders/copy_vs.hlsl vs_6_0 main g_copy_vs_dxil render/shaders copy_vs.hlsl)
    pgr4_compile_dxil_shader(PGR4_BLIT_SHADER_HEADERS
        src/render/shaders/copy_ps.hlsl ps_6_0 main g_copy_ps_dxil render/shaders copy_ps.hlsl)
    add_custom_target(pgr4_blit_shaders DEPENDS ${PGR4_BLIT_SHADER_HEADERS})
    add_dependencies(pgr4_recompiled pgr4_blit_shaders)
    target_sources(pgr4_recompiled PRIVATE ${PGR4_BLIT_SHADER_HEADERS})

    # dxcompiler.dll / dxil.dll must sit next to the exe for the runtime
    # spec-constant link, and must be the same build as dxc.exe above.
    foreach(dxc_runtime_dll dxcompiler.dll dxil.dll)
        if(EXISTS "${PGR4_DXC_DIR}/bin/x64/${dxc_runtime_dll}")
            add_custom_command(TARGET pgr4_recompiled POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${PGR4_DXC_DIR}/bin/x64/${dxc_runtime_dll}"
                        "$<TARGET_FILE_DIR:pgr4_recompiled>/${dxc_runtime_dll}"
                VERBATIM)
        endif()
    endforeach()

    message(STATUS "PGR4: native Plume renderer ENABLED (xenos GPU plugin bypassed)")
else()
    message(STATUS "PGR4: native Plume renderer disabled; using the xenos GPU plugin")
endif()
