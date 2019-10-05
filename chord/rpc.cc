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

    size_t packed_size = packed_args.size();
    packed_size = packed_size + sizeof(packed_size);
    
    if (send_exact(peer_sockfd, (void*)packed_args.c_str(), packed_args.size(), 0) <= 0) {
        close(peer_sockfd);
        return false;
    }

    // size_t val = 0;
    // if ((val = recv_exact(sockfd, res_buf, 40, 0)) != 40) {
    //     info_log("Recv %d", val);
    //     warn_log("Failed to receive hash response");
    //     return false;
    // }
    return true;
}
}  // namespace chord