#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bigint.h"

namespace chord {

uint8_t ZERO[BYTES] = {0};
uint8_t ONE[BYTES]  = {[BYTES - 1] = 1};

void add(const uint8_t *a, uint8_t *b) {
    int8_t index = 0, overflow = 0;
    for (index = BYTES - 1; index > -1; index--) {
        uint16_t sum = a[index] + b[index] + overflow;
        overflow     = sum > UCHAR_MAX ? 1 : 0;
        b[index]     = sum & 0xff;
    }
}

void pow2(uint8_t exponent, uint8_t *dest) {
    memset(dest, 0, BYTES);
    dest[BYTES - (exponent / 8) - 1] = 1 << (exponent % 8);
}

bool within(const void *value, const void *lower, const void *upper) {
    int lowupp = compare(lower, upper);
    int lowcmp = compare(lower, value), upcmp = compare(value, upper);
    int lowlim = 0, uplim = 1;

    if (lowupp < 0)
        return lowcmp < lowlim && upcmp < uplim;
    else if (lowupp > 0)
        return lowcmp < lowlim || upcmp < uplim;
    else
        return lowcmp != 0;
}

void print(const uint8_t *a) {
    int index;
    for (index = 0; index < BYTES; index++) printf("%02x", a[index]);
}

void sprint(char *dest, const uint8_t *a) {
    int index;
    for (index = 0; index < BYTES; index++) sprintf(dest + 2 * index, "%02x", a[index]);
}
}  // namespace chord