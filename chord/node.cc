#include "node.h"
#include "common/bigint.h"
#include "common/net-buffer.h"
#include "common/socket-util.h"
#include "rpc.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace chord {

inline std::string hash2string(const uint8_t* hash, uint16_t size) {
    std::stringstream buffer;
    for (int i = 0; i < size; i++) {
        buffer << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];
    }
    return buffer.str();
}

Node::Node() { id = new uint8_t[SHA_DIGEST_LENGTH]; }

Node::Node(const protocol::Node& node) {
    id = new uint8_t[SHA_DIGEST_LENGTH];
    memcpy(id, node.id().c_str(), SHA_DIGEST_LENGTH);
    addr = node.address();
    CHECK_GE(inet_pton(AF_INET, addr.c_str(), &address.sin_addr.s_addr), 1) << "Invalid IPv4 address";
    address.sin_family = AF_INET;
    port               = node.port();
    address.sin_port   = htons(port);
}

void Node::create() {
    predecessor = nullptr;
    successor   = new protocol::Node();
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
    std::cout << hash2string(hash, SHA_DIGEST_LENGTH);
    puts("");

    // The successor client's node information
    Node* succ = this->findSuccessor(hash);
    std::cout << "< ";
    std::cout << hash2string(succ->getId(), SHA_DIGEST_LENGTH);
    std::cout << " " + succ->getAddr() + " " + std::to_string(succ->getPort());
    puts("");
}

void Node::dump() {
    // The Chord client's own node information
    std::cout << "< Self " << hash2string(this->getId(), SHA_DIGEST_LENGTH);
    std::cout << " " + this->getAddr() + " " + std::to_string(this->getPort());
    puts("");

    // The node information for all nodes in the successor list
    std::cout << "< Successor [1] " << hash2string((const uint8_t*)this->successor->id().c_str(), SHA_DIGEST_LENGTH);
    std::cout << " " + this->successor->address() + " " + std::to_string(this->successor->port());
    puts("");

    // The node information for all nodes in the finger table
    for (int i = 0; i < finger_table.size(); ++i) {
        std::cout << "< Finger [" << i + 1 << "] " << hash2string(finger_table[i]->getId(), SHA_DIGEST_LENGTH);
        std::cout << " " + finger_table[i]->getAddr() + " " + std::to_string(finger_table[i]->getPort());
        puts("");
    }
}

void Node::rpc_server() {
    int opt       = 1;
    server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_GE(server_sockfd, 0) << "Failed to open socket";
    CHECK_GE(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)), 0);
    CHECK_GE(bind(server_sockfd, (struct sockaddr*)&address, sizeof(address)), 0) << "Failed to bind to port";
    CHECK_GE(listen(server_sockfd, MAX_TCP_CONNECTIONS), 0) << "Listen failed";
    // CHECK_GE(fcntl(server_sockfd, F_SETFL, fcntl(server_sockfd, F_GETFL, 0) | O_NONBLOCK), 0)
    //     << "Failed to set listen socket to non-blocking";
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
    LOG(INFO) << "[stabilize] called periodically.";
    auto pred = get_predecessor(*successor);
    if (pred != nullptr &&
        within((const uint8_t*)pred->id().c_str(), this->getId(), (const uint8_t*)successor->id().c_str())) {
        successor = pred;
    }
    notify();
}

void Node::initFingers() {
    for (int i = 1; i <= SHA_DIGEST_LENGTH * 8; ++i) {
        uint8_t t[SHA_DIGEST_LENGTH];
        pow2((i - 1), t);
        add(this->id, t);
        finger_table.push_back(findSuccessor(t));
    }
}

void Node::fixFingers() {
    LOG(INFO) << "[fix fingers] called periodically.";
    static size_t next;
    next = next + 1;
    if (next > SHA_DIGEST_LENGTH * 8) {
        next = 1;
    }

    uint8_t t[SHA_DIGEST_LENGTH];
    pow2((next - 1), t);
    add(this->id, t);
    finger_table[next] = findSuccessor(t);
}

void Node::checkPredecessor() {
    LOG(INFO) << "[checkPredecessor] called periodically.";
    if (predecessor != nullptr && predecessor->has_id()) {
        int32_t pred_sockfd;
        CHECK_GE(pred_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), 0) << "Failed to create socket";
        auto pred = new chord::Node(*predecessor);

        if (connect(pred_sockfd, (struct sockaddr*)&(pred->address), sizeof(pred->address)) == 0) {
            rpc_send_check_predecessor(pred_sockfd);
            close(pred_sockfd);
        } else {
            LOG(WARNING) << "Predecessor has failed";
            predecessor = nullptr;
        }

        delete pred;
    }
}

Node* Node::findSuccessor(const uint8_t* id) {
    if (within(id, this->getId(), (const uint8_t*)successor->id().c_str())) {
        return new chord::Node(*successor);
    } else {
        auto node = closetPrecedingNode(id);

        struct sockaddr_in addr;
        CHECK_GE(inet_pton(AF_INET, node->getAddr().c_str(), &addr.sin_addr.s_addr), 1) << "Invalid IPv4 address";
        addr.sin_port   = htons(node->getPort());
        addr.sin_family = AF_INET;

        int32_t peer_sockfd;
        CHECK_GE(peer_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), 0) << "Failed to create socket";

        if (connect(peer_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(peer_sockfd);
            LOG(FATAL) << "Failed to connect to server";
        }

        rpc_send_find_successor(peer_sockfd, node);
        return new chord::Node(*node->successor);
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
