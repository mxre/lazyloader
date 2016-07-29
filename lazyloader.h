
// try to load from a list ob predifined library names and places
// look at environment variables
// returns 0 on success
int initialize_lazy_loader(int min_version);

// try to load one library (use before initalize)
// returns 0 on success
int try_lazy_load(const char* library, int min_version);

void set_lazyloader_error_callback(void (*f)(const char *err, void* cb_data), void* cb_data);
