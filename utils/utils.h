#ifndef UTILITIES
#define UTILITIES


#define DEBUG_INFO(message) \
        fprintf(stderr, "%sChecking from {file name: %s :%s @%s}.\n",\
                message, __FILE_NAME__, __LINE__, __FUNCTION__)

char* my_strdup(const char *src);
void reverseArray(int arr[], int size);
const char* randomString(int len, const char *suffix);
void error(int type, int lineNum, const char *message, ...);

#endif
