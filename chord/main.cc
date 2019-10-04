#include <iostream>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "chord.h"

DEFINE_string(a, "127.0.0.1", "The IPv4 address to bind to and advertise to other nodes");
DEFINE_uint32(p, -1, "The port to bind to");
DEFINE_string(ja, "127.0.0.1", "The IPv4 address of a node whose ring to join");
DEFINE_uint32(jp, -1, "The port of a node whose ring to join");
DEFINE_uint32(ts, 100, "The time in milliseconds between invocations of 'stabilize'");
DEFINE_uint32(tff, 100, "The time in milliseconds between invocations of 'fix fingers'");
DEFINE_uint32(tcp, 100, "The time in milliseconds between invocations of 'check predecessor'");
DEFINE_uint32(r, 4, "The number of successors to maintain");
DEFINE_bool(d, false, "Enable Debugging");

int main(int argc, char *argv[]) {
    cxxopts::Options options("Chord", "A Scalable Peer-to-Peer Lookup Protocol for Internet Applications");

    // clang-format off
    options.allow_unrecognised_options().add_options("Chord")
        ("a",       "The IPv4 address to bind to and advertise to other nodes", cxxopts::value<std::string>()->default_value("127.0.0.1"))
        ("p",       "The port to bind to", cxxopts::value<int32_t>())
        ("ja",      "The IPv4 address of a node whose ring to join", cxxopts::value<std::string>()->default_value("127.0.0.1"))
        ("jp",      "The port of a node whose ring to join", cxxopts::value<int32_t>())
        ("ts",      "The time in milliseconds between invocations of 'stabilize'", cxxopts::value<int32_t>())
        ("tff",     "The time in milliseconds between invocations of 'fix fingers'", cxxopts::value<int32_t>())
        ("tcp",     "The time in milliseconds between invocations of 'check predecessor'", cxxopts::value<int32_t>())
        ("r",       "The number of successors to maintain", cxxopts::value<int32_t>())
        ("h,help",  "Print help")
        ("v",       "Enable verbose");
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") || result.count("h")) {
        std::cout << options.help({"", "Chord"}) << std::endl;
        exit(0);
    }

    if (!result.count("v")) {
        google::InitGoogleLogging(argv[0]);
    }

    if (result.count("a")) {
        if (inet_pton(AF_INET, value, &args->address.sin_addr.s_addr) < 1) argp_error(state, "Invalid IPv4 address");
        args->address.sin_family = AF_INET;
    }
    if (result.count("help")) {
    }

    LOG(INFO) << "AAAA";
    LOG(INFO) << result["a"].as<std::string>();
    return 0;
}
