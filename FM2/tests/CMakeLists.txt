# FM2 tests - standalone validators (no Catch2/REXGLUE_BUILD_TESTS dependency,
# to avoid pulling in the core SDK's full PPC-toolchain test suite for a
# single generated-data check).

add_executable(fm2_shader_cache_test shader_cache_test.cpp)
target_sources(fm2_shader_cache_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../generated/shader_cache.cpp
)
target_include_directories(fm2_shader_cache_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
add_test(NAME fm2_shader_cache_test COMMAND fm2_shader_cache_test)
if(FM2_DXC_EXECUTABLE)
    add_test(NAME fm2_msaa_blit_test COMMAND "${CMAKE_COMMAND}"
        "-DDXC=${FM2_DXC_EXECUTABLE}"
        "-DSHADER=${FM2_GENERATED_SHADER_BINARY_DIR}/copy_msaa_ps.hlsl.dxil"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_msaa_blit.cmake")
endif()

add_executable(fm2_render_snapshot_test render_snapshot_test.cpp)
target_include_directories(fm2_render_snapshot_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/plume
)
add_test(NAME fm2_render_snapshot_test COMMAND fm2_render_snapshot_test)

# Exercise the real queue/recording ownership without a GPU or guest runtime.
add_executable(fm2_render_queue_test render_queue_test.cpp ../src/render/render_queue.cpp)
target_include_directories(fm2_render_queue_test PRIVATE
    ../src ../../include ../../thirdparty/plume)
target_link_libraries(fm2_render_queue_test PRIVATE spdlog::spdlog)
target_compile_definitions(fm2_render_queue_test PRIVATE NOMINMAX)
if(MSVC)
    target_compile_options(fm2_render_queue_test PRIVATE /EHsc)
endif()
add_test(NAME fm2_render_queue_test COMMAND fm2_render_queue_test)
