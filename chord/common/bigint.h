#pragma once

#include <openssl/sha.h>
#include <stdbool.h>

#ifndef BYTES
#define BYTES SHA_DIGEST_LENGTH
#endif

#define compare(A, B) memcmp((A), (B), BYTES)

namespace chord {
void add(const uint8_t *a, uint8_t *b);
void pow2(uint8_t a, uint8_t *b);
bool within(const void *value, const void *lower, const void *upper);
void print(const uint8_t *a);
void sprint(char *dest, const uint8_t *a);
}  // namespace chord
