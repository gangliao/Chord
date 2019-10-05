#include "rpc.h"
#include "chord.h"
#include "common/socket-util.h"

namespace chord {
bool rpc_join(int32_t peer_sockfd, chord::Node* node) {
    protocol::FindSuccessorArgs args;
    args.set_id(node->getId(), SHA_DIGEST_LENGTH);
    std::string packed_args;
    CHECK_EQ(args.SerializeToString(&packed_args), true);

    protocol::Call call;
    call.set_name("find_successor");
    call.set_args(packed_args);
    CHECK_EQ(call.SerializeToString(&packed_args), true);

    if (send_exact(peer_sockfd, (void*)packed_args.c_str(), packed_args.size(), 0) <= 0) {
        close(peer_sockfd);
        return false;
    }
    return true;
}
}  // namespace chord