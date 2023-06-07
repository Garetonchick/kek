#pragma once
#include "session.h"

enum EVENT_TYPE { NEW_CONNECTION, CLIENT_INPUT, COMMAND_INPUT, COMMAND_END };

typedef struct Event {
    Session *sus;
    int fd;
    int flags;
    int type;
    void *data;
} Event;

bool InitEventSystem();
bool AddEvent(Session *sus, int efd, int eflags, int etype, void *edata);
bool AddNotifier(Session *sus, int etype);
bool Notify(int notify_fd);
Event *WaitEvent();
Session *DelEvent(Event *e);
