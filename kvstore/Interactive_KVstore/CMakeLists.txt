# # Minimum CMake required
# cmake_minimum_required(VERSION 3.15)


# # Protobuf
# set(protobuf_MODULE_COMPATIBLE TRUE)
# find_package(Protobuf CONFIG REQUIRED)
# message(STATUS "Using protobuf ${protobuf_VERSION}")

# # Protobuf-compiler
# set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# # gRPC
# find_package(gRPC CONFIG REQUIRED)
# message(STATUS "Using gRPC ${gRPC_VERSION}")
# set(_GRPC_GRPCPP gRPC::grpc++)
# set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# # Proto file
# get_filename_component(kv_proto "keyvalue.proto" ABSOLUTE)
# get_filename_component(kv_proto_path "${kv_proto}" PATH)

# # Generated sources
# set(kv_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.pb.cc")
# set(kv_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.pb.h")
# set(kv_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.grpc.pb.cc")
# set(kv_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.grpc.pb.h")
# add_custom_command(
#       OUTPUT "${kv_proto_srcs}" "${kv_proto_hdrs}" "${kv_grpc_srcs}" "${kv_grpc_hdrs}"
#       COMMAND ${_PROTOBUF_PROTOC}
#       ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
#         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
#         -I "${kv_proto_path}"
#         --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
#         "${kv_proto}"
#       DEPENDS "${kv_proto}")

# # Include generated *.pb.h files
# include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# # hw_grpc_proto
# add_library(kv_grpc_proto
#   ${kv_grpc_srcs}
#   ${kv_grpc_hdrs}
#   ${kv_proto_srcs}
#   ${kv_proto_hdrs})

# target_link_libraries(kv_grpc_proto
#   ${_REFLECTION}
#   ${_GRPC_GRPCPP}
#   ${_PROTOBUF_LIBPROTOBUF})

# # Targets (client|server)
# foreach(_target
#   client server)
#   add_executable(${_target} "${_target}.cc"
#     ${kv_proto_srcs}
#     ${kv_grpc_srcs})
#   target_link_libraries(${_target}
#     kv_grpc_proto
#     ${_REFLECTION}
#     ${_GRPC_GRPCPP}
#     ${_PROTOBUF_LIBPROTOBUF})
# endforeach()


# Copyright 2018 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# cmake build file for C++ helloworld example.
# Assumes protobuf and gRPC have been installed using cmake.
# See cmake_externalproject/CMakeLists.txt for all-in-one cmake build
# that automatically builds all the dependencies before building helloworld.

cmake_minimum_required(VERSION 3.5.1)

project(keyvalue C CXX)

include(common.cmake)

# Proto file
get_filename_component(kv_proto "../protos/keyvalue.proto" ABSOLUTE)
get_filename_component(kv_proto_path "${kv_proto}" PATH)

# Generated sources
set(kv_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.pb.cc")
set(kv_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.pb.h")
set(kv_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.grpc.pb.cc")
set(kv_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/keyvalue.grpc.pb.h")
add_custom_command(
      OUTPUT "${kv_proto_srcs}" "${kv_proto_hdrs}" "${kv_grpc_srcs}" "${kv_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${kv_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${kv_proto}"
      DEPENDS "${kv_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# hw_grpc_proto
add_library(kv_grpc_proto
  ${kv_grpc_srcs}
  ${kv_grpc_hdrs}
  ${kv_proto_srcs}
  ${kv_proto_hdrs})
target_link_libraries(kv_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Targets greeter_[async_](client|server)
foreach(_target
  client server)
  add_executable(${_target} "${_target}.cc")
  target_link_libraries(${_target}
    kv_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
endforeach()
