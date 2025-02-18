#include "util.h"

int oct2bin(const char *str, int size) {
    int n = 0;
    char c;

    for (int i = 0; i < size; i++) {
        c  = str[i];
        n *= 8;
        n += (c - '0');
    }

    return n;
}
