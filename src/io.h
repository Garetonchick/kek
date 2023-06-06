#pragma once
#include <stdbool.h>

bool WriteInt(int fd, int val);

bool ReadInt(int fd, int* val);
