cmake_minimum_required(VERSION 3.10)

project(libcfud)

add_library(cfud STATIC
  libcfud.c
)

add_library(cfud-preload SHARED
  libcfud-preload.c
  src/fuzz_cwd.c
  src/prefix_subst.c
)

target_compile_options(cfud-preload PRIVATE
  -fPIC
)

install(
  TARGETS cfud cfud-preload
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
