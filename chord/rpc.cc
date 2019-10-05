#include "rpc.h"
#include "chord.h"
#include "common/net-buffer.h"
#include "common/socket-util.h"

namespace chord {

namespace {
const std::string kFindSuccessor    = "find_successor";
const std::string kNotify           = "notify";
const std::string kGetPredecessor   = "get_predecessor";
const std::string kGetSuccessorList = "get_successor_list";
}  // namespace

// rpc_join is a blocking request
bool rpc_send_find_successor(int32_t peer_sockfd, chord::Node* node) {
    protocol::FindSuccessorArgs args;
    args.set_id(node->getId(), SHA_DIGEST_LENGTH);
    std::string packed_args;
    CHECK_EQ(args.SerializeToString(&packed_args), true);

    protocol::Call call;
    call.set_name(kFindSuccessor);
    call.set_args(packed_args);
    CHECK_EQ(call.SerializeToString(&packed_args), true);

    uint64_t packed_size = packed_args.size() + sizeof(uint64_t);
    uint8_t* buffer      = (uint8_t*)malloc(packed_size);
    memset(buffer, 0, packed_size);
    *(reinterpret_cast<uint64_t*>(buffer)) = htonll(packed_size);
    memcpy((uint8_t*)buffer + sizeof(uint64_t), packed_args.c_str(), packed_args.size());

    if (send_exact(peer_sockfd, (void*)buffer, packed_size, 0) <= 0) {
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

void rpc_recv_find_successor(const protocol::Call& call, chord::Node* node) {
    std::string binary = call.args();
    protocol::FindSuccessorArgs args;
    CHECK_EQ(args.ParseFromString(binary), true);
    CHECK_EQ(args.has_id(), true);
    // node->findSuccessor((const uint8_t*)args.id().c_str());
}

void rpc_daemon(int32_t server_sockfd, chord::Node* node) {
    int32_t client_sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    while (1) {
        client_len = sizeof(client_addr);
        if ((client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len)) < 0) {
            LOG(WARNING) << "accept() failed, moving on to next client";
        } else {
            LOG(INFO) << "Recieved connection from " << inet_ntoa(client_addr.sin_addr);
            uint8_t* recv_buf = (uint8_t*)malloc(sizeof(uint64_t));
            NetBuffer net_buf;
            netbuf_init(&net_buf, recv_buf, sizeof(uint64_t));
            if (recv_exact(client_sockfd, recv_buf, sizeof(uint64_t), 0) != sizeof(uint64_t)) {
                LOG(ERROR) << "Invalid hash request header";
            }

            uint64_t size = 0;
            read_uint64(&net_buf, &size);
            uint64_t rest = size - sizeof(uint64_t);
            recv_buf      = (uint8_t*)malloc(rest);
            if (recv_exact(client_sockfd, recv_buf, rest, 0) != rest) {
                LOG(ERROR) << "Invalid hash request header";
            }
            protocol::Call call;
            CHECK_EQ(call.ParseFromArray(recv_buf, rest), true);
            free(recv_buf);

            if (call.name() == kFindSuccessor) {
                rpc_recv_find_successor(call, node);
            } else if (call.name() == kNotify) {
            } else if (call.name() == kGetPredecessor) {
            } else if (call.name() == kGetSuccessorList) {
            }
        }
    }
    close(client_sockfd);
}

}  // namespace chord