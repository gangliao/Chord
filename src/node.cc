
#include <iostream>
#include <string>

#include "common.h"

namespace chord {
class Node {
   private:
    std::string ip_;
    std::uint32_t port_;
    byte id_[32];

   public:
    /*! \brief creates a new Chord ring. */
    void create();
    /*! \brief joins a Chord ring containing node n. */
    void join(const Node& n);

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

   private:
    /*! \brief n thinks it might be our predecessor. */
    void notify(const Node& n);
    /**
     * \brief  checks whether predecessir has failed.
     * \note   called periodically.
     */
    void checkPredecessor();
    /*! \brief asks node to find the successor of id. */
    Node* findSuccessor(const byte& id) { return nullptr; }
    /*! \brief searches the local table for the highest predecessor of id. */
    Node* closetPrecedingNode(const byte& id) { return nullptr; }
};
}  // namespace chord