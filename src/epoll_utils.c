#include "epoll_utils.h"

#include <memory.h>
#include <stdlib.h>
#include <sys/epoll.h>

typedef struct EventData {
    int fd;
    void* data;
} EventData;

static int last_waited_fd = -1;
static EventData* last_waited_event_data = NULL;

struct epoll_event CreateEpollEvent(int fd, int flags) {
    struct epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.data.fd = fd;
    evt.events = flags;
    return evt;
}

void AddEpollEvent(int epollfd, int fd, void* data, int flags) {
    struct epoll_event evt = CreateEpollEvent(fd, flags);
    EventData* edata = calloc(1, sizeof(*edata));
    edata->data = data;
    edata->fd = fd;
    evt.data.ptr = (void*)edata;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &evt);
}

void DelEpollEvent(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);

    if(fd == last_waited_fd) {
        free(last_waited_event_data);
        last_waited_event_data = NULL;
        last_waited_fd = -1;
    }
}

int WaitEpollEvent(int epollfd, void** data, int* fd, uint32_t* events) {
    struct epoll_event evt = {.events = *events, .data.ptr = NULL };
    int rval = epoll_wait(epollfd, &evt, 1, -1); 
    *events = evt.events;
    last_waited_event_data = evt.data.ptr;
    *fd = last_waited_fd = last_waited_event_data->fd;

    if(data) {
        *data = last_waited_event_data->data;
    }

    return rval;
}
