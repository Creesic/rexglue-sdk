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
)

if(PGR4_ENABLE_PLUME)
    list(APPEND PGR4RECOMPILED_SOURCES
        src/render/video.cpp
        src/render/d3d_hooks.cpp
        src/render/guest_gpu.cpp
    )
endif()

if(WIN32)
    add_executable(pgr4_recompiled WIN32 ${PGR4RECOMPILED_SOURCES})
else()
    add_executable(pgr4_recompiled ${PGR4RECOMPILED_SOURCES})
endif()

rexglue_setup_target(pgr4_recompiled)

if(PGR4_ENABLE_PLUME)
    set(PGR4_PLUME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/plume")

    # Checking only for plume/CMakeLists.txt is not enough: plume carries its
    # own submodules, and `git submodule add` does not recurse into them. That
    # file is present while contrib/ is still empty, and the failure surfaces
    # later as "Cannot find source file" / "No SOURCES given to target: plume".
    # Check the nested sources plume's own add_library() consumes instead.
    set(PGR4_PLUME_REQUIRED_SOURCES
        "${PGR4_PLUME_DIR}/CMakeLists.txt"
        "${PGR4_PLUME_DIR}/contrib/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp"
        "${PGR4_PLUME_DIR}/contrib/volk/volk.c"
        "${PGR4_PLUME_DIR}/contrib/VulkanMemoryAllocator/include/vk_mem_alloc.h"
        "${PGR4_PLUME_DIR}/contrib/Vulkan-Headers/include/vulkan/vulkan.h"
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
            "PGR4_ENABLE_PLUME is ON but plume is not fully checked out. Missing:\n"
            "    ${PGR4_PLUME_MISSING_TEXT}\n\n"
            "plume has nested submodules (volk, VulkanMemoryAllocator, Vulkan-Headers, "
            "D3D12MemoryAllocator). Run:\n"
            "    git submodule update --init --recursive thirdparty/plume")
    endif()

    set(PLUME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    if(NOT TARGET plume)
        add_subdirectory("${PGR4_PLUME_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/plume")
    endif()

    # plume builds its D3D12 backend but leaves the system import libraries to
    # the consumer, so D3D12CreateDevice / D3D12SerializeRootSignature are
    # unresolved without these. Matches what FM2 links.
    target_link_libraries(pgr4_recompiled PRIVATE plume)
    if(WIN32)
        target_link_libraries(pgr4_recompiled PRIVATE d3d12 dxgi)
    endif()
    target_include_directories(pgr4_recompiled PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${PGR4_PLUME_DIR}"
    )
    target_compile_definitions(pgr4_recompiled PRIVATE PGR4_ENABLE_PLUME=1)

    message(STATUS "PGR4: native Plume renderer ENABLED (xenos GPU plugin bypassed)")
else()
    message(STATUS "PGR4: native Plume renderer disabled; using the xenos GPU plugin")
endif()
