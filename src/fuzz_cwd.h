#include <stddef.h>
#include <stdint.h>

void libcfud_fuzz_cwd_init();

size_t libcfud_fuzz_cwd_reset(const uint8_t *data, size_t size);
