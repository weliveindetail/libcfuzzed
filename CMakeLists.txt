cmake_minimum_required(VERSION 3.10)

project(libcfuzzed)

add_library(cfuzzed STATIC
  libcfuzzed.c
)

add_library(cfuzzed-preload SHARED
  libcfuzzed-preload.c
  src/fuzz_cwd.c
  src/prefix_subst.c
)

target_compile_options(cfuzzed-preload PRIVATE
  -fPIC
)

install(
  TARGETS cfuzzed cfuzzed-preload
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
