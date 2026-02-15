if(NOT DEFINED BUILD_DIR)
    message(FATAL_ERROR "BUILD_DIR is required (path to build directory).")
endif()
if(NOT DEFINED OUT_FILE)
    message(FATAL_ERROR "OUT_FILE is required (path to artefacts JSON).")
endif()

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." REALPATH)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${REPO_ROOT}" -B "${BUILD_DIR}"
            -DTHREADBARE_ARTEFACTS_OUT="${OUT_FILE}"
    RESULT_VARIABLE RESOLVE_RESULT
)

if(NOT RESOLVE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to resolve artefacts (cmake configure failed).")
endif()

message(STATUS "Wrote artefacts to ${OUT_FILE}")
