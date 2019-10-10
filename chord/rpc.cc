#include "rpc.h"
#include "chord.h"
#include "common/net-buffer.h"
#include "common/socket-util.h"
#include "common/thread_pool.h"

namespace chord {

namespace {
const std::string kFindSuccessor    = "find_successor";
const std::string kNotify           = "notify";
const std::string kGetPredecessor   = "get_predecessor";
const std::string kCheckPredecessor = "check_predecessor";
const std::string kGetSuccessorList = "get_successor_list";

const int32_t kPoolSize = 32;

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

uint64_t recv_proto(int32_t peer_sockfd, uint8_t** recv_buf) {
    *recv_buf = (uint8_t*)malloc(sizeof(uint64_t));
    NetBuffer net_buf;
    netbuf_init(&net_buf, *recv_buf, sizeof(uint64_t));
    if (recv_exact(peer_sockfd, *recv_buf, sizeof(uint64_t), 0) != sizeof(uint64_t)) {
        LOG(ERROR) << "Invalid hash request header";
    }

    uint64_t size = 0;
    read_uint64(&net_buf, &size);
    free(*recv_buf);
    uint64_t rest = size - sizeof(uint64_t);
    *recv_buf     = (uint8_t*)malloc(rest);
    if (recv_exact(peer_sockfd, *recv_buf, rest, 0) != rest) {
        LOG(ERROR) << "Invalid hash request args";
    }
    return rest;
}

}  // namespace

// rpc_join is a blocking request
bool rpc_send_find_successor(int32_t peer_sockfd, chord::Node* node) {
    protocol::FindSuccessorArgs args;
    std::string s(node->getId(), node->getId() + SHA_DIGEST_LENGTH);
    args.set_id(s);
    std::string packed_args;
    CHECK_EQ(args.SerializeToString(&packed_args), true);

    protocol::Call call;
    call.set_name(kFindSuccessor);
    call.set_args(packed_args);
    CHECK_EQ(call.SerializeToString(&packed_args), true);

    send_proto(peer_sockfd, packed_args);

    uint8_t* proto_buff;
    uint64_t proto_size = recv_proto(peer_sockfd, &proto_buff);

    protocol::Return ret;
    CHECK_EQ(ret.ParseFromArray(proto_buff, proto_size), true);
    CHECK_EQ(ret.success(), true);
    protocol::FindSuccessorRet fsret;
    CHECK_EQ(fsret.ParseFromString(ret.value()), true);
    CHECK_EQ(fsret.has_node(), true);

    node->successor = new protocol::Node(fsret.node());
    // *(node->successor) = fsret.node();

    free(proto_buff);
    return true;
}

void rpc_recv_find_successor(int32_t peer_sockfd, const protocol::FindSuccessorArgs& args, chord::Node* node) {
    CHECK_EQ(args.has_id(), true);
    chord::Node* succ = node->findSuccessor((const uint8_t*)args.id().c_str());

    protocol::Node* n = new protocol::Node();
    n->set_address(succ->getAddr());
    n->set_port(succ->getPort());
    std::string s(succ->getId(), succ->getId() + SHA_DIGEST_LENGTH);
    n->set_id(s);

    std::string packed_args;
    protocol::FindSuccessorRet fsret;
    fsret.set_allocated_node(n);
    CHECK_EQ(fsret.SerializeToString(&packed_args), true);

    protocol::Return ret;
    ret.set_success(true);
    ret.set_value(packed_args);
    CHECK_EQ(ret.SerializeToString(&packed_args), true);

    send_proto(peer_sockfd, packed_args);
}

bool rpc_send_get_predecessor(int32_t peer_sockfd, chord::Node* node) {
    std::string packed_args;
    protocol::Call call;
    protocol::GetPredecessorArgs args;
    call.set_name(kGetPredecessor);
    args.SerializeToString(&packed_args);
    call.set_args(packed_args);
    CHECK_EQ(call.SerializeToString(&packed_args), true);

    send_proto(peer_sockfd, packed_args);

    uint8_t* proto_buff;
    uint64_t proto_size = recv_proto(peer_sockfd, &proto_buff);

    protocol::Return ret;
    CHECK_EQ(ret.ParseFromArray(proto_buff, proto_size), true);
    CHECK_EQ(ret.success(), true);

    protocol::GetPredecessorRet gpret;
    CHECK_EQ(gpret.ParseFromString(ret.value()), true);
    if (gpret.has_node() && gpret.node().has_id()) {
        node->predecessor = new protocol::Node(gpret.node());
    } else {
        node->predecessor = nullptr;
    }

    free(proto_buff);
    return true;
}

void rpc_recv_get_predecessor(int32_t peer_sockfd, chord::Node* node) {
    std::string packed_args;
    protocol::GetPredecessorRet gpret;

    if (node->predecessor != nullptr && node->predecessor->has_id()) {
        protocol::Node* pred = new protocol::Node(*node->predecessor);
        gpret.set_allocated_node(pred);
    }
    CHECK_EQ(gpret.SerializeToString(&packed_args), true);

    protocol::Return ret;
    ret.set_success(true);
    ret.set_value(packed_args);
    CHECK_EQ(ret.SerializeToString(&packed_args), true);

    send_proto(peer_sockfd, packed_args);
}

bool rpc_send_notify(int32_t peer_sockfd, chord::Node* node) {
    std::string packed_args;

    protocol::Node* n = new protocol::Node();
    n->set_address(node->getAddr());
    n->set_port(node->getPort());
    std::string s(node->getId(), node->getId() + SHA_DIGEST_LENGTH);
    n->set_id(s);

    protocol::NotifyArgs args;
    args.set_allocated_node(n);
    CHECK_EQ(args.SerializeToString(&packed_args), true);

    protocol::Call call;
    call.set_name(kNotify);
    call.set_args(packed_args);
    CHECK_EQ(call.SerializeToString(&packed_args), true);

    send_proto(peer_sockfd, packed_args);

    uint8_t* proto_buff;
    uint64_t proto_size = recv_proto(peer_sockfd, &proto_buff);

    protocol::Return ret;
    CHECK_EQ(ret.ParseFromArray(proto_buff, proto_size), true);
    CHECK_EQ(ret.success(), true);

    free(proto_buff);
    return true;
}

void rpc_recv_notify(int32_t peer_sockfd, const protocol::NotifyArgs& args, chord::Node* node) {
    protocol::Node n = args.node();
    if (node->predecessor == nullptr || !node->predecessor->has_id() ||
        within(n.id().c_str(), node->predecessor->id().c_str(), node->getId())) {
        node->predecessor = new protocol::Node(n);
    }

    std::string packed_args;
    std::shared_ptr<protocol::Return> ret(new protocol::Return());
    ret->set_success(true);
    CHECK_EQ(ret->SerializeToString(&packed_args), true);
    send_proto(peer_sockfd, packed_args);
}

bool rpc_send_check_predecessor(int32_t peer_sockfd) {
    std::string packed_args;

    protocol::CheckPredecessorArgs args;
    CHECK_EQ(args.SerializeToString(&packed_args), true);

    protocol::Call call;
    call.set_name(kCheckPredecessor);
    call.set_args(packed_args);
    CHECK_EQ(call.SerializeToString(&packed_args), true);

    send_proto(peer_sockfd, packed_args);

    uint8_t* proto_buff;
    uint64_t proto_size = recv_proto(peer_sockfd, &proto_buff);

    protocol::Return ret;
    CHECK_EQ(ret.ParseFromArray(proto_buff, proto_size), true);
    CHECK_EQ(ret.success(), true);

    free(proto_buff);
    return true;
}

void rpc_recv_check_predecessor(int32_t peer_sockfd) {
    std::string packed_args;
    std::shared_ptr<protocol::Return> ret(new protocol::Return());
    ret->set_success(true);
    CHECK_EQ(ret->SerializeToString(&packed_args), true);
    send_proto(peer_sockfd, packed_args);
}

void rpc_daemon(int32_t server_sockfd, chord::Node* node) {
    int32_t client_sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_len;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server_sockfd, &readfds);

