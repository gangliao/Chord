#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef MSG_NOSIGNAL
// linuxism here.
#define MSG_NOSIGNAL 0
#endif

namespace chord {

/**
 *  Will continue receiving bytes until exactly len bytes are received.
 *  Return value will be the number of bytes received on SUCCESS.
 *  Return value will be 0 if the socket is closed.
 *  Return value will be -1 on ERROR.
 *  If the socket is closed or there is an error the buffer
 *  is left in a undefined state.
 */
ssize_t recv_exact(int sockfd, void *buf, size_t len, int flags);

/**
 *  Return value will be the number of bytes sent on SUCCESS.
 *  Return value will be 0 if the socket is closed.
 *  Return value will be -1 on ERROR.
 */
ssize_t send_exact(int sockfd, void *buf, size_t len, int flags);

/**
 * Returns the size read/sent on SUCCESS
 * Returns -1 if an error occurred sending the data
 * Returns -2 if an error occurred reading the file
 */
#define SEND_FILE_CHUNK_SIZE 4096
ssize_t send_file(int sockfd, FILE *data_file, size_t len, int flags);

/* Read the size of the bytes buffered that could be read with recv() */

/**
 * Read the size of the bytes buffered that could be read with recv().
 * Should only be used for debugging purposes.
 * @param  sockfd : sockfd to check on
 * @return        : the number of bytes buffered
 */
size_t buffered_bytes(int sockfd);

/**
 * Check if a socket fd is still online, or if it has been disconnected.
 * Will report the connection as connected if there is still data to be
 * read from tthe socket, but will detect its current status otherwise.
 * @param  sockfd [description]
 * @return        [description]
 */
bool socket_connected(int sockfd);

#endif /* SOCKET_UTIL_H */
}