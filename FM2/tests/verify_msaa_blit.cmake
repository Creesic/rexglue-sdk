execute_process(COMMAND "${DXC}" -dumpbin "${SHADER}"
    RESULT_VARIABLE result OUTPUT_VARIABLE disassembly ERROR_VARIABLE errors)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "MSAA blit DXIL validation failed: ${errors}")
endif()
# Ensure the shipped shader retains an MSAA resource, per-sample loads,
# runtime sample-count query and averaging, not an ordinary Texture2D sample.
foreach(required "2dMS" "dx.op.textureLoad" "dx.op.getDimensions" "fdiv")
    string(FIND "${disassembly}" "${required}" found)
    if(found EQUAL -1)
        message(FATAL_ERROR "MSAA blit missing ${required}")
    endif()
endforeach()
if(disassembly MATCHES "dx.op.sampleLevel")
    message(FATAL_ERROR "MSAA blit incorrectly uses single-sample sampling")
endif()
