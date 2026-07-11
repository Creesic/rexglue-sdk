# rexaudio - Audio subsystem library
# Audio processing and XMA decoding for Xbox 360 emulation

add_library(rexaudio OBJECT
    
    audio_driver.cpp
    audio_system.cpp
    xma_context.cpp
    xma_decoder.cpp
    xma_register_file.cpp
    # NOP backend (always available)
    nop/nop_audio_system.cpp
    # SDL backend
    sdl/sdl_audio_system.cpp
    sdl/sdl_audio_driver.cpp
)
add_library(rex::audio ALIAS rexaudio)

target_include_directories(rexaudio PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(rexaudio
    PUBLIC rexcore SDL3::SDL3
    PRIVATE libavcodec libavutil
)

if(WIN32)
    add_library(rexaudio-xaudio2 STATIC
        xaudio2/xaudio2_audio_system.cpp
        xaudio2/xaudio2_audio_driver.cpp
    )
    add_library(rex::audio-xaudio2 ALIAS rexaudio-xaudio2)
    set_target_properties(rexaudio-xaudio2 PROPERTIES EXCLUDE_FROM_ALL ON)

    target_include_directories(rexaudio-xaudio2 PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_link_libraries(rexaudio-xaudio2 PUBLIC rex::audio PRIVATE ole32)
endif()
