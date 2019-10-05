proto_library(chord_proto
    SRCS proto/chord.proto)

cc_binary(chord
    SRCS main.cc node.cc finger_table.cc successor.cc
         rpc.cc proto/chord.pb.cc common/socket-util.cc
    DEPS crypto chord_proto)
