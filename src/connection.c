#include "connection.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

int CreateConnection(const char *hostname, const char *port) {
    struct addrinfo *addrinfo = NULL;
    struct addrinfo hint = {.ai_family = AF_INET,
                            .ai_socktype = SOCK_STREAM,
                            .ai_protocol = IPPROTO_TCP};
    int gay_err;
    if ((gay_err = getaddrinfo(hostname, port, &hint, &addrinfo)) != 0) {
        fprintf(stderr, "%s\n", gai_strerror(gay_err));
        return -1;
    }

    int sock = -1;

    for (struct addrinfo *ai = addrinfo; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC, ai->ai_protocol);

        if (sock < 0) {
            perror("socket");
            continue;
        }

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
            perror("connect");
            close(sock);
            sock = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(addrinfo);
    return sock;
}

int CreateListener(const char *port) {
    struct addrinfo *addrinfo = NULL;
    struct addrinfo hint = {.ai_family = AF_INET,
                            .ai_socktype = SOCK_STREAM,
                            .ai_protocol = IPPROTO_TCP,
                            .ai_flags = AI_PASSIVE};
    int gay_err;
    if ((gay_err = getaddrinfo(NULL, port, &hint, &addrinfo)) != 0) {
        fprintf(stderr, "Gay err: %s\n", gai_strerror(gay_err));
        return -1;
    }

    int sock = -1;

    for (struct addrinfo *ai = addrinfo; ai; ai = ai->ai_next) {
        sock = socket(ai->ai_family, ai->ai_socktype | SOCK_CLOEXEC, 0);

        if (sock < 0) {
            perror("socket");
            continue;
        }
        int one = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));

        if (bind(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
            perror("bind");
            close(sock);
            sock = -1;
            continue;
        }
        if (listen(sock, SOMAXCONN) < 0) {
            perror("listen");
            close(sock);
            sock = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(addrinfo);
    return sock;
}
