#ifndef UTILITIES
#define UTILITIES

char* my_strdup(const char* src);
void reverseArray(int arr[], int size);
const char* randomString(int len, const char* suffix);
void error(int type, int lineNum, const char* message);

#endif
