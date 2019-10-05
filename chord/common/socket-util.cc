#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "socket-util.h"

namespace chord {

ssize_t recv_exact(int sockfd, void *buf, size_t len, int flags) {
    size_t total_received = 0;
    ssize_t curr_received;
    while (total_received < len) {
        if ((curr_received = recv(sockfd, buf + total_received, len - total_received, flags)) <= 0) {
            return curr_received;
        } else {
            total_received += curr_received;
        }
    }
    return total_received;
}

ssize_t send_exact(int sockfd, void *buf, size_t len, int flags) {
    size_t total_sent = 0;
    ssize_t curr_sent;
    while (total_sent < len) {
        if ((curr_sent = send(sockfd, buf + total_sent, len - total_sent, flags)) <= 0) {
            return curr_sent;
        } else {
            total_sent += curr_sent;
        }
    }
    return total_sent;
}

ssize_t send_file(int sockfd, FILE *data_file, size_t len, int flags) {
    uint8_t data_buf[SEND_FILE_CHUNK_SIZE];

    size_t total_read = 0;
    size_t curr_read;
    size_t next_read;
    while (total_read < len) {
        next_read = (len - total_read < SEND_FILE_CHUNK_SIZE) ? (len - total_read) : SEND_FILE_CHUNK_SIZE;
        curr_read = fread(data_buf, 1, next_read, data_file);
        if (curr_read == 0 || feof(data_file) || ferror(data_file)) {
            return -2;
        } else {
            total_read += curr_read;
            send_exact(sockfd, data_buf, curr_read, flags);
        }
    }
    return total_read;
}

size_t buffered_bytes(int sockfd) {
    size_t bytes_buffered = 0;
    ioctl(sockfd, FIONREAD, &bytes_buffered);
    return bytes_buffered;
}

bool socket_online(int sockfd) {
    /* Check for socket disconnects */
    uint8_t test_recv;
    ssize_t recv_status = recv(sockfd, &test_recv, 1, MSG_NOSIGNAL | MSG_PEEK | MSG_DONTWAIT);

    /* The EAGAIN or EWOULDBLOCK errors are allowed,  but others should remove the fd */
    bool disconnected = (recv_status == 0 || (recv_status == -1 && !(errno == EAGAIN || errno == EWOULDBLOCK)));
    return !disconnected;
}

}  // namespace chord