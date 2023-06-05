#include "argparse.h"

#include <string.h>

uint32_t CalcArgsSize(char** args) {
    uint32_t size = 0;

    while(*args) {
        size += strlen(*args);
    }

    return size;
}
