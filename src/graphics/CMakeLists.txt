# rexgraphics: GPU emulation core (ported from Xenia)
# Namespace: rex::graphics

# Core GPU types - minimal dependencies, fully ported
set(REXGRAPHICS_CORE_SOURCES
    flags.cpp
    xenos.cpp
    pipeline/texture/info.cpp
    pipeline/texture/info_formats.cpp
    register_file.cpp
    registers.cpp
    pipeline/shader/shader.cpp
    format/ucode.cpp
    sampler_info.cpp
)

# GPU system files - full backend
set(REXGRAPHICS_SYSTEM_SOURCES
    graphics_system.cpp
    command_processor.cpp
    trace_writer.cpp
    pipeline/texture/extent.cpp
    pipeline/texture/util.cpp
    util/draw_extent_estimator.cpp
    util/draw.cpp
    packet_disassembler.cpp
    primitive_processor.cpp
    pipeline/render_target/cache.cpp
    shared_memory.cpp
    pipeline/texture/cache.cpp
    pipeline/texture/conversion.cpp
    pipeline/shader/interpreter.cpp
    pipeline/shader/translator.cpp
    pipeline/shader/translator_disasm.cpp
)

# SPIR-V shader translation
set(REXGRAPHICS_SPIRV_SOURCES
    pipeline/shader/spirv_builder.cpp
    pipeline/shader/spirv.cpp
    pipeline/shader/spirv_translator.cpp
    pipeline/shader/spirv_translator_alu.cpp
    pipeline/shader/spirv_translator_fetch.cpp
    pipeline/shader/spirv_translator_memexport.cpp
    pipeline/shader/spirv_translator_rb.cpp
)

# Vulkan backend
set(REXGRAPHICS_VULKAN_SOURCES
    vulkan/graphics_system.cpp
    vulkan/deferred_command_buffer.cpp
    vulkan/command_processor.cpp
    vulkan/pipeline_cache.cpp
    vulkan/primitive_processor.cpp
    vulkan/render_target_cache.cpp
    vulkan/shader.cpp
    vulkan/shared_memory.cpp
    vulkan/texture_cache.cpp
)

# DXBC shader translation (required for D3D12 and Metal)
if(REXGLUE_USE_D3D12 OR REXGLUE_USE_METAL)
    # DXBC third-party dependency
    set(REXGRAPHICS_DXBC_THIRDPARTY_SOURCES
        ${REXGLUE_ROOT}/thirdparty/dxbc/DXBCChecksum.cpp
    )

    set(REXGRAPHICS_DXBC_SOURCES
        dxbc_geometry_shader.cpp
        pipeline/shader/dxbc.cpp
        pipeline/shader/dxbc_translator.cpp
        pipeline/shader/dxbc_translator_alu.cpp
        pipeline/shader/dxbc_translator_fetch.cpp
        pipeline/shader/dxbc_translator_memexport.cpp
        pipeline/shader/dxbc_translator_om.cpp
    )
endif()

if(REXGLUE_USE_D3D12)
    # D3D12 GPU backend
    set(REXGRAPHICS_D3D12_SOURCES
        d3d12/graphics_system.cpp
        d3d12/command_processor.cpp
        d3d12/primitive_processor.cpp
        d3d12/render_target_cache.cpp
        d3d12/shader.cpp
        d3d12/shared_memory.cpp
        d3d12/texture_cache.cpp
        d3d12/deferred_command_list.cpp
        d3d12/pipeline_cache.cpp
    )
endif()

# Metal backend
if(REXGLUE_USE_METAL)
    set(REXGRAPHICS_METAL_SOURCES
        metal/dxbc_to_dxil_converter.cc
        metal/ir_runtime_impl.mm
        metal/metal_backend_telemetry.cc
        metal/metal_command_processor.cc
        metal/metal_direct_host_resolve.cc
        metal/metal_geometry_shader.cc
        metal/metal_graphics_system.cc
        metal/metal_heap_pool.cc
        metal/metal_pipeline_cache.cc
        metal/metal_primitive_processor.cc
        metal/metal_render_target_cache.cc
        metal/metal_shader.cc
        metal/metal_shader_cache.cc
        metal/metal_shader_converter.cc
        metal/metal_shared_memory.cc
        metal/metal_stage_compile_cache.cc
        metal/metal_texture_cache.cc
        metal/metal_upload_buffer_pool.cc
        metal/metal_zpd_visibility_pool.cc
    )
endif()

# Combine sources
set(REXGRAPHICS_SOURCES
    ${REXGRAPHICS_CORE_SOURCES}
    ${REXGRAPHICS_SYSTEM_SOURCES}
)

