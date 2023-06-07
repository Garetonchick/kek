#include "logger.h"

#include <stdio.h>

static FILE *g_log_file = NULL;

bool InitLogger(const char *logname) {
    g_log_file = fopen(logname, "we");  // are the champions

    if (!g_log_file) {
        return false;
    }

    Log("Started logger\n");
    return true;
}

void DestroyLogger() {
    if (g_log_file) {
        fclose(g_log_file);
    }
}

void Log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(g_log_file, format, args);
    va_end(args);
}

void Logf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(g_log_file, format, args);
    va_end(args);
    Flush();
}

void Flush() {
    fflush(g_log_file);
}
