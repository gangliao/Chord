CC=gcc
CFLAGS=-Wall -I. -Wextra -std=c++11 -ggdb
LDLIBS=-lprotobuf -lcrypto
# VPATH=chord

all: chord

chord: main.cc chord.pb.cc

chord.pb.cc: proto/chord.proto
	protoc --cpp_out=. $<

clean:
	rm -rf *~ *.o proto/chord.pb.* chord

.PHONY : clean all
