#include "node.h"

namespace chord {
void Node::create() {}
void Node::join(const Node& n) {}

void Node::stabilize() {}

void Node::fixFingers() {}

void Node::checkPredecessor() {}
Node* Node::findSuccessor(const Array<Byte>& id) { return nullptr; }
Node* Node::closetPrecedingNode(const Array<Byte>& id) { return nullptr; }
}  // namespace chord