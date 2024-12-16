#include <ctype.h>
#include <time.h>
#ifdef LOCAL
#include <utils.h>
#else
#include "utils.h"
#endif

// utility to duplicate a string
char* my_strdup(const char *src) {
  if (src == NULL) return NULL;
  const unsigned long len = strlen(src) + 1;
  char *dest = malloc(len);
  assert(dest != NULL);
  memcpy(dest, src, sizeof(char) * len);
  return dest;
}

// simple utility function
void reverseArray(int arr[], const int size) {
  int start = 0, end = size - 1;
  while (start < end) {
    const int temp = arr[start];
    arr[start] = arr[end];
    arr[end] = temp;
    start++;
    end--;
  }
}

/**
 * @return a copy of string
*/
const char* randomString(const int len, const char *suffix) {
  assert(len >= 1 && suffix != NULL);
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const int charsetSize = sizeof(charset) - 1;
  srand(time(0));
  char *str = malloc(sizeof(char) * (len + strlen(suffix) + 1));
  for (int i = 0; i < len; ++i) {
    const int key = rand() % charsetSize;
    str[i] = charset[key];
  }
  str[len] = '\0';
  strcat(str, suffix);
  return str;
}

void error(const int type, const int lineNum, const char *message, ...) {
  printf("Error type %d at Line %d: ", type, lineNum);
  va_list arg;
  va_start(arg, message);
  vprintf(message, arg);
  va_end(arg);
}

/**
 * @brief turn int to string
 * @note remember to free.
 */
const char* int2String(const int n) {
  char *s;
  asprintf(&s, "%d", n);
  return s;
}

/**
 * @brief turn float to string
 * @note remember to free.
 */
const char* float2String(const float f) {
  char *s;
  asprintf(&s, "%f", f);
  return s;
}

bool in(const int target, const int num, ...) {
  va_list args;
  va_start(args, num);
  for (int i = 0; i < num; ++i) {
    if (target == va_arg(args, int)) {
      va_end(args);
      return true;
    }
  }
  va_end(args);
  return false;
}

// helper function for deleteLabel
int cmp_int(const void *a, const void *b) {
  return *(int *) a - *(int *) b;
}
