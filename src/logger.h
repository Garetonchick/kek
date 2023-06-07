#pragma once
#include <stdbool.h>

bool InitLogger(const char *logname);
void DestroyLogger();
void Log(const char *format, ...);
void Logf(const char *format, ...);
void Flush();
