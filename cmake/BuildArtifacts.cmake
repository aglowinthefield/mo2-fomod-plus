# Run inside CMake's resolved configure-preset environment without configuring again.
# The outer command uses --preset ... -N -D... -C <this file>.
if(NOT DEFINED FOMOD_BUILD_DIRECTORY OR FOMOD_BUILD_DIRECTORY STREQUAL ""
    OR NOT DEFINED FOMOD_BUILD_RESULT OR FOMOD_BUILD_RESULT STREQUAL "")
  message(FATAL_ERROR "Missing resolved build directory")
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${FOMOD_BUILD_DIRECTORY}" --target fomod_artifacts
  RESULT_VARIABLE _result)
# In view mode, even an initial-cache FATAL_ERROR can leave CMake's exit code at zero.
# The caller must check this explicit result, never just the outer process exit code.
file(WRITE "${FOMOD_BUILD_RESULT}" "${_result}")
