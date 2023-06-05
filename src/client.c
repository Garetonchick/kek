#include "connection.h"
#include "protocol.h"
#include "epoll_utils.h"
#include "logger.h"

#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#define SERVER_PORT "444444"

void PrintHelpMessage() {
    const char* message = "Usage: client <host> spawn <command> [args...]";
    printf("%s\n", message);
}

void PrintUnknownClientCommandMessage(const char* command) {
    const char* message = "Unknown client command: %s\n";
    printf(message, command);
}

bool HandleServerInput(int epollfd, int conn) {
    (void)epollfd;
    KDU kdu;

    if(!RecieveKDU(conn, &kdu)) {
        return false;
    }

    if(kdu.type != KEKS_COMMAND_OUTPUT) {
        return false;
    }

    write(STDOUT_FILENO, kdu.data, kdu.size);

    FreeKDU(&kdu);
    return true;
}

bool HandleUserInput(int epollfd, int conn) {
    (void)epollfd;
    static char rdbuf[KDU_MAX_DATA_SIZE];
    int bytes_read = 0;

    if((bytes_read = read(STDIN_FILENO, rdbuf, KDU_MAX_DATA_SIZE)) <= 0) {
        return false;
    }
    printf("bytes_read = %d\n", bytes_read);
    fflush(stdout);

    if(!SendKDU5(conn, KEKC_USER_INPUT, bytes_read, rdbuf)) {
        printf("Connection closed\n");
        return false;
    }

    return true;
}

void HandleEvents(int conn, char** args) {
    int epollfd = epoll_create1(0);
    AddEpollEvent(epollfd, conn, NULL, EPOLLIN);
    AddEpollEvent(epollfd, STDIN_FILENO, NULL, EPOLLIN);

    if(!SendCommandKDU(conn, *args, args + 1)) {
        printf("Connection closed\n");
        return;
    }

    while(true) {
        int efd;
        uint32_t evt = EPOLLIN;

        if(!WaitEpollEvent(epollfd, NULL, &efd, &evt)) {
            return;
        }

        if(evt != EPOLLIN) {
            Logf("Error epoll event\n");
            return;
        }

        if(efd == conn) {
            if(!HandleServerInput(epollfd, conn)) {
                return;
            }
        } else {
            printf("Handle user input\n");
            fflush(stdout);
            if(!HandleUserInput(epollfd, conn)) {
                return;
            }
        }
    }
}

void HandleSpawn(char** args, const char* host) {
    int conn = CreateConnection(host, SERVER_PORT);

    if(conn < 0) {
        fprintf(stderr, "Couldn't connect to server\n");
        return;
    }

    HandleEvents(conn, args);
}

int main(int argc, char** args) { 
    if (argc < 4) {
        PrintHelpMessage();       
        return 1;
    }

    if(!InitLogger("clog.txt")) {
        fprintf(stderr, "Failed to start logger\n");
        return 1;
    }

    const char* host = args[1];
    const char* client_command = args[2];

    if(strcmp(client_command, "spawn") == 0) {
        HandleSpawn(args + 3, host);        
    } else {
        PrintUnknownClientCommandMessage(client_command);
        return 1;
    }
}
