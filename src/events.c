#include "events.h"

#include "epoll_utils.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>

static int epollfd = -1;

bool InitEventSystem() {
    epollfd = epoll_create1(EPOLL_CLOEXEC);
    return epollfd != -1;
}

bool AddEvent(Session* sus, int efd, int eflags, int etype, void* edata) {
    Event* e = calloc(1, sizeof(*e));

    if(!e) {
        return false;
    }

    e->sus = sus;
    e->fd = efd;
    e->flags = eflags;
    e->type = etype;
    e->data = edata;

    if(e->sus) {
        e->sus->refcnt++;
    }

    AddEpollEvent4(epollfd, efd, e, eflags);
    return true;
}

bool AddNotifier(Session* sus, int etype) {
    int efd = eventfd(0, EFD_CLOEXEC);
    return efd != -1 && AddEvent(sus, efd, EPOLLIN, etype, NULL);
}

bool Notify(int notify_fd) {
    char buf[8];
    return write(notify_fd, buf, sizeof(buf)) == sizeof(buf);
}

Event* WaitEvent(uint32_t eflags) {
    Event* e;
    if(WaitEpollEventData(epollfd, (void**)&e, &eflags) <= 0) {
        return NULL;
    }
    e->flags = eflags;

    return e;
}

Session* DelEvent(Event* e) {
    Session* sus = e->sus;
    DelEpollEvent(epollfd, e->fd);
    e->sus->refcnt--;
    free(e->data);
    free(e);
    return sus;
}
