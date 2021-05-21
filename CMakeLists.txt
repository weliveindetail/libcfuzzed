cmake_minimum_required(VERSION 3.10)

project(libcfuzzer)

add_library(cfud STATIC
  libcfud.c
)

add_library(cfud-preload SHARED
  libcfu.c
)

target_compile_options(cfud-preload PRIVATE
  -fPIC
)

install(
  TARGETS cfud cfud-preload
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
