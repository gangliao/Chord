
#pragma once

#include <deque>
#include <iostream>
#include <memory>
#include <string>

#include <openssl/sha.h>

#include "chord.h"

namespace chord {
class Node {
   public:
    // marshalling attributes
    uint8_t id[SHA_DIGEST_LENGTH];
    std::string addr;
    std::int16_t port;

   public:
    int32_t r;
    std::shared_ptr<Node*> preccessor;
    std::shared_ptr<Node*> successor;
    std::deque<std::shared_ptr<Node*>> succ_list;

   public:
    int32_t server_sockfd;
    struct sockaddr_in address;
    struct sockaddr_in join_address;

   public:
    Milliseconds tv_stabilize;
    Milliseconds tv_fix_fingers;
    Milliseconds tv_check_predecessor;

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

    inline int16_t getPort() { return ntohs(address.sin_port); }

    inline std::string getAddr() { return inet_ntoa(address.sin_addr); }

   public:
    void bind_and_listen();

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

   private:
    /*! \brief n thinks it might be our predecessor. */
    void notify(const Node& n);

    /*! \brief asks node to find the successor of id. */
    Node* findSuccessor(const uint8_t* id);

    /*! \brief searches the local table for the highest predecessor of id. */
    Node* closetPrecedingNode(const uint8_t* id);
};
}  // namespace chord