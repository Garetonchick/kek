#include "epoll_utils.h"

#include <memory.h>
#include <stdlib.h>
#include <sys/epoll.h>

typedef struct EventData {
    int fd;
    void* data;
} EventData;

struct epoll_event CreateEpollEvent(int fd, int flags) {
    struct epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.data.fd = fd;
    evt.events = flags;
    return evt;
}

void AddEpollEvent4(int epollfd, int fd, void* data, int flags) {
    struct epoll_event evt = CreateEpollEvent(fd, flags);
    evt.data.ptr = data;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &evt);
}

void AddEpollEvent3(int epollfd, int fd, int flags) {
    struct epoll_event evt = CreateEpollEvent(fd, flags);
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &evt);
}

void DelEpollEvent(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

int WaitEpollEventData(int epollfd, void** edata, uint32_t* events) {
    struct epoll_event evt = {.events = *events, .data.ptr = NULL };
    int rval = epoll_wait(epollfd, &evt, 1, -1); 
    *events = evt.events;
    *edata = evt.data.ptr;

    return rval;
}

int WaitEpollEventFD(int epollfd, int* efd, uint32_t* events) {
    struct epoll_event evt = {.events = *events, .data.fd = 0 };
    int rval = epoll_wait(epollfd, &evt, 1, -1); 
    *events = evt.events;
    *efd = evt.data.fd;

    return rval;
}
