#pragma once
#include <sys/epoll.h>

struct epoll_event CreateEpollEvent(int fd, int flags);
void AddEpollEvent(int epollfd, int fd, void* data, int flags);
void DelEpollEvent(int epollfd, int fd);
int WaitEpollEvent(int epollfd, void** data, int* fd, uint32_t* events);
