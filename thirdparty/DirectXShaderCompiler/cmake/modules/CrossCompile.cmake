function(llvm_create_cross_target_internal target_name toochain buildtype)
  set(LLVM_CROSS_NATIVE_CMAKE_ARGS
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5
      -DCMAKE_MACOSX_BUNDLE=OFF
      -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON
      -DLLVM_INCLUDE_TOOLS=OFF
      -DLLVM_BUILD_TOOLS=OFF
      -DLLVM_INCLUDE_TESTS=OFF
      -DLLVM_INCLUDE_EXAMPLES=OFF
      -DLLVM_INCLUDE_DOCS=OFF)

  if(APPLE)
    set(LLVM_CROSS_NATIVE_DEPLOYMENT_TARGET "15.0" CACHE STRING
        "macOS deployment target for native tools used while cross-compiling")
    list(APPEND LLVM_CROSS_NATIVE_CMAKE_ARGS
         -DCMAKE_OSX_SYSROOT=macosx
         "-DCMAKE_OSX_DEPLOYMENT_TARGET=${LLVM_CROSS_NATIVE_DEPLOYMENT_TARGET}")
  endif()

  foreach(_dxc_header_var D3D12_INCLUDE_DIR DXGI_INCLUDE_DIR WSL_INCLUDE_DIR)
    if(DEFINED ${_dxc_header_var})
      list(APPEND LLVM_CROSS_NATIVE_CMAKE_ARGS
           "-D${_dxc_header_var}=${${_dxc_header_var}}")
    endif()
  endforeach()

  if(NOT DEFINED LLVM_${target_name}_BUILD)
    set(LLVM_${target_name}_BUILD "${CMAKE_BINARY_DIR}/${target_name}")
    set(LLVM_${target_name}_BUILD ${LLVM_${target_name}_BUILD} PARENT_SCOPE)
    message(STATUS "Setting native build dir to " ${LLVM_${target_name}_BUILD})
  endif(NOT DEFINED LLVM_${target_name}_BUILD)

  if (EXISTS ${LLVM_MAIN_SRC_DIR}/cmake/platforms/${toolchain}.cmake)
    set(CROSS_TOOLCHAIN_FLAGS_${target_name} 
        -DCMAKE_TOOLCHAIN_FILE=\"${LLVM_MAIN_SRC_DIR}/cmake/platforms/${toolchain}.cmake\"
        CACHE STRING "Toolchain file for ${target_name}")
  endif()

  add_custom_command(OUTPUT ${LLVM_${target_name}_BUILD}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${LLVM_${target_name}_BUILD}
    COMMENT "Creating ${LLVM_${target_name}_BUILD}...")

  add_custom_command(OUTPUT ${LLVM_${target_name}_BUILD}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
        ${LLVM_CROSS_NATIVE_CMAKE_ARGS}
        ${CROSS_TOOLCHAIN_FLAGS_${target_name}} ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${LLVM_${target_name}_BUILD}
    DEPENDS ${LLVM_${target_name}_BUILD}
    COMMENT "Configuring ${target_name} LLVM...")

  add_custom_target(CONFIGURE_LLVM_${target_name}
                    DEPENDS ${LLVM_${target_name}_BUILD}/CMakeCache.txt)

  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
                                      ${LLVM_${target_name}_BUILD})

  if(NOT IS_DIRECTORY ${LLVM_${target_name}_BUILD})
    

    message(STATUS "Configuring ${target_name} build...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
      ${LLVM_${target_name}_BUILD} )

    message(STATUS "Configuring ${target_name} targets...")
    if (buildtype)
      set(build_type_flags "-DCMAKE_BUILD_TYPE=${buildtype}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} ${build_type_flags}
        -G "${CMAKE_GENERATOR}" -DLLVM_TARGETS_TO_BUILD=${LLVM_TARGETS_TO_BUILD}
        ${LLVM_CROSS_NATIVE_CMAKE_ARGS}
        ${CROSS_TOOLCHAIN_FLAGS_${target_name}} ${CMAKE_SOURCE_DIR}
      WORKING_DIRECTORY ${LLVM_${target_name}_BUILD} )
  endif(NOT IS_DIRECTORY ${LLVM_${target_name}_BUILD})

endfunction()

function(llvm_create_cross_target target_name sysroot)
  llvm_create_cross_target_internal(${target_name} ${sysroot} ${CMAKE_BUILD_TYPE})
endfunction()

llvm_create_cross_target_internal(NATIVE "" Release)
