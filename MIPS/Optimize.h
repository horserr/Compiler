#ifndef OPTIMIZE__H
#define OPTIMIZE__H
#ifdef LOCAL
#include <IR.h>
#include <utils.h>
#else
#include "utils.h"
#endif

void optimize(const Chunk *sentinel);

#endif
