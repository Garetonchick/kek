#pragma once
#include <sys/epoll.h>

struct epoll_event CreateEpollEvent(int fd, int flags);
void AddEpollEvent4(int epollfd, int fd, void *data, int flags);
void AddEpollEvent3(int epollfd, int fd, int flags);
void DelEpollEvent(int epollfd, int fd);
int WaitEpollEventData(int epollfd, void **edata, uint32_t *events);
int WaitEpollEventFD(int epollfd, int *efd, uint32_t *events);
