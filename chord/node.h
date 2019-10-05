#pragma once

#include "chord.h"
#include "common/bigint.h"

namespace chord {
class Node {
   public:
    // marshalling attributes
    uint8_t* id;
    std::string addr;
    std::int16_t port;

   public:
    int32_t r;
    Node* predecessor;
    Node* successor;
    std::deque<Node*> succ_list;
    std::deque<Node*> finger_table;

   public:
    int32_t server_sockfd;
    struct sockaddr_in address;
    struct sockaddr_in join_address;

   public:
    /*! \brief A type for seconds */
    typedef double Milliseconds;
    Milliseconds tv_stabilize;
    Milliseconds tv_fix_fingers;
    Milliseconds tv_check_predecessor;

   public:
    Node() { id = new uint8_t[SHA_DIGEST_LENGTH]; }
    Node(const protocol::Node& node) {
        id = new uint8_t[SHA_DIGEST_LENGTH];
        memcpy(id, node.id().c_str(), SHA_DIGEST_LENGTH);
        addr = node.address();
        CHECK_GE(inet_pton(AF_INET, addr.c_str(), &address.sin_addr.s_addr), 1) << "Invalid IPv4 address";
        address.sin_family = AF_INET;
    }

   public:
    /*! \brief creates a new Chord ring. */
    void create();

    /*! \brief joins a Chord ring containing node n. */
    void join();

    /*! \brief looks up a value from Chord. */
    void lookup(std::string key);

    /*! \brief prints its local state information at the current time. */
    void dump();

   public:
    inline const uint8_t* getId() { return id; }

    inline const int16_t getPort() { return ntohs(address.sin_port); }

    inline const std::string getAddr() { return inet_ntoa(address.sin_addr); }

   public:
    void rpc_server();

    /**
     * \brief  verifies its immediate successor, and tells the successor.
     * \note   called periodically.
     */
    void stabilize();

    /**
     * \brief  refreshes finger table entries. next stores the index of
     *         the next finger to fix.
     * \note   called periodically.
     */
    void fixFingers();

    /**
     * \brief  checks whether predecessir has failed.
     * \note   called periodically.
     */
    void checkPredecessor();

   public:
    /*! \brief n thinks it might be our predecessor. */
    void notify(const Node* n);

    /*! \brief asks node to find the successor of id. */
    Node* findSuccessor(const uint8_t* id);

    /*! \brief searches the local table for the highest predecessor of id. */
    Node* closetPrecedingNode(const uint8_t* id);
};
}  // namespace chord