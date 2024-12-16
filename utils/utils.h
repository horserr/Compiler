#ifndef UTILITIES
#define UTILITIES

#include <assert.h>
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

#define swap(a, b) do {\
    typeof(a) temp_ = a;\
    a = b;\
    b = temp_;\
  } while (false)

char* my_strdup(const char *src);
void reverseArray(int arr[], int size);
const char* randomString(int len, const char *suffix);
void error(int type, int lineNum, const char *message, ...);
const char* int2String(int n);
const char* float2String(float f);
bool in(int target, int num, ...);
int cmp_int(const void *a, const void *b);

#endif
