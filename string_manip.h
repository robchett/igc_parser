#pragma once

char *itos(int64_t input);
char *fitos(int64_t input, char *format);
char *dtos(double input);
char *fdtos(double input, char *format);

char *create_buffer(char *string);
char *vstrcat(char *buffer, ...);