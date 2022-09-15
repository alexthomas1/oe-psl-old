# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

include ../../config.mk

CXX_COMPILER=g++

CFLAGS=$(shell pkg-config oehost-$(COMPILER) --cflags)
CXXFLAGS=$(shell pkg-config oeenclave-$(CXX_COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oehost-$(CXX_COMPILER) --variable=includedir)

# PROTO=../protobufs-bin/bin/protoc
# PROTO_INCL=-I../protobufs-bin/include -I/usr/include/ -I/usr/local/include/
# PROTO_LIB=-L../protobufs-bin/lib -lprotobuf -lgflags -lglog -lpthread -lcrypto -lzmq -labsl_strings -labsl_raw_logging_internal -labsl_throw_delegate
PROTO_INCL=-I/usr/include/ -I/usr/local/include/ -I./include/
PROTO_LIB=-lprotobuf -lgflags -lglog -lpthread -lcrypto -lzmq -labsl_strings -labsl_raw_logging_internal -labsl_throw_delegate


build:
	@ echo $(COMPILER)
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../helloworld.edl --untrusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(PROTO) -I./proto/ --cpp_out=. ./proto/capsule.proto
	$(PROTO) -I./proto/ --cpp_out=. ./proto/hello.proto
	# g++ -g -c $(CXXFLAGS) $(INCLUDES) $(PROTO_INCL) -std=c++11 helloworld_u.c host.cc zmq_comm.cc asylo_sgx.cc spinlock.cc capsule.pb.cc hello.pb.cc util/proto_util.cc
	g++ -g -c $(CXXFLAGS) $(INCLUDES) $(PROTO_INCL) -std=c++17 src/host.cc src/zmq_comm.cc src/asylo_sgx.cc src/spinlock.cc capsule.pb.cc hello.pb.cc util/proto_util.cc
	g++ -o helloworldhost -lstdc++ host.o zmq_comm.o asylo_sgx.o capsule.pb.o spinlock.o proto_util.o $(LDFLAGS) $(PROTO_LIB)

	# g++ -o helloworldhost helloworld_u.o host.o zmq_comm.o asylo_sgx.o capsule.pb.o spinlock.o proto_util.o $(LDFLAGS) $(PROTO_LIB) -loecxx

clean:
	rm -f helloworldhost host.o helloworld_u.o helloworld_u.c helloworld_u.h helloworld_args.h *.o
