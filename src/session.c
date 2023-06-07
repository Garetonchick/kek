#include "session.h"

#include "protocol.h"
#include "logger.h"
#include "events.h"
#include "io.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>

static void Exec(const char* command) {
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
    cur = copy;

    while(*cur) {
        if(*cur == ' ') {
            args[idx++] = cur + 1;
            *cur = '\0';
        }
        ++cur;
    }

    execvp(args[0], args);
}

static void ExecAndNotifyEnd(const char* command, int notify_fd) {
    for (int fd = 3; fd < 256; ++fd) {
        if(fd != notify_fd) {
            (void)close(fd);
        }
    }

    int pid = fork();

    if(pid < 0) {
        WriteInt(notify_fd, -1);
        return;
    } else if(pid == 0) {
        Exec(command);
        _exit(1);
    }

    WriteInt(notify_fd, pid); // Send command pid
    waitpid(pid, NULL, 0);
    WriteInt(notify_fd, 1); // Notify command end

    _exit(0);
}

static bool StartCommand(Session* sus, const char* command) {
    int to_child[2];
    int to_batya[2];
    int to_batya_err[2];
    int notify_end[2];

    pipe(to_child);
    pipe(to_batya);
    pipe(notify_end);

    dup2(to_child[0], STDIN_FILENO); 
    dup2(to_batya[1], STDOUT_FILENO); 
    dup2(to_batya_err[1], STDERR_FILENO); 
    sus->command_infd = to_batya[0];
    sus->command_err_infd = to_batya_err[0];
    sus->command_outfd = to_child[1];

    AddEvent(sus, notify_end[0], EPOLLIN, COMMAND_END, NULL);

    int pid = fork();

    if(pid < 0) {
        return -1;
    } else if(pid == 0) {
        ExecAndNotifyEnd(command, notify_end[1]);
    }

    sus->waiter_pid = pid;
    ReadInt(notify_end[0], &sus->command_pid);

    return sus->waiter_pid >= 0 && sus->command_pid >= 0;
}

bool CreateSession(int conn) {
    Session* sus = calloc(1, sizeof(*sus));
    sus->conn = conn;
    AddEvent(sus, conn, EPOLLIN | EPOLLRDHUP, CLIENT_INPUT, NULL);
    KDU kdu;
    
    if(!RecieveKDU(conn, &kdu)) {
        Logf("Connection closed\n");
        close(sus->conn);
        sus->conn = -1;
        return false; 
    }
    if(kdu.type != KEKC_EXEC_COMMAND) {
        Logf("Expected command KDU\n");
        FreeKDU(&kdu);
        close(sus->conn);
        sus->conn = -1;
        return false;
    }

    Logf("Before start command\n");

    if(!StartCommand(sus, kdu.data)) {
        Logf("Invalid command\n");
        FreeKDU(&kdu);
        close(sus->conn);
        sus->conn = -1;
        return false;
    }

    Logf("After start command\n");
    FreeKDU(&kdu);
    AddEvent(sus, sus->command_infd, EPOLLIN, COMMAND_INPUT, NULL);
    AddEvent(sus, sus->command_err_infd, EPOLLIN, COMMAND_INPUT, NULL);

    return true;
}

void ShutdownSession(Session* sus) {
    if(sus->conn != SESSION_CLOSING) {
        close(sus->command_infd);
        close(sus->command_err_infd);
        close(sus->command_outfd);
        close(sus->conn);
        kill(sus->command_pid, SIGTERM);
        sus->conn = SESSION_CLOSING;
    } 

    if(!sus->refcnt) {
        Logf("Free session");
        free(sus);
    }
}
