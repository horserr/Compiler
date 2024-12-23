#ifndef UTILITIES
#define UTILITIES

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG_INFO(message...) do {\
    fprintf(stderr, message); \
    fprintf(stderr, "Checking from {%s :%d @%s}.\n",\
      __FILE__, __LINE__, __FUNCTION__);\
  } while(false)

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

// calculate the index of pointer
#define INDEX(base, p, size) ((((void*)p) - ((void*)base)) / (size))

#define RESIZE(capacity) (capacity) = ((capacity) + ((capacity) >> 1))

/**
 * @brief swap the contents which are pointed by `a` and `b` respectively, int a
 * byte-to-byte way
 * @note a and b both should be pointer types
 * copy from "qsort.c"
 * @link https://github.com/embeddedartistry/libc/blob/master/src/stdlib/qsort.c
 */
#define SWAP(a, b, size) do {\
    size_t __size = (size);\
    char *__a = (a), *__b = (b);\
    do {\
        char __tmp = *__a;/* a single byte */\
        *__a++ = *__b;\
        *__b++ = __tmp;\
    } while (--__size > 0);\
  } while (0)

int cmp_int(const void *a, const void *b);
int cmp_str(const void *a, const void *b);

char* my_strdup(const char *src);
const char* randomString(int len, const char *suffix);
void error(int type, int lineNum, const char *message, ...);
const char* int2String(int n);
const char* float2String(float f);
bool in(int target, int num, ...);

void reverseArray(void *base, size_t len, size_t size);
void removeDuplicates(void *base, size_t *len, size_t size, __compar_fn_t cmp);
int findInArray(const void *key, const void *base, size_t len,
                size_t size, __compar_fn_t cmp);

#endif
