#include <iostream>

#include "chord.h"
#include "common/async_timer_queue.h"
#include "common/cxxopts.h"
#include "node.h"

void init_node(const cxxopts::ParseResult& result, chord::Node* node) {
    // address
    CHECK_GE(inet_pton(AF_INET, result["a"].as<std::string>().c_str(), &node->address.sin_addr.s_addr), 1)
        << "Invalid IPv4 address";
    node->address.sin_family = AF_INET;
    node->addr               = result["a"].as<std::string>();

    // port
    if (result.count("p")) {
        int16_t port = result["p"].as<int16_t>();
        CHECK_GE(port, 1024) << "Invalid option for a port, must be greater than or equal to 1024";
        CHECK_LE(port, 65535) << "Invalid option for a port, must be less than or equal to 65535";
        node->address.sin_port = htons(port);
        node->port             = result["p"].as<std::int16_t>();
    }

    // join address
    CHECK_GE(inet_pton(AF_INET, result["ja"].as<std::string>().c_str(), &node->join_address.sin_addr.s_addr), 1)
        << "Invalid IPv4 address";
    node->join_address.sin_family = AF_INET;

    // join ip
    if (result.count("jp")) {
        int16_t port = result["jp"].as<int16_t>();
        CHECK_GE(port, 1024) << "Invalid option for a port, must be greater than or equal to 1024";
        CHECK_LE(port, 65535) << "Invalid option for a port, must be less than or equal to 65535";
        node->join_address.sin_port = htons(port);
    }

    // stabilize time
    int32_t ts = result["ts"].as<int32_t>();
    CHECK_GE(ts, 1) << "The time in milliseconds between invocations of 'stabilize' must be greater than or equal to 1";
    CHECK_LE(ts, 60000)
        << "The time in milliseconds between invocations of 'stabilize' must be less than or equal to 60000";
    node->tv_stabilize = ts;

    // fix fingers time
    int32_t tff = result["tff"].as<int32_t>();
    CHECK_GE(tff, 1)
        << "The time in milliseconds between invocations of 'fix fingers' must be greater than or equal to 1";
    CHECK_LE(tff, 60000)
        << "The time in milliseconds between invocations of 'fix fingers' must be less than or equal to 60000";
    node->tv_fix_fingers = tff;

    // check predecessor time
    int32_t tcp = result["tcp"].as<int32_t>();
    CHECK_GE(tcp, 1)
        << "The time in milliseconds between invocations of 'check predecessor' must be greater than or equal to 1";
    CHECK_LE(tcp, 60000)
        << "The time in milliseconds between invocations of 'check predecessor' must be less than or equal to 60000";
    node->tv_check_predecessor = tcp;

    // # successors
    int32_t r = result["r"].as<int32_t>();
    CHECK_GE(r, 1) << "The number of successors maintained must be must be greater than or equal to 1";
    CHECK_LE(r, 32) << "The number of successors maintained must be must be less than or equal to 32";
    node->r = r;

    // id = hash(ip:port)
    std::string ip_port = result["a"].as<std::string>() + ":" + std::to_string(result["p"].as<int16_t>());
    SHA1((const uint8_t*)ip_port.c_str(), ip_port.size(), node->id);
}

int main(int argc, char* argv[]) {
    cxxopts::Options options("Chord", "A Scalable Peer-to-Peer Lookup Protocol for Internet Applications");

    // clang-format off
    options.allow_unrecognised_options().add_options("Chord")
        ("a,addr",  "The IPv4 address to bind to and advertise to other nodes", cxxopts::value<std::string>()->default_value("127.0.0.1"))
        ("p,port",  "The port to bind to (required)", cxxopts::value<int16_t>())
        ("ja",      "The IPv4 address of a node whose ring to join", cxxopts::value<std::string>()->default_value("127.0.0.1"))
        ("jp",      "The port of a node whose ring to join (required)", cxxopts::value<int16_t>())
        ("ts",      "The time in milliseconds between invocations of 'stabilize'", cxxopts::value<int32_t>()->default_value("30000"))
        ("tff",     "The time in milliseconds between invocations of 'fix fingers'", cxxopts::value<int32_t>()->default_value("1000"))
        ("tcp",     "The time in milliseconds between invocations of 'check predecessor'", cxxopts::value<int32_t>()->default_value("30000"))
        ("r",       "The number of successors to maintain", cxxopts::value<int32_t>()->default_value("3"))
        ("h,help",  "Print help")
        ("v",       "Enable verbose");
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") || result.count("h") || (!result.count("p") && !result.count("jp"))) {
        std::cout << options.help({"", "Chord"}) << std::endl;
        exit(0);
    }

    google::InstallFailureSignalHandler();
    if (!result.count("v")) {
        google::InitGoogleLogging(argv[0]);
    }

    // init chord node
    auto node = new chord::Node();
    init_node(result, node);

    if (!result.count("jp")) {
        node->create();
        node->initFingers();
    } else {
        node->join();
        node->initFingers();
    }

    // called periodically
    std::thread asyncthread(&chord::AsyncTimerQueue::timerLoop, &chord::AsyncTimerQueue::Instance());
    chord::AsyncTimerQueue::Instance().create(node->tv_fix_fingers, true, &chord::Node::fixFingers, node);
    chord::AsyncTimerQueue::Instance().create(node->tv_check_predecessor, true, &chord::Node::checkPredecessor, node);
    chord::AsyncTimerQueue::Instance().create(node->tv_stabilize, true, &chord::Node::stabilize, node);

    // bind and listen to socket (non-blocking)
    node->rpc_server();

    std::string line;
    while (1) {
        std::cout << "> ";
        std::getline(std::cin, line);
        std::istringstream stream(line);
        std::string cmd;
        std::string key;
        stream >> cmd >> key;
        if (cmd == "Lookup") {
            if (key.empty()) {
                continue;
            }
            node->lookup(key);
        } else if (cmd == "PrintState") {
            node->dump();
        }
    }

    asyncthread.join();
    return 0;
}
