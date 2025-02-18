#include "util.h"

int oct2bin(const char *str, int size) {
    int n            = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}
