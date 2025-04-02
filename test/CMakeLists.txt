add_subdirectory(fixtures)

list(APPEND tests
  basic
)

foreach(test IN LISTS tests)
  add_executable(${test} ${test}.c)

  target_link_libraries(
    ${test}
    PRIVATE
      daemon_static
  )

  add_dependencies(${test} sleep)

  target_compile_definitions(
    ${test}
    PRIVATE
      "FIXTURE_SLEEP=\"$<TARGET_FILE:sleep>\""
  )

  add_test(
    NAME ${test}
    COMMAND ${test}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  set_tests_properties(
    ${test}
    PROPERTIES
    TIMEOUT 30
  )
endforeach()
