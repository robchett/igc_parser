#ifndef GEOMETRY_STRING
#define GEOMETRY_STRING

char *itos(long input);
char *fitos(long input, char *format);
char *dtos(double input);
char *fdtos(double input, char *format);

char *create_buffer(char *string);
char *vstrcat(char *buffer, ...);

#endif