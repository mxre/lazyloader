
void initialize_lazy_loader(const char* library);

void set_lazyloader_error_callback(void (*f)(const char *err, void* cb_data), void* cb_data);