    threadpool pool(kPoolSize);
    size_t max_sd = server_sockfd;
    while (1) {
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            LOG(WARNING) << "select() failed";
        }
        if (FD_ISSET(server_sockfd, &readfds)) {
            client_len = sizeof(client_addr);
            client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);
            if (client_sockfd < 0) {
                continue;
            } else {
                LOG(INFO) << "Recieved connection from " << inet_ntoa(client_addr.sin_addr);
                uint8_t* proto_buff;
                uint64_t proto_size = recv_proto(client_sockfd, &proto_buff);
                protocol::Call call;
                CHECK_EQ(call.ParseFromArray(proto_buff, proto_size), true);

                if (call.name() == kFindSuccessor) {
                    std::string binary = call.args();
                    protocol::FindSuccessorArgs args;
                    CHECK_EQ(args.ParseFromString(binary), true);
                    pool.AddTask([&] { rpc_recv_find_successor(client_sockfd, args, node); });
                } else if (call.name() == kNotify) {
                    std::string binary = call.args();
                    protocol::NotifyArgs args;
                    CHECK_EQ(args.ParseFromString(binary), true);
                    rpc_recv_notify(client_sockfd, args, node);
                } else if (call.name() == kGetPredecessor) {
                    pool.AddTask([&] { rpc_recv_get_predecessor(client_sockfd, node); });
                } else if (call.name() == kGetSuccessorList) {
                } else if (call.name() == kCheckPredecessor) {
                    pool.AddTask([&] { rpc_recv_check_predecessor(client_sockfd); });
                }
                free(proto_buff);
            }
        }
    }
    close(client_sockfd);
}

}  // namespace chord
