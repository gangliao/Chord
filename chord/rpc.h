#pragma once

#include "node.h"

namespace chord {
void rpc_daemon(int32_t server_sockfd, chord::Node* node);

bool rpc_send_check_predecessor(int32_t peer_sockfd);
void rpc_recv_check_predecessor(int32_t peer_sockfd);

bool rpc_send_find_successor(int32_t peer_sockfd, chord::Node* node);
void rpc_recv_find_successor(int32_t peer_sockfd, const protocol::FindSuccessorArgs& args, chord::Node* node);

bool rpc_send_get_predecessor(int32_t peer_sockfd, chord::Node* node);
void rpc_recv_get_predecessor(int32_t peer_sockfd, chord::Node* node);

bool rpc_send_notify(int32_t peer_sockfd, chord::Node* node);
void rpc_recv_notify(int32_t peer_sockfd, const protocol::NotifyArgs& args, chord::Node* node);

}  // namespace chord