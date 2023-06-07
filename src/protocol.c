#include "protocol.h"

#include "logger.h"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool ReadFull(int sock, char *buf, int nbytes) {
    while (nbytes > 0) {
        int bytes_read = 0;
        if ((bytes_read = read(sock, buf, nbytes)) <= 0) {
            break;
        }
        nbytes -= bytes_read;
    }
    return nbytes == 0;
}

bool SendKDU(int sock, const KDU *kdu) {
    return SendKDU5(sock, kdu->type, kdu->size, kdu->data);
}

bool RecieveKDU(int sock, KDU *kdu) {
    return RecieveKDU5(sock, &kdu->type, &kdu->size, &kdu->data);
}

bool SendKDU5(int sock, uint32_t type, uint32_t size, const char *data) {
    Logf("Sending\n%s\n", SerializeKDU3(type, size, data));

    char header[KDU_HEADER_SIZE];
    *(uint32_t *)header = htonl(type);
    *(uint32_t *)(header + 4) = htonl(size);

    if (write(sock, header, KDU_HEADER_SIZE) != KDU_HEADER_SIZE ||
        write(sock, data, size) != size) {
        return false;
    }

    Logf("Send success\n");
    return true;
}

bool RecieveKDU5(int sock, uint32_t *type, uint32_t *size, char **data) {
    Logf("Try recieving KDU\n");
    char header[KDU_HEADER_SIZE];
    if (!ReadFull(sock, header, KDU_HEADER_SIZE)) {
        return false;
    }

    *type = ntohl(*(uint32_t *)header);
    *size = ntohl(*(uint32_t *)(header + 4));

    // if(*size > KDU_MAX_DATA_SIZE) {
    //     Logf("Used costil\n");
    //     *size = *type;
    //     *type = *(uint32_t*)(header + 4);
    // }

    *data = calloc(*size + 1, 1);

    Logf("Recieved KDU header ");
    Logf("[type:%" PRIu32 ", size:%" PRIu32 "]\n", *type, *size);

    if (!*data) {
        return false;
    }

    if (!ReadFull(sock, *data, *size)) {
        return false;
    }

    Logf("Recieve\n%s\n", SerializeKDU3(*type, *size, *data));
    return true;
}

bool SendCommandKDU(int sock, const char *command, char **args) {
    static char buf[KDU_MAX_DATA_SIZE];
    int offset = 0;
    int written = 0;

    if ((written = snprintf(buf + offset, KDU_MAX_DATA_SIZE - offset, "%s",
                            command)) < 0) {
        return false;
    }
    offset += written;

    if (offset >= KDU_MAX_DATA_SIZE) {
        return false;
    }

    while (*args) {
        if ((written = snprintf(buf + offset, KDU_MAX_DATA_SIZE - offset, " %s",
                                *args)) < 0) {
            return false;
        }
        offset += written;

        if (offset >= KDU_MAX_DATA_SIZE) {
            return false;
        }

        ++args;
    }

    return SendKDU5(sock, KEKC_EXEC_COMMAND, offset, buf);
}

const char *SerializeKDU(const KDU *kdu) {
    return SerializeKDU3(kdu->type, kdu->size, kdu->data);
}

const char *SerializeKDU3(uint32_t type, uint32_t size, const char *data) {
    static char buf[KDU_MAX_DATA_SIZE + KDU_HEADER_SIZE * 10];

    const char *format = "KDU[type:%" PRIu32 ", size:%" PRIu32 "]\nData:\"%s\"";
    char *tmp = strndup(data, size);
    int written = snprintf(buf, sizeof(buf), format, type, size, tmp);
    free(tmp);

    if (written <= 0 || written >= sizeof(buf)) {
        buf[0] = '\0';
        return buf;
    }

    return buf;
}

void FreeKDU(KDU *kdu) {
    free(kdu->data);
}
