proto_library(chord_proto
    SRCS proto/chord.proto)

cc_binary(chord
    SRCS main.cc node.cc rpc.cc
         successor.cc
         proto/chord.pb.cc
         common/socket-util.cc
         common/net-buffer.cc
         common/bigint.cc
    DEPS crypto chord_proto)
