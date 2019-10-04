
#pragma once

#include <openssl/sha.h>
#include <iostream>
#include <string>
#include <vector>

#include "chord.h"
#include "detail/short_alloc.h"

namespace chord {
class Node {
   public:
    // Create a vector<T> template with a small buffer of 32 bytes.
    //   Note for vector it is possible to reduce the alignment requirements
    //   down to alignof(T) because vector doesn't allocate anything but T's.
    //   And if we're wrong about that guess, it is a compile-time error, not
    //   a run time error.
    template <class T, std::size_t BufSize = 32>
    using Array = std::vector<T, short_alloc<T, BufSize, alignof(T)>>;
    Array<Byte>::allocator_type::arena_type a;
    Array<Byte> id{a};

   public:
    struct sockaddr_in address;
    struct sockaddr_in join_address;

   public:
    int32_t r;
    Milliseconds tv_stabilize;
    Milliseconds tv_fix_fingers;
    Milliseconds tv_check_predecessor;

   public:
    /*! \brief creates a new Chord ring. */
    void create();

    /*! \brief joins a Chord ring containing node n. */
    void join(const Node& n);

    /*! \brief looks up a value from Chord. */
    std::string lookup(std::string key);

    /*! \brief prints its local state information at the current time. */
    std::string dump();

   public:
    inline Array<Byte> getId() { return id; }

    inline int16_t getPort() { return ntohs(address.sin_port); }

    inline std::string getAddr() { return inet_ntoa(address.sin_addr); }

   public:
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
    Node* findSuccessor(const Array<Byte>& id);

    /*! \brief searches the local table for the highest predecessor of id. */
    Node* closetPrecedingNode(const Array<Byte>& id);
};
}  // namespace chord