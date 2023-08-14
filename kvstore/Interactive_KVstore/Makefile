LDFLAGS = -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
	-pthread\
	-lgrpc++_reflection\
	-ldl

CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11

GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

all: client server

client: keyvalue.pb.o keyvalue.grpc.pb.o client.o
	$(CXX) $^ $(LDFLAGS) -o $@

server: keyvalue.pb.o keyvalue.grpc.pb.o server.o
	$(CXX) $^ $(LDFLAGS) -o $@

%.grpc.pb.cc: %.proto
	protoc --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	protoc --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h client server

run_server:
	./server

run_client:
	./client