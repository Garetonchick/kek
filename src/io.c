#include "io.h"

#include <unistd.h>

bool WriteInt(int fd, int val) {
    char buf[4];
    *(int*)buf = val;
    return write(fd, buf, 4) == 4;
}

bool ReadInt(int fd, int* val) {
    char buf[4];
    if(read(fd, buf, 4) != 4) {
        return false;
    }

    *val = *(int*)buf;
    return true;
}
