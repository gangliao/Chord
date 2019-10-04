cc_library(chord_core
    SRCS node.cc finger_table.cc successor.cc
    DEPS crypto)

proto_library(chord_proto
    SRCS proto/chord.proto)

cc_binary(chord
    SRCS main.cc
    DEPS chord_core chord_proto)
