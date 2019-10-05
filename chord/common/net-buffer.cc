#include <arpa/inet.h>

#if defined(OS_MACOSX) || defined(__APPLE__)
#include <machine/endian.h>
#elif defined(OS_SOLARIS)
#include <sys/isa_defs.h>
#ifdef _LITTLE_ENDIAN
#define LITTLE_ENDIAN
#else
#define BIG_ENDIAN
#endif
#elif defined(OS_FREEBSD) || defined(OS_OPENBSD) || defined(OS_NETBSD) || defined(OS_DRAGONFLYBSD)
#include <sys/endian.h>
#include <sys/types.h>
#else
#include <endian.h>
#endif

#include <string.h>
#include "net-buffer.h"

#if !(defined(OS_MACOSX) || defined(__APPLE__))
uint64_t htonll(uint64_t val) { return htobe64(val); }

uint64_t ntohll(uint64_t val) { return be64toh(val); }
#endif

namespace chord {

void netbuf_init(NetBuffer *net_buf, void *in_buf, size_t len) {
    net_buf->buf      = (uint8_t *)in_buf;
    net_buf->len      = len;
    net_buf->position = 0;
}

bool netbuf_seek_offset(NetBuffer *net_buf, size_t offset) {
    if (net_buf->position + offset <= net_buf->len) {
        net_buf->position += offset;
        return true;
    }
    return false;
}

bool netbuf_seek(NetBuffer *net_buf, size_t position) {
    if (position <= net_buf->len) {
        net_buf->position = position;
        return true;
    }
    return false;
}

void netbuf_seek_start(NetBuffer *net_buf) { net_buf->position = 0; }

void netbuf_seek_end(NetBuffer *net_buf) { net_buf->position = net_buf->len - 1; }

bool netbuf_at_end(NetBuffer *net_buf) { return (net_buf->position >= net_buf->len); }

bool push_uint8(NetBuffer *net_buf, uint8_t val) {
    if (net_buf->position + 1 <= net_buf->len) {
        memcpy(net_buf->buf + net_buf->position, &val, 1);
        net_buf->position += 1;
        return true;
    }
    return false;
}

bool push_uint16(NetBuffer *net_buf, uint16_t val) {
    if (net_buf->position + 2 <= net_buf->len) {
        uint16_t nbyte_order = htons(val);
        memcpy(net_buf->buf + net_buf->position, &nbyte_order, 2);
        net_buf->position += 2;
        return true;
    }
    return false;
}

bool push_uint32(NetBuffer *net_buf, uint32_t val) {
    if (net_buf->position + 4 <= net_buf->len) {
        uint32_t nbyte_order = htonl(val);
        memcpy(net_buf->buf + net_buf->position, &nbyte_order, 4);
        net_buf->position += 4;
        return true;
    }
    return false;
}

bool push_uint64(NetBuffer *net_buf, uint64_t val) {
    if (net_buf->position + 8 <= net_buf->len) {
        uint64_t nbyte_order = htonll(val);
        memcpy(net_buf->buf + net_buf->position, &nbyte_order, 8);
        net_buf->position += 8;
        return true;
    }
    return false;
}

bool push_generic_uint(NetBuffer *net_buf, uint64_t val, size_t uint_bytes) {
    if (uint_bytes > 8) {
        return false;
    }

    if (net_buf->position + uint_bytes <= net_buf->len) {
        uint64_t nbyte_order = htonll(val);
        memcpy(net_buf->buf + net_buf->position, (((uint8_t *)(&nbyte_order)) + (8 - uint_bytes)), uint_bytes);
        net_buf->position += uint_bytes;
        return true;
    }
    return false;
}

bool push_data(NetBuffer *net_buf, const void *data, size_t data_len) {
    if (data && net_buf->position + data_len <= net_buf->len) {
        memcpy(net_buf->buf + net_buf->position, data, data_len);
        net_buf->position += data_len;
        return true;
    }
    return false;
}

bool read_uint8(NetBuffer *net_buf, uint8_t *out) {
    if (net_buf->position + 1 <= net_buf->len) {
        memcpy(out, net_buf->buf + net_buf->position, 1);
        net_buf->position += 1;
    }
    return false;
}

bool read_uint16(NetBuffer *net_buf, uint16_t *out) {
    if (net_buf->position + 2 <= net_buf->len) {
        uint16_t nbyte_order;
        memcpy(&nbyte_order, net_buf->buf + net_buf->position, 2);
        *out = ntohs(nbyte_order);
        net_buf->position += 2;
    }
    return false;
}

bool read_uint32(NetBuffer *net_buf, uint32_t *out) {
    if (net_buf->position + 4 <= net_buf->len) {
        uint32_t nbyte_order;
        memcpy(&nbyte_order, net_buf->buf + net_buf->position, 4);
        *out = ntohl(nbyte_order);
        net_buf->position += 4;
    }
    return false;
}

bool read_uint64(NetBuffer *net_buf, uint64_t *out) {
    if (net_buf->position + 8 <= net_buf->len) {
        uint64_t nbyte_order;
        memcpy(&nbyte_order, net_buf->buf + net_buf->position, 8);
        *out = ntohll(nbyte_order);
        net_buf->position += 8;
    }
    return false;
}

bool read_generic_uint(NetBuffer *net_buf, uint64_t *out, size_t uint_bytes) {
    if (net_buf->position + uint_bytes <= net_buf->len) {
        uint64_t nbyte_order = 0;
        memcpy(&nbyte_order, net_buf->buf + net_buf->position, uint_bytes);
        nbyte_order <<= 64 - (8 * uint_bytes);
        *out = ntohll(nbyte_order);
        net_buf->position += uint_bytes;
    }
    return false;
}

bool read_data(NetBuffer *net_buf, void *data_out, size_t data_len) {
    if (net_buf->position + data_len <= net_buf->len) {
        memcpy(data_out, net_buf->buf + net_buf->position, data_len);
        net_buf->position += data_len;
        return true;
    }
    return false;
}

bool read_string_data(NetBuffer *net_buf, void *string_out, size_t raw_string_len) {
    bool ret                             = read_data(net_buf, string_out, raw_string_len);
    ((char *)string_out)[raw_string_len] = '\0'; /* Add the null terminator to the raw string */
    return ret;
}
}  // namespace chord
