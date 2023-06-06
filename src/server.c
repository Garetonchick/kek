#include "connection.h"

#include "logger.h"
#include "protocol.h"
#include "session.h"
#include "events.h"

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

void HandleClientInputEvent(Event* e) {
    KDU kdu;

    if(!RecieveKDU(e->fd, &kdu)) {
        ShutdownSession(DelEvent(e));
        return;
    }

    if(!ProcessKDU(e->sus, &kdu)) {
        Logf("Failed to process KDU\n");
        FreeKDU(&kdu);
        ShutdownSession(DelEvent(e));
        return;
    } 

    FreeKDU(&kdu);
}

void HandleCommandInputEvent(Event* e) {
    static char buf[KDU_MAX_DATA_SIZE];
    int bytes_read = 0;

    if ((bytes_read = read(e->sus->command_infd, buf, sizeof(buf))) <= 0) {
        ShutdownSession(DelEvent(e));
        return;
    }

    if(!SendKDU5(e->sus->conn, KEKS_COMMAND_OUTPUT, bytes_read, buf)) {
        ShutdownSession(DelEvent(e));
        return;
    }
}

void HandleNewConnectionEvent(Event* e) {
    Logf("Handle new connection event\n");
    int conn = accept(e->fd, NULL, NULL);
    Logf("Accepted connection\n");

    if(!CreateSession(conn)) {
        Logf("Create session failed\n");
    }
}

void HandleCommandEndEvent(Event* e) {
    ShutdownSession(DelEvent(e));   
}

void HandleEvent(Event* e) {
    if(e->flags != EPOLLIN) {
        Logf("Error epoll event\n");
        ShutdownSession(DelEvent(e));
        return;
    }

    switch(e->type) {
        case NEW_CONNECTION:
            HandleNewConnectionEvent(e);        
            break;
        
        case CLIENT_INPUT:
            HandleClientInputEvent(e);
            break;

        case COMMAND_INPUT:
            HandleCommandInputEvent(e);
            break;

        case COMMAND_END:
            HandleCommandEndEvent(e);
            break;

        default:
            assert(false && "Met unknown event type");
            break;
    }
}

void RunServer(int listener) {
    AddEvent(NULL, listener, EPOLLIN, NEW_CONNECTION, NULL);

    while(true) {
        Event* e;

        if((e = WaitEvent(EPOLLIN)) == NULL) {
            Logf("Wait epoll failed\n");
            continue;
        }

        HandleEvent(e);
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

    if(!InitEventSystem()) {
        fprintf(stderr, "Failed to init event system\n");
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
