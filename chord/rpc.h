#pragma once

#include "node.h"

namespace chord {
void rpc_daemon(int32_t server_sockfd, chord::Node* node);
bool rpc_send_join(int32_t peer_sockfd, chord::Node* node);
void rpc_recv_join(const protocol::Call& call, chord::Node* node);
}  // namespace chord