if(REXGLUE_USE_VULKAN)
    list(APPEND REXGRAPHICS_SOURCES ${REXGRAPHICS_SPIRV_SOURCES} ${REXGRAPHICS_VULKAN_SOURCES})
endif()

if(REXGLUE_USE_D3D12 OR REXGLUE_USE_METAL)
    list(APPEND REXGRAPHICS_SOURCES ${REXGRAPHICS_DXBC_THIRDPARTY_SOURCES} ${REXGRAPHICS_DXBC_SOURCES})
endif()

if(REXGLUE_USE_D3D12)
    list(APPEND REXGRAPHICS_SOURCES ${REXGRAPHICS_D3D12_SOURCES})
endif()

if(REXGLUE_USE_METAL)
    list(APPEND REXGRAPHICS_SOURCES ${REXGRAPHICS_METAL_SOURCES})
endif()

add_library(rexgraphics OBJECT ${REXGRAPHICS_SOURCES})
add_library(rex::graphics ALIAS rexgraphics)

target_include_directories(rexgraphics PUBLIC
    $<BUILD_INTERFACE:${REXGLUE_ROOT}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(rexgraphics PRIVATE
    ${REXGLUE_ROOT}/thirdparty/renderdoc
    ${REXGLUE_ROOT}  # For thirdparty/dxbc/DXBCChecksum.h include
)

target_compile_options(rexgraphics PRIVATE -Wno-unused-variable)

target_link_libraries(rexgraphics PUBLIC
    rexcore
    rexui
    xxHash::xxhash
    snappy
)

if(REXGLUE_USE_VULKAN)
    target_link_libraries(rexgraphics PUBLIC
        volk::volk
        glslang::SPIRV
        GPUOpen::VulkanMemoryAllocator
    )
    target_link_libraries(rexgraphics PRIVATE
        glslang
        MachineIndependent
        GenericCodeGen
        OSDependent
        OGLCompiler
    )
    target_compile_definitions(rexgraphics PUBLIC REX_HAS_VULKAN=1)
endif()

if(REXGLUE_USE_D3D12)
    target_link_libraries(rexgraphics PUBLIC
        d3d12
        dxgi
        dxguid
    )
    target_compile_definitions(rexgraphics PUBLIC REX_HAS_D3D12=1)
endif()

if(REXGLUE_USE_METAL)
    target_link_libraries(rexgraphics PUBLIC
        metal-cpp
        rex-thirdparty-dxilconv
        rex-thirdparty-metal-shader-converter
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
    )
    add_dependencies(rexgraphics
        rex-thirdparty-dxilconv-stage
        rex-thirdparty-metal-shader-converter-stage)
    target_compile_definitions(rexgraphics PUBLIC REX_HAS_METAL=1 IR_RUNTIME_METALCPP)
    target_include_directories(rexgraphics PRIVATE "${PROJECT_BINARY_DIR}/generated")

    rex_shader_rules_metal(rexgraphics
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "rex/graphics/shaders")

    set(_metal_shader_args)
    if(CMAKE_OSX_DEPLOYMENT_TARGET)
        list(APPEND _metal_shader_args
            --metal-min-version-flag
            "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
    if(REXGLUE_METAL_SHADER_DEBUG_INFO)
        list(APPEND _metal_shader_args --metal-debug)
    endif()

    set(_direct_host_resolve_generated_root "${PROJECT_BINARY_DIR}/generated")
    set(_direct_host_resolve_bytecode_dir
        "${_direct_host_resolve_generated_root}/rex/graphics/shaders/bytecode/metal")
    set(_direct_host_resolve_outputs)
    file(MAKE_DIRECTORY "${_direct_host_resolve_bytecode_dir}")

    macro(_rex_metal_direct_host_resolve_variant id input)
        set(_variant_define_args)
        foreach(_define ${ARGN})
            list(APPEND _variant_define_args --define "${_define}")
        endforeach()
        set(_variant_out "${_direct_host_resolve_bytecode_dir}/${id}.h")
        set(_variant_dep "${_variant_out}.d")
        list(APPEND _direct_host_resolve_outputs "${_variant_out}")
        add_custom_command(
            OUTPUT "${_variant_out}"
            COMMAND $<TARGET_FILE:rex-metal-shader-cc>
                    ${_metal_shader_args}
                    --identifier "${id}"
                    ${_variant_define_args}
                    --depfile "${_variant_dep}"
                    "${input}"
                    "${_variant_out}"
            DEPENDS "${input}" rex-metal-shader-cc
            DEPFILE "${_variant_dep}"
            COMMENT "Metal: ${id}"
            VERBATIM
        )
    endmacro()

    set(_resolve_host_color_entry
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/resolve_host_color_entry.xesli")
    set(_resolve_host_color_full_entry
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/resolve_host_color_full_entry.xesli")
    set(_resolve_host_depth_entry
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/resolve_host_depth_entry.xesli")

    foreach(_source_uint 0 1)
        foreach(_bpp 32 64)
            foreach(_msaa 1 2 4)
                foreach(_scaled 0 1)
                    set(_id "resolve_host_color")
                    if(_source_uint)
                        string(APPEND _id "_uint")
                    endif()
                    string(APPEND _id "_${_bpp}bpp_${_msaa}xmsaa")
                    set(_defines
                        "XE_RESOLVE_HOST_COLOR_BPP=${_bpp}"
                        "XE_RESOLVE_HOST_COLOR_MSAA_SAMPLES=${_msaa}"
                        "XE_RESOLVE_HOST_COLOR_SOURCE_UINT=${_source_uint}")
                    if(_scaled)
                        string(APPEND _id "_scaled")
                        list(APPEND _defines "XE_RESOLVE_RESOLUTION_SCALED=1")
                    endif()
                    string(APPEND _id "_cs")
                    _rex_metal_direct_host_resolve_variant(
                        "${_id}" "${_resolve_host_color_entry}" ${_defines})
                endforeach()
            endforeach()
        endforeach()

        foreach(_bpp 8 16 32 64 128)
            foreach(_msaa 1 2 4)
                foreach(_scaled 0 1)
                    set(_id "resolve_host_color_full")
                    if(_source_uint)
                        string(APPEND _id "_uint")
                    endif()
                    string(APPEND _id "_${_bpp}bpp_${_msaa}xmsaa")
                    set(_defines
                        "XE_RESOLVE_HOST_COLOR_FULL_DEST_BPP=${_bpp}"
                        "XE_RESOLVE_HOST_COLOR_MSAA_SAMPLES=${_msaa}"
                        "XE_RESOLVE_HOST_COLOR_SOURCE_UINT=${_source_uint}")
                    if(_scaled)
                        string(APPEND _id "_scaled")
                        list(APPEND _defines "XE_RESOLVE_RESOLUTION_SCALED=1")
                    endif()
                    string(APPEND _id "_cs")
                    _rex_metal_direct_host_resolve_variant(
                        "${_id}" "${_resolve_host_color_full_entry}" ${_defines})
                endforeach()
            endforeach()
        endforeach()
    endforeach()

    foreach(_msaa 1 2 4)
        foreach(_scaled 0 1)
            set(_id "resolve_host_depth_32bpp_${_msaa}xmsaa")
            set(_defines "XE_RESOLVE_HOST_DEPTH_MSAA_SAMPLES=${_msaa}")
            if(_scaled)
                string(APPEND _id "_scaled")
                list(APPEND _defines "XE_RESOLVE_RESOLUTION_SCALED=1")
            endif()
            string(APPEND _id "_cs")
            _rex_metal_direct_host_resolve_variant(
                "${_id}" "${_resolve_host_depth_entry}" ${_defines})
        endforeach()
    endforeach()

    add_custom_target(rexgraphics-metal-direct-host-resolve-shaders
        DEPENDS ${_direct_host_resolve_outputs})
    add_dependencies(rexgraphics rexgraphics-metal-direct-host-resolve-shaders)

    set(_texture_upload_repack_shader
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/texture_upload_repack.metal")
    set(_texture_upload_repack_out
        "${_direct_host_resolve_bytecode_dir}/texture_upload_repack.h")
    set(_texture_upload_repack_dep "${_texture_upload_repack_out}.d")
    add_custom_command(
        OUTPUT "${_texture_upload_repack_out}"
        COMMAND $<TARGET_FILE:rex-metal-shader-cc>
                ${_metal_shader_args}
                --depfile "${_texture_upload_repack_dep}"
                "${_texture_upload_repack_shader}"
                "${_texture_upload_repack_out}"
        DEPENDS "${_texture_upload_repack_shader}" rex-metal-shader-cc
        DEPFILE "${_texture_upload_repack_dep}"
        COMMENT "Metal: texture_upload_repack.metal"
        VERBATIM
    )
    add_custom_target(rexgraphics-metal-texture-upload-repack-shader
        DEPENDS "${_texture_upload_repack_out}")
    add_dependencies(rexgraphics rexgraphics-metal-texture-upload-repack-shader)
    set_source_files_properties("${_texture_upload_repack_shader}"
        PROPERTIES HEADER_FILE_ONLY TRUE)
    target_sources(rexgraphics PRIVATE "${_texture_upload_repack_shader}")
endif()
