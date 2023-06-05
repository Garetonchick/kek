#include "session.h"

#include "protocol.h"
#include "logger.h"
#include "epoll_utils.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void Exec(const char* command) {
    char* copy = strdup(command); 
    char* cur = copy;
    int cnt = 0;

    while(*cur) {
        if(*cur == ' ') {
            ++cnt;
        }
        ++cur;
    }
    ++cnt;

    char** args = calloc(cnt + 1, sizeof(char*));
    args[0] = copy; 
    int idx = 1;

    while(*cur) {
        if(*cur == ' ') {
            args[idx++] = cur + 1;
            *cur = '\0';
        }
        ++cur;
    }

    execvp(args[0], args);
}

static bool StartCommand(const char* command, int* infd, int* outfd) {
    int to_child[2];
    int to_batya[2];

    pipe(to_child);
    pipe(to_batya);

    dup2(to_child[0], STDIN_FILENO); 
    dup2(to_batya[1], STDOUT_FILENO); 
    *infd = to_batya[0];
    *outfd = to_child[1];

    int pid = fork();

    if(pid < 0) {
        return false;
    } else if(pid == 0) {
        Exec(command);
        _exit(1);
    }

    return true;
}

bool CreateSession(int epollfd, int conn) {
    Session* sus = calloc(1, sizeof(*sus));
    sus->conn = conn;
    AddEpollEvent(epollfd, conn, sus, EPOLLIN);
    KDU kdu;
    
    if(!RecieveKDU(conn, &kdu)) {
        Logf("Connection closed\n");
        return false;
    }

    if(kdu.type != KEKC_EXEC_COMMAND) {
        Logf("Expected command KDU\n");
        FreeKDU(&kdu);
        return false;
    }

    Logf("Before start command\n");

    if(!StartCommand(kdu.data, &sus->command_infd, &sus->command_outfd)) {
        Logf("Invalid command\n");
        FreeKDU(&kdu);
        return false;
    }

    Logf("After start command\n");
    FreeKDU(&kdu);

    AddEpollEvent(epollfd, sus->command_infd, sus, EPOLLIN);

    return true;
}

void ShutdownSession(int epollfd, Session* sus) {
    DelEpollEvent(epollfd, sus->conn);
    DelEpollEvent(epollfd, sus->command_infd);
    close(sus->command_infd);
    close(sus->command_outfd);
    close(sus->conn);
    free(sus);
}
