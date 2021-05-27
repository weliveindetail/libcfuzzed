function(find_fuzzer_no_main archive_full_name)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -print-target-triple
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  OUTPUT_VARIABLE clang_triple
                  RESULT_VARIABLE clang_exit_code)
  if(NOT clang_exit_code EQUAL "0")
    message(FATAL_ERROR "Failed to obtain target triple from clang")
  endif()

  string(REPLACE "-" ";" clang_triple ${clang_triple})
  list(GET clang_triple 0 clang_arch_str)

  set(libfuzzer_filename libclang_rt.fuzzer_no_main-${clang_arch_str}.a)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -print-file-name=${libfuzzer_filename}
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                  OUTPUT_VARIABLE clang_libfuzzer_fullname
                  RESULT_VARIABLE clang_exit_code)
  if(NOT clang_exit_code EQUAL "0")
    message(FATAL_ERROR "Failed to obtain target triple from clang")
  endif()

  set(${archive_full_name} ${clang_libfuzzer_fullname} PARENT_SCOPE)
endfunction()

add_executable(unit_test
  unit_test.c
)

target_include_directories(unit_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}
)

target_compile_options(unit_test PRIVATE
  -fsanitize=fuzzer-no-link
  -Wno-gnu-folding-constant
  -Wno-deprecated-declarations
)

target_link_options(unit_test PRIVATE
  -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/unit_test.exported
  -fsanitize=fuzzer-no-link
  -fuse-ld=lld-12
)

find_fuzzer_no_main(libclang_rt_fuzzer_no_main)
message(STATUS "Found clang fuzzer static archive: ${libclang_rt_fuzzer_no_main}")

target_link_libraries(unit_test PRIVATE
  cfuzzed
  cfuzzed-preload
  ${libclang_rt_fuzzer_no_main}
)

configure_file(test_file_abc ../libcfuzzed_build_root)
configure_file(test_file_abc libcfuzzed_build_test)
configure_file(test_file_abc nested/libcfuzzed_build_nested)
