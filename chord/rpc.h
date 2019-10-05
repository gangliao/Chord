#pragma once

#include "node.h"

namespace chord {
bool rpc_join(int32_t peer_sockfd, chord::Node* node);
}