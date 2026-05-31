
# fh1 - ReXGlue Recompiled Project
#
# This file is yours to edit. 'rexglue migrate' will NOT overwrite it.
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(fh1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

include(generated/rexglue.cmake)

# Anchor-based codegen patcher (see patcher/Paths-fixes.md).
add_executable(fh1-patcher
    patcher/patcher.cpp
    patcher/patcher_main.cpp
)
target_include_directories(fh1-patcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/patcher
    ${REXSDK_DIR}/include
)
target_link_libraries(fh1-patcher PRIVATE rex::core fmt::fmt)

add_custom_command(TARGET fh1_codegen POST_BUILD
    COMMAND $<TARGET_FILE:fh1-patcher> ${CMAKE_CURRENT_SOURCE_DIR}/generated
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Applying FH1 anchor patches to generated sources"
    VERBATIM
)
add_dependencies(fh1_codegen fh1-patcher)

# Sources
set(FH1_SOURCES
    src/main.cpp
    src/roundevenf_compat.cpp
    src/fh1_guest_pc_fiber.cpp
    src/fh1_loader_epoch.cpp
    src/fh1_track_loader_alloc.cpp
    src/fh1_workqueue_dispatch.cpp
    src/fh1_world_load_guards.cpp
    src/fh1_thread_start_guard.cpp
)

if(WIN32)
    add_executable(fh1 WIN32 ${FH1_SOURCES})
else()
    add_executable(fh1 ${FH1_SOURCES})
endif()

rexglue_setup_target(fh1)

add_dependencies(fh1 fh1_codegen)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        target_compile_options(fh1 PRIVATE -mfma)
    endif()
elseif(MSVC)
    target_compile_options(fh1 PRIVATE /arch:AVX2)
endif()

if(WIN32)
    target_link_options(fh1 PRIVATE /INCREMENTAL:NO)
endif()
