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

bool send_proto(int32_t peer_sockfd, std::string& binary) {
    uint64_t packed_size = binary.size() + sizeof(uint64_t);
    uint8_t* output      = (uint8_t*)malloc(packed_size);
    memset(output, 0, packed_size);
    *(reinterpret_cast<uint64_t*>(output)) = htonll(packed_size);
    memcpy((uint8_t*)output + sizeof(uint64_t), binary.c_str(), binary.size());

    if (send_exact(peer_sockfd, (void*)output, packed_size, 0) <= 0) {
        close(peer_sockfd);
        LOG(WARNING) << "Failed to send back";
        return false;
    }
    free(output);
    return true;
}

uint64_t recv_proto(int32_t peer_sockfd, uint8_t* recv_buf) {
    recv_buf = (uint8_t*)malloc(sizeof(uint64_t));
    NetBuffer net_buf;
    netbuf_init(&net_buf, recv_buf, sizeof(uint64_t));
    if (recv_exact(peer_sockfd, recv_buf, sizeof(uint64_t), 0) != sizeof(uint64_t)) {
        LOG(ERROR) << "Invalid hash request header";
    }

    uint64_t size = 0;
    read_uint64(&net_buf, &size);
    free(recv_buf);
    uint64_t rest = size - sizeof(uint64_t);
    recv_buf      = (uint8_t*)malloc(rest);
    if (recv_exact(peer_sockfd, recv_buf, rest, 0) != rest) {
        LOG(ERROR) << "Invalid hash request args";
    }
    return rest;
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

    send_proto(peer_sockfd, packed_args);

    uint8_t* proto_buff = nullptr;
    uint64_t proto_size = recv_proto(peer_sockfd, proto_buff);

    protocol::FindSuccessorRet ret;
    CHECK_EQ(ret.ParseFromArray(proto_buff, proto_size), true);
    CHECK_EQ(ret.has_node(), true);
    chord::Node* succ = new chord::Node(ret.node());
    node->successor   = succ;

    free(proto_buff);
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

    send_proto(peer_sockfd, packed_args);
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
            uint8_t* proto_buff = nullptr;
            uint64_t proto_size = recv_proto(client_sockfd, proto_buff);

            protocol::Call call;
            CHECK_EQ(call.ParseFromArray(proto_buff, proto_size), true);

            if (call.name() == kFindSuccessor) {
                std::string binary = call.args();
                protocol::FindSuccessorArgs args;
                CHECK_EQ(args.ParseFromString(binary), true);
                rpc_recv_find_successor(client_sockfd, args, node);
            } else if (call.name() == kNotify) {
            } else if (call.name() == kGetPredecessor) {
            } else if (call.name() == kGetSuccessorList) {
            }
            free(proto_buff);
        }
    }
    close(client_sockfd);
}

}  // namespace chord