void set_actual_cwd(const char *cwd);
void set_fuzzed_cwd(const char *cwd);

void fuzz_cwd(const char *path, char *path_out);
void unfuzz_cwd(const char *path, char *path_out);
