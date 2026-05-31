# rexaudio - Audio subsystem library
# Audio processing and XMA decoding for Xbox 360 emulation

add_library(rexaudio OBJECT
    
    audio_driver.cpp
    audio_system.cpp
    fm2_native/codec.cpp
    fm2_native/diag.cpp
    fm2_native/runtime.cpp
    fm2_native/scheduler.cpp
    xma_context.cpp
    xma_decoder.cpp
    xma_register_file.cpp
    # NOP backend (always available)
    nop/nop_audio_system.cpp
    # SDL backend
    sdl/sdl_audio_system.cpp
    sdl/sdl_audio_driver.cpp
)

if (WIN32)
    target_sources(rexaudio PRIVATE
        xaudio2/xaudio2_audio_system.cpp
        xaudio2/xaudio2_audio_driver.cpp
    )
endif()
add_library(rex::audio ALIAS rexaudio)

target_include_directories(rexaudio PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(rexaudio
    PUBLIC rexcore SDL3::SDL3
    PRIVATE libavcodec libavutil
)

if (WIN32)
    target_link_libraries(rexaudio PRIVATE ole32)
endif()
