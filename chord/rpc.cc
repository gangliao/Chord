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

uint64_t padding_size(std::string binary, uint8_t* output) {
    uint64_t packed_size = binary.size() + sizeof(uint64_t);
    output               = (uint8_t*)malloc(packed_size);
    memset(output, 0, packed_size);
    *(reinterpret_cast<uint64_t*>(output)) = htonll(packed_size);
    memcpy((uint8_t*)output + sizeof(uint64_t), binary.c_str(), binary.size());
    return packed_size;
}
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

    uint8_t* buffer = nullptr;
    uint64_t packed_size = padding_size(packed_args, buffer);
    if (send_exact(peer_sockfd, (void*)buffer, packed_size, 0) <= 0) {
        close(peer_sockfd);
        return false;
    }
    free(buffer);
    // size_t val = 0;
    // if ((val = recv_exact(sockfd, res_buf, 40, 0)) != 40) {
    //     info_log("Recv %d", val);
    //     warn_log("Failed to receive hash response");
    //     return false;
    // }
    return true;
}

void rpc_recv_find_successor(int32_t peer_sockfd, const protocol::FindSuccessorArgs& args, chord::Node* node) {
    CHECK_EQ(args.has_id(), true);
    chord::Node* succ = node->findSuccessor((const uint8_t*)args.id().c_str());

    protocol::Node n;
    n.set_id(succ->getId(), SHA_DIGEST_LENGTH);
    n.set_address(succ->getAddr());
    n.set_port(succ->getPort());

    std::string packed_args;
    protocol::FindSuccessorRet ret;
    ret.set_allocated_node(&n);
    CHECK_EQ(ret.SerializeToString(&packed_args), true);

    uint8_t* buffer = nullptr;
    uint64_t packed_size = padding_size(packed_args, buffer);
    if (send_exact(peer_sockfd, (void*)buffer, packed_size, 0) <= 0) {
        close(peer_sockfd);
        LOG(WARNING) << "Failed to send FindSuccessorRet";
    }
    free(buffer);
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
            free(recv_buf);
            uint64_t rest = size - sizeof(uint64_t);
            recv_buf      = (uint8_t*)malloc(rest);
            if (recv_exact(client_sockfd, recv_buf, rest, 0) != rest) {
                LOG(ERROR) << "Invalid hash request header";
            }
            protocol::Call call;
            CHECK_EQ(call.ParseFromArray(recv_buf, rest), true);
            free(recv_buf);

            if (call.name() == kFindSuccessor) {
                std::string binary = call.args();
                protocol::FindSuccessorArgs args;
                CHECK_EQ(args.ParseFromString(binary), true);
                rpc_recv_find_successor(client_sockfd, args, node);
            } else if (call.name() == kNotify) {
            } else if (call.name() == kGetPredecessor) {
            } else if (call.name() == kGetSuccessorList) {
            }
        }
    }
    close(client_sockfd);
}

}  // namespace chord