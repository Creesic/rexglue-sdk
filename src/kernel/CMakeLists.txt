set(REXKERNEL_SOURCES
    kernel_init.cpp

    # XAM subsystem - provides __imp__Xam* symbols
    xam/xam_module.cpp
    xam/xam_avatar.cpp
    xam/xam_content.cpp
    xam/xam_content_aggregate.cpp
    xam/xam_content_device.cpp
    xam/xam_debug.cpp
    xam/xam_enum.cpp
    xam/xam_info.cpp
    xam/xam_input.cpp
    xam/xam_locale.cpp
    xam/xam_msg.cpp
    xam/xam_net.cpp
    xam/xam_notify.cpp
    xam/xam_nui.cpp
    xam/xam_party.cpp
    xam/xam_task.cpp
    xam/xam_ui.cpp
    xam/xam_user.cpp
    xam/xam_video.cpp
    xam/xam_voice.cpp
    xam/xam_misc.cpp
    xam/apps/xam_app.cpp
    xam/apps/xgi_app.cpp
    xam/apps/xlivebase_app.cpp
    xam/apps/xmp_app.cpp

    # XBDM subsystem - provides __imp__Xbdm* symbols
    xbdm/xbdm_module.cpp
    xbdm/xbdm_misc.cpp

    # xboxkrnl exports - provides __imp__ symbols for generated code
    # xboxkrnl/cert_monitor.cpp    # TODO: Translate JIT methods for AOT
    # xboxkrnl/debug_monitor.cpp   # TODO: Translate JIT methods for AOT
    xboxkrnl/xboxkrnl_crypt.cpp
    xboxkrnl/xboxkrnl_debug.cpp
    xboxkrnl/xboxkrnl_error.cpp
    xboxkrnl/xboxkrnl_hal.cpp
    xboxkrnl/xboxkrnl_hid.cpp
    xboxkrnl/xboxkrnl_io.cpp
    xboxkrnl/xboxkrnl_io_info.cpp
    xboxkrnl/xboxkrnl_memory.cpp
    xboxkrnl/xboxkrnl_misc.cpp
    xboxkrnl/xboxkrnl_module.cpp
    xboxkrnl/xboxkrnl_modules.cpp
    xboxkrnl/xboxkrnl_ob.cpp
    xboxkrnl/xboxkrnl_rtl.cpp
    xboxkrnl/xboxkrnl_strings.cpp
    xboxkrnl/xboxkrnl_threading.cpp
    # xboxkrnl/xboxkrnl_usbcam.cpp  # TODO: lol eventually.
    xboxkrnl/xboxkrnl_video.cpp
    xboxkrnl/xboxkrnl_xconfig.cpp
    xboxkrnl/xboxkrnl_audio.cpp
    xboxkrnl/xboxkrnl_audio_xma.cpp

    crt/heap.cpp
    crt/file.cpp
    crt/memory.cpp
    crt/string.cpp
    crt/threading.cpp
)

list(TRANSFORM REXKERNEL_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
target_sources(rexruntime PRIVATE ${REXKERNEL_SOURCES})

target_link_libraries(rexruntime PRIVATE rexgraphics o1heap rexcore rexfilesystem rexui rexaudio rexinput imgui)

# Propagate graphics backend compile definitions to consumers.
# These are normally set by rexgraphics via PUBLIC compile definitions,
# but since we link it PRIVATE to avoid transitive export issues,
# we re-declare them here.
if(REXGLUE_USE_METAL)
    target_compile_definitions(rexruntime PUBLIC REX_HAS_METAL=1)
endif()
if(REXGLUE_USE_VULKAN)
    target_compile_definitions(rexruntime PUBLIC REX_HAS_VULKAN=1)
endif()
if(REXGLUE_USE_D3D12)
    target_compile_definitions(rexruntime PUBLIC REX_HAS_D3D12=1)
endif()
