#==========================================================
# rexglue_configure_target() - Configure a consumer target
# with platform-specific settings and SDK source files.
#
# Usage:
#   rexglue_configure_target(<target>)
#
# Adds:
#   - Platform entry point source (windowed_app_main_*.cpp)
#   - ReXApp base class source (rex_app.cpp)
#   - Platform-specific link/compile settings
#==========================================================
macro(_rexglue_normalize_macos_vulkan_root candidate out_var)
    set(${out_var} "")
    if(NOT "${candidate}" STREQUAL "")
        foreach(_rexglue_candidate_root "${candidate}" "${candidate}/macOS")
            if(EXISTS "${_rexglue_candidate_root}/lib/libvulkan.1.dylib"
               OR EXISTS "${_rexglue_candidate_root}/Frameworks/vulkan.framework/vulkan"
               OR EXISTS "${_rexglue_candidate_root}/lib/libMoltenVK.dylib")
                set(${out_var} "${_rexglue_candidate_root}")
                break()
            endif()
        endforeach()
    endif()
endmacro()

macro(_rexglue_append_macos_vulkan_root roots_var candidate)
    _rexglue_normalize_macos_vulkan_root("${candidate}" _rexglue_normalized_vulkan_root)
    if(NOT "${_rexglue_normalized_vulkan_root}" STREQUAL "")
        list(FIND ${roots_var} "${_rexglue_normalized_vulkan_root}" _rexglue_vulkan_root_index)
        if(_rexglue_vulkan_root_index EQUAL -1)
            list(APPEND ${roots_var} "${_rexglue_normalized_vulkan_root}")
        endif()
    endif()
endmacro()

function(rexglue_find_macos_vulkan_runtime out_var)
    set(_rexglue_roots)

    if(DEFINED REXGLUE_VULKAN_RUNTIME_DIR AND NOT "${REXGLUE_VULKAN_RUNTIME_DIR}" STREQUAL "")
        _rexglue_append_macos_vulkan_root(_rexglue_roots "${REXGLUE_VULKAN_RUNTIME_DIR}")
    endif()

    if(DEFINED REXGLUE_SHARE_DIR AND EXISTS "${REXGLUE_SHARE_DIR}/vulkan")
        _rexglue_append_macos_vulkan_root(_rexglue_roots "${REXGLUE_SHARE_DIR}/vulkan")
    endif()

    if(DEFINED ENV{REX_VULKAN_SDK} AND NOT "$ENV{REX_VULKAN_SDK}" STREQUAL "")
        _rexglue_append_macos_vulkan_root(_rexglue_roots "$ENV{REX_VULKAN_SDK}")
    endif()

    if(DEFINED ENV{VULKAN_SDK} AND NOT "$ENV{VULKAN_SDK}" STREQUAL "")
        _rexglue_append_macos_vulkan_root(_rexglue_roots "$ENV{VULKAN_SDK}")
    endif()

    if(DEFINED ENV{HOME} AND IS_DIRECTORY "$ENV{HOME}/VulkanSDK")
        file(GLOB _rexglue_sdk_versions LIST_DIRECTORIES true "$ENV{HOME}/VulkanSDK/*")
        list(SORT _rexglue_sdk_versions COMPARE NATURAL ORDER DESCENDING)
        foreach(_rexglue_sdk_version IN LISTS _rexglue_sdk_versions)
            _rexglue_append_macos_vulkan_root(_rexglue_roots "${_rexglue_sdk_version}")
        endforeach()
    endif()

    _rexglue_append_macos_vulkan_root(_rexglue_roots "/usr/local")
    _rexglue_append_macos_vulkan_root(_rexglue_roots "/opt/homebrew")

    set(_rexglue_runtime_root "")
    if(_rexglue_roots)
        list(GET _rexglue_roots 0 _rexglue_runtime_root)
    endif()

    set(${out_var} "${_rexglue_runtime_root}" PARENT_SCOPE)
endfunction()

function(_rexglue_warn_missing_macos_vulkan_runtime)
    get_property(_rexglue_warning_emitted GLOBAL PROPERTY REXGLUE_MACOS_VULKAN_RUNTIME_WARNING_EMITTED)
    if(NOT _rexglue_warning_emitted)
        message(WARNING
            "No macOS Vulkan runtime was found. Install the LunarG Vulkan SDK or set "
            "REXGLUE_VULKAN_RUNTIME_DIR/REX_VULKAN_SDK to a Vulkan runtime root.")
        set_property(GLOBAL PROPERTY REXGLUE_MACOS_VULKAN_RUNTIME_WARNING_EMITTED TRUE)
    endif()
endfunction()

