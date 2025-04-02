cmake_minimum_required(VERSION 3.31)

project(daemon C)

add_library(daemon OBJECT)

set_target_properties(
  daemon
  PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  daemon
  INTERFACE
    include/daemon.h
    include/daemon/posix.h
    include/daemon/win32.h
)

if(WIN32)
  target_sources(
    daemon
    PRIVATE
      src/win32.c
  )
else()
  target_sources(
    daemon
    PRIVATE
      src/posix.c
  )
endif()

target_include_directories(
  daemon
  PUBLIC
    include
)

add_library(daemon_shared SHARED)

set_target_properties(
  daemon_shared
  PROPERTIES
  OUTPUT_NAME daemon
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  daemon_shared
  PUBLIC
    daemon
)

add_library(daemon_static STATIC)

set_target_properties(
  daemon_static
  PROPERTIES
  OUTPUT_NAME daemon
  PREFIX lib
)

target_link_libraries(
  daemon_static
  PUBLIC
    daemon
)

install(TARGETS daemon_shared daemon_static)

install(FILES include/daemon.h DESTINATION include)

install(DIRECTORY include/daemon DESTINATION include)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
endif()
