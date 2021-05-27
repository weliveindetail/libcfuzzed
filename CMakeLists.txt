cmake_minimum_required(VERSION 3.10)

project(libcfuzzed)

add_library(cfuzzed STATIC
  libcfuzzed.c
)

add_library(cfuzzed-preload SHARED
  libcfuzzed-preload.c
  src/fuzz_cwd.c
)

target_include_directories(cfuzzed-preload PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
)

target_compile_options(cfuzzed-preload PRIVATE
  -fPIC
)

install(
  TARGETS cfuzzed cfuzzed-preload
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

add_subdirectory(test)
