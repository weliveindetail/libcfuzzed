#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

// The preload lib provides this function and expects it to be called for each
// new corpus.
void libcfuzzed_reset(const uint8_t *data, size_t size);
