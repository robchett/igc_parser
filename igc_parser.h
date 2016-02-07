#pragma once

char *gridref_number_to_letter(int64_t e, int64_t n);

#ifdef DEBUG_LEVEL
#define note(S, ...) fprintf(stderr, "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[90mnote:\x1b[0m " S "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define warn(S, ...) fprintf(stderr, "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[33mwarning:\x1b[0m " S "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define errn(S, ...) fprintf(stderr, "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[31merror:\x1b[0m " S "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define note(S, ...)
#define warn(S, ...)
#define errn(S, ...)
#endif
