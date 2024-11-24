#ifndef UTILITIES
#define UTILITIES

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_INFO(message) \
  fprintf(stderr, "%sChecking from {%s :%d @%s}.\n",\
    message, __FILE__, __LINE__, __FUNCTION__)

#define ARRAY_LEN(array) (sizeof(array) / sizeof((array)[0]))

char* my_strdup(const char *src);
void reverseArray(int arr[], int size);
const char* randomString(int len, const char *suffix);
void error(int type, int lineNum, const char *message, ...);
const char* int2String(const int n);
const char* float2String(const float f);

#endif
