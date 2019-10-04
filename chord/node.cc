#include "node.h"

#include <sstream>

namespace chord {

inline void print_hash(const uint8_t *hash, uint16_t size) {
    for (int i = 0; i < size; i++) {
        printf("%02x", hash[i]);
    }
    /* Print newline */
    puts("");
}

void Node::create() {}

void Node::join(const Node& n) {}

void Node::lookup(std::string key) {
    std::stringstream a("< " + key + " ");

    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1((const uint8_t*)key.c_str(), key.size(), hash);
    std::cout << "< " + key + " ";
    print_hash(hash, SHA_DIGEST_LENGTH);

    Node* succ = this->findSuccessor(hash);
    std::cout << "< ";
    print_hash(succ->getId(), SHA_DIGEST_LENGTH);
    std::cout << " " + succ->getAddr() + " " + std::to_string(succ->getPort());
}

void Node::dump() {}

void Node::stabilize() {}

void Node::fixFingers() {}

void Node::checkPredecessor() {}

Node* Node::findSuccessor(const uint8_t* id) { return nullptr; }

Node* Node::closetPrecedingNode(const uint8_t* id) { return nullptr; }

}  // namespace chord