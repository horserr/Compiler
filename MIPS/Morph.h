#ifndef MORPH__H
#define MORPH__H
#ifdef LOCAL
#include <IR.h>
#include <Optimize.h>
#include <utils.h>
#else
#include "IR.h"
#include "Optimize.h"
#include "utils.h"
#endif

void printMIPS(const char *file_name, const Block *blocks);

#endif
