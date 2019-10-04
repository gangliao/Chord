CC=gcc
CFLAG_INCLUDE=-Iinclude -I.
CFLAGS=-Wall -Wextra -std=c++11 -ggdb
LDLIBS=-lprotobuf -lcrypto
VPATH=src

CFLAGS=-Wall -Wextra $(CFLAG_INCLUDE) -std=gnu99
LDFLAGS := -lcrypto -pthread
VPATH :=src:include

all: chord

chord: main.cc chord.pb.cc node.cc finger_table.cc successor.cc

chord.pb.cc: proto/chord.proto
	protoc --cpp_out=. $<

clean:
	rm -rf *~ *.o proto/chord.pb.* chord

.PHONY : clean all
