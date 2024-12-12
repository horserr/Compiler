#ifndef MORPH__H
#define MORPH__H
#ifdef LOCAL
#include <IR.h>
#include <utils.h>
#else
#include "utils.h"
#endif

typedef struct {
} Object;


void printMIPS(const char *file_name, const Chunk *sentinel);


#endif
