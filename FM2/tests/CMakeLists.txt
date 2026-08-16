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

add_executable(fm2_render_snapshot_test render_snapshot_test.cpp)
target_include_directories(fm2_render_snapshot_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
add_test(NAME fm2_render_snapshot_test COMMAND fm2_render_snapshot_test)
