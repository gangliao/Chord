#ifndef NETBUFFER_H
#define NETBUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if !(defined(OS_MACOSX) || defined(__APPLE__))
uint64_t htonll(uint64_t val);
uint64_t ntohll(uint64_t val);
#endif

namespace chord {

/* BUFFER UTILS */

typedef struct
{
    uint8_t *buf;
    size_t len;
    size_t position;
} NetBuffer;

/* In buffer should have a size of length */
void netbuf_init(NetBuffer *net_buf, void *in_buf, size_t len);

/**
 * Returns true if seek if within bounds,
 * otherwise returns false and the position
 * remains unchanged.
 */
bool netbuf_seek_offset(NetBuffer *net_buf, size_t offset);
bool netbuf_seek(NetBuffer *net_buf, size_t position);
void netbuf_seek_start(NetBuffer *net_buf);
void netbuf_seek_end(NetBuffer *net_buf);
bool netbuf_at_end(NetBuffer *net_buf);

/**
 * Converts from host to network byte order.
 * Pushes data onto the end of the buffer.
 * Returns true if within the bounds of the buffer,
 * otherwise returns false and nothing happens.
 */
bool push_uint8(NetBuffer *net_buf, uint8_t val);
bool push_uint16(NetBuffer *net_buf, uint16_t val);
bool push_uint32(NetBuffer *net_buf, uint32_t val);
bool push_uint64(NetBuffer *net_buf, uint64_t val);
bool push_generic_uint(NetBuffer *net_buf, uint64_t val, size_t uint_bytes);

/**
 * Doesn't convert byte order.
 * Returns true if within the bounds of the buffer,
 * otherwise returns false and nothing happens.
 */
bool push_data(NetBuffer *net_buf, const void *data, size_t data_len);

/**
 * Converts from network to host byte order.
 * Reads data starting at the beginning of the buffer,
 * and working towards the end. Returns true
 * if within the bounds of the buffer,
 * otherwise returns false and nothing happens.
 */
bool read_uint8(NetBuffer *net_buf, uint8_t *out);
bool read_uint16(NetBuffer *net_buf, uint16_t *out);
bool read_uint32(NetBuffer *net_buf, uint32_t *out);
bool read_uint64(NetBuffer *net_buf, uint64_t *out);
bool read_generic_uint(NetBuffer *net_buf, uint64_t *out, size_t uint_bytes);

/**
 * Doesn't convert byte order.
 * Returns true if within the bounds of the buffer,
 * otherwise returns false and nothing happens.
 */
bool read_data(NetBuffer *net_buf, void *data_out, size_t data_len);

/* string_out should be of size raw_string_len + 1 to hold the null terminator */
bool read_string_data(NetBuffer *net_buf, void *string_out, size_t raw_string_len);
}  // namespace chord

#endif /* END NETBUFFER_H */
