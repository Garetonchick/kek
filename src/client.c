#include "connection.h"
#include "epoll_utils.h"
#include "logger.h"
#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

void PrintHelpMessage() {
    const char *message = "Usage: client <host> spawn <command> [args...]";
    printf("%s\n", message);
}

void PrintUnknownClientCommandMessage(const char *command) {
    const char *message = "Unknown client command: %s\n";
    printf(message, command);
}

bool HandleServerInput(int conn, uint32_t evt) {
    if (evt != EPOLLIN) {
        return false;
    }

    KDU kdu;

    if (!RecieveKDU(conn, &kdu)) {
        return false;
    }

    if (kdu.type != KEKS_COMMAND_OUTPUT) {
        return false;
    }

    write(STDOUT_FILENO, kdu.data, kdu.size);

    FreeKDU(&kdu);
    return true;
}

bool SendEOI(int conn) {
    return SendKDU5(conn, KEKC_USER_EOI, 0, NULL);
}

bool HandleUserInput(int conn, uint32_t evt) {
    if (evt != EPOLLIN) {
        Logf("Recieved end of input\n");
        return SendEOI(conn);
    }

    static char rdbuf[KDU_MAX_DATA_SIZE];
    int bytes_read = 0;

    if ((bytes_read = read(STDIN_FILENO, rdbuf, KDU_MAX_DATA_SIZE)) <= 0) {
        return SendEOI(conn);
    }

    if (!SendKDU5(conn, KEKC_USER_INPUT, bytes_read, rdbuf)) {
        Logf("Connection closed\n");
        return false;
    }

    return true;
}

void HandleEvents(int conn, char **args) {
    int epollfd = epoll_create1(0);
    AddEpollEvent3(epollfd, conn, EPOLLIN | EPOLLERR | EPOLLRDHUP);
    AddEpollEvent3(epollfd, STDIN_FILENO, EPOLLIN | EPOLLERR | EPOLLRDHUP);

    if (!SendCommandKDU(conn, *args, args + 1)) {
        Logf("Connection closed\n");
        return;
    }

    while (true) {
        int efd;
        uint32_t evt = EPOLLIN | EPOLLERR | EPOLLRDHUP;

        if (!WaitEpollEventFD(epollfd, &efd, &evt)) {
            Logf("Epoll wait failed\n");
            continue;
        }

        if (efd == conn) {
            if (!HandleServerInput(conn, evt)) {
                return;
            }
        } else {
            if (!HandleUserInput(conn, evt)) {
                return;
            }
        }
    }
}

void HandleSpawn(char **args, const char *host, const char *port) {
    int conn = CreateConnection(host, port);

    if (conn < 0) {
        fprintf(stderr, "Couldn't connect to server\n");
        return;
    }

    HandleEvents(conn, args);
}

int main(int argc, char **args) {
    if (argc < 4) {
        PrintHelpMessage();
        return 1;
    }

    if (!InitLogger("clog.txt")) {
        fprintf(stderr, "Failed to start logger\n");
        return 1;
    }

    const char *host = args[1];
    const char *port = args[2];
    const char *client_command = args[3];

    if (strcmp(client_command, "spawn") == 0) {
        HandleSpawn(args + 4, host, port);
    } else {
        PrintUnknownClientCommandMessage(client_command);
        return 1;
    }
}
