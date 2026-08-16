# fm2 - ReXGlue Recompiled Project
#
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
    src/render/video.cpp
    src/render/render_queue.cpp
    src/render/d3d_hooks.cpp
    src/render/d3d_resource_hooks.cpp
    src/render/pipeline.cpp
    src/render/render_state.cpp
    generated/shader_cache.cpp
)

if(WIN32)
    add_executable(fm2 WIN32 ${FM2_SOURCES})
else()
    add_executable(fm2 ${FM2_SOURCES})
endif()

rexglue_setup_target(fm2)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/plume/CMakeLists.txt")
    set(PLUME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    if(NOT TARGET plume)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/plume" "${CMAKE_CURRENT_BINARY_DIR}/plume")
    endif()

    # Use the same pinned DXC build XenosRecomp itself vendors (thirdparty/dxc-bin,
    # renderbag/dxc-bin), not whatever DXC happens to be installed with the local
    # Windows SDK. DXC embeds a compiler-version tag in every DXIL library it
    # produces and its linker refuses to link libraries built by different DXC
    # versions -- a runtime-loaded system DXC that doesn't match the DXC XenosRecomp
    # used to compile the shader cache means every spec-constant-specialized shader
    # (via IDxcLinker::Link in pipeline.cpp) fails to link, silently and completely
    # breaking rendering, since XenosRecomp's shader cache and this renderer's own
    # runtime spec-constant-library compile+link step must produce mutually
    # link-compatible DXIL modules.
    set(FM2_DXC_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/dxc-bin/bin/x64/dxc.exe")
    if(NOT EXISTS "${FM2_DXC_EXECUTABLE}")
        set(FM2_DXC_EXECUTABLE "")
    endif()

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
            src/render/shaders/placeholder_ps.hlsl ps_6_0 main g_placeholder_ps_dxil
            render/shaders placeholder_ps.hlsl)
        add_custom_target(fm2_blit_shaders DEPENDS ${FM2_BLIT_SHADER_HEADERS})
        add_dependencies(fm2 fm2_blit_shaders)
        target_sources(fm2 PRIVATE ${FM2_BLIT_SHADER_HEADERS})
        target_include_directories(fm2 BEFORE PRIVATE
            "${FM2_GENERATED_SHADER_INCLUDE_DIR}")

        # dxcompiler.dll/dxil.dll live alongside dxc.exe in thirdparty/dxc-bin;
        # the runtime LoadLibraryW for DXC needs them reachable next to
        # fm2.exe, not just on the dev-shell PATH -- and it's essential this
        # is the SAME dxcompiler.dll as dxc.exe above, not a system one (see
        # the version-mismatch comment on FM2_DXC_EXECUTABLE).
        get_filename_component(FM2_DXC_DIR "${FM2_DXC_EXECUTABLE}" DIRECTORY)
        foreach(dxc_runtime_dll dxcompiler.dll dxil.dll)
            if(EXISTS "${FM2_DXC_DIR}/${dxc_runtime_dll}")
                add_custom_command(TARGET fm2 POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${FM2_DXC_DIR}/${dxc_runtime_dll}"
                        "$<TARGET_FILE_DIR:fm2>/${dxc_runtime_dll}"
                    COMMENT "Staging ${dxc_runtime_dll} next to fm2")
            endif()
        endforeach()
    else()
        message(WARNING "DXC not found; FM2 native renderer blit shaders will be missing")
    endif()

    target_link_libraries(fm2 PRIVATE plume d3d12 dxgi)
    if(TARGET libzstd_static)
        target_link_libraries(fm2 PRIVATE libzstd_static)
        target_include_directories(fm2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/zstd/lib")
    endif()
    target_include_directories(fm2 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/xxHash")
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=1 NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_definitions(fm2 PRIVATE FM2_HAS_PLUME=0)
endif()

enable_testing()
add_subdirectory(tests)
