#pragma once
#include <stdint.h>

int ParseArgs(char** argv, const char* format, ...);
uint32_t CalcArgsSize(char** args);
uint32_t CalcArgsNum(char** args);
