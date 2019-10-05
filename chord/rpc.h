#pragma once

#include "node.h"

namespace chord {
void rpc_daemon(int32_t server_sockfd, chord::Node* node);
bool rpc_send_find_successor(int32_t peer_sockfd, chord::Node* node);
void rpc_recv_find_successor(const protocol::Call& call, chord::Node* node);
}  // namespace chord