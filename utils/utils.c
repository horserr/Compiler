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

int cmp_int(const void *a, const void *b) { return *(int *) a - *(int *) b; }

// notice this function!
int cmp_str(const void *a, const void *b) { return strcmp(*(char **) a, *(char **) b); }

/**
 * reverse an array.
 * @param base the name of array to be reversed
 * @param len the number of members in array, mostly equals to array length
 * @param size the type size of each element
 */
void reverseArray(void *base, const size_t len, const size_t size) {
  char *start = base, *end = start + (len - 1) * size;
  while (start < end) {
    SWAP(start, end, size);
    start += size;
    end -= size;
  }
}

/**
 * @brief remove duplicate elements within the "ordered" array, and may update the @param len
 * @note the given array must be ordered
 * @param base the name of array to be modified
 * @param len pointer to the number of members in array, mostly equals to array length
 * @param size the type size of each element
 * @param cmp the compare function
 */
void removeDuplicates(void *base, size_t *len, const size_t size,
                      const __compar_fn_t cmp) {
  char *base_ = base;
  for (int i = 1; i < *len; ++i) {
    const void *p = (char *) base + i * size;
    if (cmp(base_, p) != 0) {
      base_ += size;
      memcpy(base_, p, size);
    }
  }
  *len = INDEX(base, base_, size) + 1;
}

// Find in sequential order in an array. Alas, the better approach is to use native `bsearch`
__attribute__((warn_unused_result))
int findInArray(const void *key, const void *base, const size_t len,
                const size_t size, const __compar_fn_t cmp) {
  for (int i = 0; i < len; ++i) {
    const void *tmp = (char *) base + i * size;
    if (cmp(key, tmp) == 0) return i;
  }
  return -1;
}
