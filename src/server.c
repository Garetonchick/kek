#include "connection.h"

#include "epoll_utils.h"
#include "logger.h"
#include "protocol.h"
#include "session.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <bits/types/sigset_t.h>

typedef struct ConnectionInfo {
    int conn;
    int infd;
    int outfd; 
} ConnectionInfo;

void PrintHelpMessage() {
    const char* message = "Usage: server <port>";
    printf("%s\n", message);
}

bool StartCommand(const char* command, int* infd, int* outfd) {
    int to_child[2];
    int to_batya[2];

    pipe(to_child);
    pipe(to_batya);

    dup2(to_child[0], STDIN_FILENO); 
    dup2(to_batya[1], STDOUT_FILENO); 
    *infd = to_batya[0];
    *outfd = to_child[1];

    if(system(command) < 0) {
        return false;
    }

    return true;
}

bool ProcessKDU(Session* sus, KDU* kdu) {
    if(kdu->type == KEKC_USER_INPUT) {
        if(write(sus->command_outfd, kdu->data, kdu->size) != kdu->size) {
            Logf("Connection closed\n");
            return false;
        }
    } else {
        Logf("Unknown kdu type\n%s\n", SerializeKDU(kdu));
        return false;
    }

    return true;
}

bool HandleClientInput(int epollfd, Session* sus) {
    KDU kdu;

    if(!RecieveKDU(sus->conn, &kdu)) {
        ShutdownSession(epollfd, sus);
        return false;
    }

    if(!ProcessKDU(sus, &kdu)) {
        Logf("Failed to process KDU\n");
        FreeKDU(&kdu);
        ShutdownSession(epollfd, sus);
        return false;
    } 

    FreeKDU(&kdu);
    return true;
}

bool HandleCommandInput(int epollfd, Session* sus) {
    (void)epollfd;
    static char buf[KDU_MAX_DATA_SIZE];
    int bytes_read = 0;

    if ((bytes_read = read(sus->command_infd, buf, sizeof(buf))) <= 0) {
        return false;
    }

    if(!SendKDU5(sus->conn, KEKS_COMMAND_OUTPUT, bytes_read, buf)) {
        return false; 
    }

    return true;
}

bool HandleEvent(int epollfd, int listener, Session* sus, int efd, uint32_t evt) {
    if(evt != EPOLLIN) {
        Logf("Error epoll event\n");
        return false;
    }

    if(efd == listener) {
        Logf("Handle accept event\n");
        int conn = accept(listener, NULL, NULL);
        Logf("Accepted connection\n");

        if(!CreateSession(epollfd, conn)) {
            Logf("Create session failed\n");
            return false;
        }
    } else if(efd == sus->conn) {
        Logf("Handle client input event\n");
        if(!HandleClientInput(epollfd, sus)) {
            return false;
        }
    } else if(efd == sus->command_infd) {
        Logf("Handle command input event\n");
        if(!HandleCommandInput(epollfd, sus)) {
            return false;
        }
    } else {
        assert(false && "how");
    }

    Logf("Sussessfuly leave handle event\n");
    return true;
}

void RunServer(int listener) {
    int epollfd = epoll_create1(0);
    AddEpollEvent(epollfd, listener, NULL, EPOLLIN);

    while(true) {
        Session* sus;
        int efd;
        uint32_t evt = EPOLLIN;

        if(!WaitEpollEvent(epollfd, (void**)&sus, &efd, &evt)) {
            Logf("Wait epoll failed\n");
            return;
        }

        if(!HandleEvent(epollfd, listener, sus, efd, evt)) {
            Logf("Handle event failed\n");
            return;
        }
    }
}

void SigchldHandler(int sig, siginfo_t* info, void* ucontext) {
    (void)sig; (void)ucontext; (void)info;
    Logf("Recieved sigchld\n");
    waitpid(info->si_pid, NULL, 0);
}

bool InitSignals() {
    sigset_t banned_sigs;
    sigfillset(&banned_sigs);
    sigdelset(&banned_sigs, SIGCHLD);
    sigdelset(&banned_sigs, SIGTERM);

    if(sigprocmask(SIG_BLOCK, &banned_sigs, NULL) < 0) {
        return false;
    }

    struct sigaction sigchld = {
        .sa_flags = SA_RESTART | SA_SIGINFO,
        .sa_sigaction = SigchldHandler
    };

    if(sigaction(SIGCHLD, &sigchld, NULL) < 0) {
        return false;
    } 

    return true;
}

bool Init() {
    if(!InitSignals()) {
        fprintf(stderr, "Failed to init signals\n");
        return false;
    }

    if(!InitLogger("slog.txt")) {
        fprintf(stderr, "Failed to start logger\n");
        return false;
    }

    if(daemon(1, 0) < 0) {
        fprintf(stderr, "Failed to start daemon\n");
        return false;
    }

    return true;
}

int main(int argc, char** args) {
    if(argc != 2) {
        PrintHelpMessage();
        return 0;
    }

    if(!Init()) {
        return 1;
    }

    const char* port = args[1];
    int listener = CreateListener(port);    

    if(listener < 0) {
        Log("Failed to listen on port: %s\n", port);
        return 1;
    }

    RunServer(listener);
    DestroyLogger();
}
