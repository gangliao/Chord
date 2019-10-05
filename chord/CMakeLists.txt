proto_library(chord_proto
    SRCS proto/chord.proto)

cc_library(chord_node
    SRCS proto/chord.pb.cc node.cc
    DEPS chord_proto)

cc_library(chord_core
    SRCS finger_table.cc successor.cc rpc.cc
    DEPS crypto chord_node)



cc_binary(chord
    SRCS main.cc
    DEPS chord_core chord_proto)
