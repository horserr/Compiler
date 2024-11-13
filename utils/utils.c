#include "utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// utility to duplicate a string
char* my_strdup(const char* src) {
    if (src == NULL) {
        return NULL;
    }
    // Allocate memory for the new string
    char* dest = malloc(strlen(src) + 1);
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
const char* randomString(const int len, const char* suffix) {
    assert(len >= 1 && suffix != NULL);
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charsetSize = sizeof(charset) - 1;
    srand(time(0));
    char* str = malloc(sizeof(char) * (len + strlen(suffix) + 1));
    for (int i = 0; i < len; ++i) {
        const int key = rand() % charsetSize;
        str[i] = charset[key];
    }
    str[len] = '\0';
    strcat(str, suffix);
    return str;
}

void error(const int type, const int lineNum, const char* message) {
    // const char* types = "ABC";
    // type = (char)toupper(type);
    // if (strchr(types, type) == NULL) {
    //     fprintf(stderr, "Wrong error type: %c. Should be %s.\n", type, types);
    //     exit(EXIT_FAILURE);
    // }
    printf("Error type %d at Line %d: %s", type, lineNum, message);
}
