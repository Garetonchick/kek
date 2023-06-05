#pragma once
#include <stdbool.h>

typedef struct Session {
    int conn;
    int command_infd;
    int command_outfd; 
} Session;

bool CreateSession(int epollfd, int conn);
void ShutdownSession(int epollfd, Session* sus);
