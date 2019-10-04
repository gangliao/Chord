#include <iostream>

#include <glog/logging.h>

#include "chord.h"
#include "common/cxxopts.h"
#include "node.h"

void init_node(const cxxopts::ParseResult& result, chord::Node* node) {
    CHECK_GE(inet_pton(AF_INET, result["a"].as<std::string>().c_str(), &node->address.sin_addr.s_addr), 1)
        << "Invalid IPv4 address";
    node->address.sin_family = AF_INET;

    int16_t port = result["p"].as<int16_t>();
    CHECK_GE(port, 1024) << "Invalid option for a port, must be greater than or equal to 1024";
    CHECK_LE(port, 65535) << "Invalid option for a port, must be less than or equal to 65535";
    node->address.sin_port = htons(port);

    CHECK_GE(inet_pton(AF_INET, result["ja"].as<std::string>().c_str(), &node->join_address.sin_addr.s_addr), 1)
        << "Invalid IPv4 address";
    node->join_address.sin_family = AF_INET;

    port = result["jp"].as<int16_t>();
    CHECK_GE(port, 1024) << "Invalid option for a port, must be greater than or equal to 1024";
    CHECK_LE(port, 65535) << "Invalid option for a port, must be less than or equal to 65535";
    node->join_address.sin_port = htons(port);

    int32_t ts = result["ts"].as<int32_t>();
    CHECK_GE(ts, 1) << "The time in milliseconds between invocations of 'stabilize' must be greater than or equal to 1";
    CHECK_LE(ts, 60000)
        << "The time in milliseconds between invocations of 'stabilize' must be less than or equal to 60000";
    node->tv_stabilize = ts * 1.0e-3;

    int32_t tff = result["tff"].as<int32_t>();
    CHECK_GE(tff, 1)
        << "The time in milliseconds between invocations of 'fix fingers' must be greater than or equal to 1";
    CHECK_LE(tff, 60000)
        << "The time in milliseconds between invocations of 'fix fingers' must be less than or equal to 60000";
    node->tv_fix_fingers = tff * 1.0e-3;

    int32_t tcp = result["tcp"].as<int32_t>();
    CHECK_GE(tcp, 1)
        << "The time in milliseconds between invocations of 'check predecessor' must be greater than or equal to 1";
    CHECK_LE(tcp, 60000)
        << "The time in milliseconds between invocations of 'check predecessor' must be less than or equal to 60000";
    node->tv_check_predecessor = tcp * 1.0e-3;

    int32_t r = result["r"].as<int32_t>();
    CHECK_GE(r, 1)
        << "The number of successors maintained must be must be greater than or equal to 1";
    CHECK_LE(r, 32)
        << "The number of successors maintained must be must be less than or equal to 32";
    node->r = r;
}

int main(int argc, char* argv[]) {
    cxxopts::Options options("Chord", "A Scalable Peer-to-Peer Lookup Protocol for Internet Applications");

    // clang-format off
    options.allow_unrecognised_options().add_options("Chord")
        ("a,addr",  "The IPv4 address to bind to and advertise to other nodes", cxxopts::value<std::string>()->default_value("127.0.0.1"))
        ("p,port",  "The port to bind to (required)", cxxopts::value<int16_t>())
        ("ja",      "The IPv4 address of a node whose ring to join", cxxopts::value<std::string>()->default_value("127.0.0.1"))
        ("jp",      "The port of a node whose ring to join (required)", cxxopts::value<int16_t>())
        ("ts",      "The time in milliseconds between invocations of 'stabilize'", cxxopts::value<int32_t>()->default_value("100"))
        ("tff",     "The time in milliseconds between invocations of 'fix fingers'", cxxopts::value<int32_t>()->default_value("100"))
        ("tcp",     "The time in milliseconds between invocations of 'check predecessor'", cxxopts::value<int32_t>()->default_value("100"))
        ("r",       "The number of successors to maintain", cxxopts::value<int32_t>()->default_value("3"))
        ("h,help",  "Print help")
        ("v",       "Enable verbose");
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") || result.count("h") || !result.count("p") || !result.count("jp")) {
        std::cout << options.help({"", "Chord"}) << std::endl;
        exit(0);
    }

    if (!result.count("v")) {
        google::InitGoogleLogging(argv[0]);
    }

    // initialize chord node
    auto node = new chord::Node();
    init_node(result, node);

    return 0;
}
