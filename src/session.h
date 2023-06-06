#pragma once
#include <stdbool.h>

enum { SESSION_CLOSING = -1 };

typedef struct Session {
    int conn;
    int waiter_pid;
    int command_pid;
    int command_infd;
    int command_outfd; 
    int refcnt;
} Session;

bool CreateSession(int conn);
void ShutdownSession(Session* sus);
