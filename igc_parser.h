#pragma once

char *gridref_number_to_letter(int64_t e, int64_t n);

// -1 = non3
// 0 = errors
// 1 = warnings
// 2 = debug
#define DEBUG_LEVEL 1

#if DEBUG_LEVEL >= 2
# define note(S, ...) fprintf(stderr, "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[90mnote:\x1b[0m " S "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
# define note(S, ...)
#endif

#if DEBUG_LEVEL >= 1
# define warn(S, ...) fprintf(stderr, "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[33mwarning:\x1b[0m " S "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
# define warn(S, ...)
#endif

#if DEBUG_LEVEL >= 0
# define errn(S, ...) fprintf(stderr, "\x1b[1m(%s:%d, %s)\x1b[0m\n  \x1b[1m\x1b[31merror:\x1b[0m " S "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
# define errn(S, ...)
#endif