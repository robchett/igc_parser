#include <php.h>

char *fitos(int64_t input, char *format) {
    int64_t length = snprintf(NULL, 0, format, input);
    char *buffer = emalloc(sizeof(char) * length + 1);
    snprintf(buffer, length + 1, format, input);
    return buffer;
}

char *fdtos(double input, char *format) {
    int64_t length = snprintf(NULL, 0, format, input);
    char *buffer = emalloc(sizeof(char) * length + 1);
    snprintf(buffer, length + 1, format, input);
    return buffer;
}

char *itos(int64_t input) {
    return fitos(input, "%d");
}

char *dtos(double input) {
    return fdtos(input, "%f");
}

char *create_buffer(char *string) {
    char *buffer = emalloc(sizeof(char) * strlen(string) + 1);
    strcpy(buffer, string);
    return buffer;
}

char *vstrcat(char *buffer, ...) {
    va_list ap;
    va_start(ap, buffer);
    size_t buffer_size = strlen(buffer);
    size_t used_size = strlen(buffer);
    char *string;
    while (string = va_arg(ap, char *)) {
        int64_t length = strlen(string);
        while (buffer_size - used_size < length) {
            buffer_size = buffer_size ? buffer_size * 2 : 100;
            buffer = erealloc(buffer, sizeof(char) * buffer_size + 1);
        }
        used_size += length;
        buffer = strcat(buffer, string);
    }
    va_end(ap);
    return buffer;
}