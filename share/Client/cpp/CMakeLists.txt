cmake_minimum_required(VERSION 3.15)
project(CardsGameClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static linking configuration for Windows
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a)
    set(Protobuf_USE_STATIC_LIBS ON)
    add_compile_options(/MT$<$<CONFIG:Debug>:d>)
endif()

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../proto)
set(PROTO_FILES ${PROTO_DIR}/cardsGame.proto)

# ---- Proto library
add_library(proto_lib STATIC)

# ---- Protobuf generation
protobuf_generate(
    TARGET proto_lib
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${PROTO_DIR}
)

# ---- gRPC generation
foreach(proto ${PROTO_FILES})
    get_filename_component(proto_name ${proto} NAME_WE)
    
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
        COMMAND protobuf::protoc
        ARGS
            --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            -I ${PROTO_DIR}
            ${proto}
        DEPENDS ${proto} gRPC::grpc_cpp_plugin
    )
    
    target_sources(proto_lib PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
    )
endforeach()

target_link_libraries(proto_lib
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

target_include_directories(proto_lib
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
)

# ---- Client
add_executable(client client.cpp)
target_link_libraries(client PRIVATE proto_lib)
target_include_directories(client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)