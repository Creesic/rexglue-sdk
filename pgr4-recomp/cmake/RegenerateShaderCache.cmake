set(candidate "${OUTPUT}.candidate")
file(REMOVE "${candidate}" "${candidate}.tmp")

execute_process(
    COMMAND "${XENOS_RECOMP}" "${INPUT_DIR}" "${candidate}" "${SHADER_COMMON}" --jobs 7
    RESULT_VARIABLE translate_result
    OUTPUT_VARIABLE translate_stdout
    ERROR_VARIABLE translate_stderr
)
if(NOT translate_result EQUAL 0)
    file(REMOVE "${candidate}" "${candidate}.tmp")
    message(FATAL_ERROR
        "XenosRecomp failed (${translate_result}); production cache was preserved\n"
        "${translate_stdout}\n${translate_stderr}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT_DIR=${INPUT_DIR}"
        "-DCACHE=${candidate}"
        -P "${VERIFY_SCRIPT}"
    RESULT_VARIABLE verify_result
    OUTPUT_VARIABLE verify_stdout
    ERROR_VARIABLE verify_stderr
)
if(NOT verify_result EQUAL 0)
    file(REMOVE "${candidate}" "${candidate}.tmp")
    message(FATAL_ERROR
        "Generated cache failed verification; production cache was preserved\n"
        "${verify_stdout}\n${verify_stderr}")
endif()

file(RENAME "${candidate}" "${OUTPUT}")
message(STATUS "${verify_stdout}")
