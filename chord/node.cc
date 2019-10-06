#include "node.h"
#include "common/bigint.h"
#include "common/net-buffer.h"
#include "common/socket-util.h"
#include "rpc.h"

#include <thread>

namespace chord {

inline void print_hash(const uint8_t* hash, uint16_t size) {
    for (int i = 0; i < size; i++) {
        printf("%02x", hash[i]);
    }
    /* Print newline */
    puts("");
}

void Node::create() {
    predecessor = nullptr;
    successor->set_address(this->getAddr());
    successor->set_port(this->getPort());
    successor->set_id(this->getId(), SHA_DIGEST_LENGTH);
}

void Node::join() {
    predecessor = nullptr;

    int32_t peer_sockfd;
    CHECK_GE(peer_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), 0) << "Failed to create socket";

    if (connect(peer_sockfd, (struct sockaddr*)&join_address, sizeof(join_address)) < 0) {
        close(peer_sockfd);
        LOG(FATAL) << "Failed to connect to server";
    }

    CHECK_EQ(rpc_send_find_successor(peer_sockfd, this), true) << "Failed to join a Chord ring";
    close(peer_sockfd);
}

void Node::lookup(std::string key) {
    // key and its hash value
    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1((const uint8_t*)key.c_str(), key.size(), hash);
    std::cout << "< " + key + " ";
    print_hash(hash, SHA_DIGEST_LENGTH);

    // The successor client's node information
    Node* succ = this->findSuccessor(hash);
    std::cout << "< ";
    print_hash(succ->getId(), SHA_DIGEST_LENGTH);
    std::cout << " " + succ->getAddr() + " " + std::to_string(succ->getPort());
}

void Node::dump() {
    // The Chord client's own node information
    std::cout << "< Self ";
    print_hash(this->getId(), SHA_DIGEST_LENGTH);
    std::cout << " " + this->getAddr() + " " + std::to_string(this->getPort());

    // The node information for all nodes in the successor list

    // The node information for all nodes in the finger table
}

void Node::rpc_server() {
    server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_GE(server_sockfd, 0) << "Failed to open socket";
    CHECK_GE(bind(server_sockfd, (struct sockaddr*)&address, sizeof(address)), 0) << "Failed to bind to port";
    CHECK_GE(listen(server_sockfd, MAX_TCP_CONNECTIONS), 0) << "Listen failed";
    CHECK_GE(fcntl(server_sockfd, F_SETFL, fcntl(server_sockfd, F_GETFL, 0) | O_NONBLOCK), 0)
        << "Failed to set listen socket to non-blocking";
    std::thread thx(rpc_daemon, server_sockfd, this);
    thx.detach();
}

void Node::notify() {
    int32_t peer_sockfd;
    CHECK_GE(peer_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), 0) << "Failed to create socket";

    chord::Node* n = new chord::Node(*successor);

    if (connect(peer_sockfd, (struct sockaddr*)&n->address, sizeof(n->address)) < 0) {
        close(peer_sockfd);
        LOG(FATAL) << "Failed to connect to server";
    }

    CHECK_EQ(rpc_send_notify(peer_sockfd, this), true) << "Failed to join a get predecessor";
    close(peer_sockfd);
}

protocol::Node* get_predecessor(const protocol::Node& node) {
    int32_t peer_sockfd;
    CHECK_GE(peer_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), 0) << "Failed to create socket";

    chord::Node* n = new chord::Node(node);

    if (connect(peer_sockfd, (struct sockaddr*)&n->address, sizeof(n->address)) < 0) {
        close(peer_sockfd);
        LOG(FATAL) << "Failed to connect to server";
    }

    CHECK_EQ(rpc_send_get_predecessor(peer_sockfd, n), true) << "Failed to join a get predecessor";
    close(peer_sockfd);

    return n->predecessor;
}

void Node::stabilize() {
    auto pred = get_predecessor(*successor);
    if (within(pred->id().c_str(), this->getId(), successor->id().c_str())) {
        successor = pred;
    }
    notify();
}

void Node::fixFingers() {
    static size_t next;
    next = next + 1;
    if (next > SHA_DIGEST_LENGTH) {
        next = 1;
    }

    uint8_t t[SHA_DIGEST_LENGTH];
    pow2((next - 1), t);
    add(this->id, t);
    finger_table[next] = findSuccessor(t);
}

void Node::checkPredecessor() {
    int32_t pred_sockfd;
    CHECK_GE(pred_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), 0) << "Failed to create socket";
    auto pred = new chord::Node(*predecessor);

    if (connect(pred_sockfd, (struct sockaddr*)&(pred->address), sizeof(pred->address)) < 0) {
        close(pred_sockfd);
        LOG(WARNING) << "Predecessor has failed";
        predecessor = nullptr;
    }
}

Node* Node::findSuccessor(const uint8_t* id) {
    if (within(id, this->getId(), successor->id().c_str())) {
        return new chord::Node(*successor);
    } else {
        auto node = closetPrecedingNode(id);
        return node->findSuccessor(id);
    }
}

Node* Node::closetPrecedingNode(const uint8_t* id) {
    for (int i = finger_table.size() - 1; i >= 0; i--) {
        if (within(finger_table[i]->getId(), this->getId(), id)) {
            return finger_table[i];
        }
    }
    return this;
}

}  // namespace chord
