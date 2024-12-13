#include <ctype.h>
#include <time.h>
#ifdef LOCAL
#include <utils.h>
#else
#include "utils.h"
#endif

// utility to duplicate a string
char* my_strdup(const char *src) {
  if (src == NULL) {
    return NULL;
  }
  // Allocate memory for the new string
  char *dest = malloc(strlen(src) + 1);
  if (dest == NULL) {
    perror("fail to allocate memory in {my_strdup}.\n");
    exit(EXIT_FAILURE);
  }

  // Copy the contents of the source string to the destination string
  strcpy(dest, src);
  return dest;
}

// simple utility function
void reverseArray(int arr[], const int size) {
  int start = 0;
  int end = size - 1;
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
  char buffer[30];
  sprintf(buffer, "%d", n);
  return my_strdup(buffer);
}

/**
 * @brief turn float to string
 * @note remember to free.
 */
const char* float2String(const float f) {
  char buffer[30];
  sprintf(buffer, "%f", f);
  return my_strdup(buffer);
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