function(_rexglue_copy_macos_vulkan_runtime target_name runtime_root)
    if("${runtime_root}" STREQUAL "")
        _rexglue_warn_missing_macos_vulkan_runtime()
        return()
    endif()

    set(_rexglue_runtime_files)
    foreach(_rexglue_runtime_file
            lib/libvulkan.1.dylib
            lib/libvulkan.dylib
            lib/libMoltenVK.dylib
            lib/libSPIRV-Tools-shared.dylib
            share/vulkan/icd.d/MoltenVK_icd.json)
        if(EXISTS "${runtime_root}/${_rexglue_runtime_file}")
            list(APPEND _rexglue_runtime_files "${_rexglue_runtime_file}")
        endif()
    endforeach()

    if(NOT _rexglue_runtime_files)
        _rexglue_warn_missing_macos_vulkan_runtime()
        return()
    endif()

    set(_rexglue_runtime_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/vulkan/lib"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/vulkan/share/vulkan/icd.d"
    )
    foreach(_rexglue_runtime_file IN LISTS _rexglue_runtime_files)
        get_filename_component(_rexglue_runtime_dir "${_rexglue_runtime_file}" DIRECTORY)
        list(APPEND _rexglue_runtime_commands
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/vulkan/${_rexglue_runtime_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_root}/${_rexglue_runtime_file}"
                "$<TARGET_FILE_DIR:${target_name}>/vulkan/${_rexglue_runtime_dir}"
        )
    endforeach()

    add_custom_command(TARGET ${target_name} POST_BUILD
        ${_rexglue_runtime_commands}
        VERBATIM
    )
endfunction()

function(rexglue_configure_target target_name)
    # Platform entry point
    if(WIN32)
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_win.cpp)
    elseif(APPLE)
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_mac.mm)
    else()
        target_sources(${target_name} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_posix.cpp)
    endif()

    # ReXApp base class
    target_sources(${target_name} PRIVATE
        ${REXGLUE_SHARE_DIR}/rex_app.cpp)

    # Build config for version stamp (rex_app.cpp uses REXGLUE_BUILD_STAMP)
    target_compile_definitions(${target_name} PRIVATE
        REXGLUE_BUILD_CONFIG="$<CONFIG>")

    # Whole-archive linking for kernel hooks
    if(WIN32)
        target_link_options(${target_name} PRIVATE
            "LINKER:/WHOLEARCHIVE:$<TARGET_FILE:rex::kernel>"
        )
    elseif(APPLE)
        target_link_options(${target_name} PRIVATE
            "LINKER:-force_load,$<TARGET_FILE:rex::kernel>"
        )
    else()
        target_link_options(${target_name} PRIVATE
            -Wl,--whole-archive
            $<TARGET_FILE:rex::kernel>
            -Wl,--no-whole-archive
        )
    endif()

    # macOS platform settings
    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            target_compile_options(${target_name} PRIVATE -msse4.1)
        endif()
    endif()

    # Linux platform settings
    if(UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
        target_include_directories(${target_name} PRIVATE ${GTK3_INCLUDE_DIRS})
        target_link_libraries(${target_name} PRIVATE ${GTK3_LIBRARIES})
        # Large executable support
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            target_link_options(${target_name} PRIVATE -Wl,--no-relax)
            target_compile_options(${target_name} PRIVATE -mcmodel=large)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
            target_compile_options(${target_name} PRIVATE -march=armv8-a)
        endif()
    endif()

    if(NOT MSVC)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            target_compile_options(${target_name} PRIVATE -msse4.1)
        endif()
        # ARM64 NEON is enabled via -march=armv8-a above
    endif()

    # Copy runtime DLLs next to the executable
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:${target_name}>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:${target_name}>;$<TARGET_FILE_DIR:${target_name}>>"
            COMMAND_EXPAND_LISTS
        )
        # FidelityFX is linked PRIVATE by rexui (to avoid propagating DLL
        # requirements to tool-mode targets), so copy its DLLs explicitly.
        if(TARGET amd_fidelityfx_vk)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:amd_fidelityfx_vk>
                    $<TARGET_FILE_DIR:${target_name}>
            )
        endif()
        if(TARGET amd_fidelityfx_dx12)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:amd_fidelityfx_dx12>
                    $<TARGET_FILE_DIR:${target_name}>
            )
        endif()
    endif()

    if(APPLE AND REXGLUE_USE_VULKAN)
        rexglue_find_macos_vulkan_runtime(_rexglue_macos_vulkan_runtime_root)
        _rexglue_copy_macos_vulkan_runtime(${target_name} "${_rexglue_macos_vulkan_runtime_root}")
    endif()
endfunction()
