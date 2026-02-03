cmake_minimum_required(VERSION 3.15)
project(ExampleGrpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)


find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_FILES
    ${PROTO_DIR}/cardsGame.proto
)

# ---- Proto library
add_library(proto_lib)

# ---- Protobuf generation (.pb.cc/.pb.h)
protobuf_generate(
    TARGET proto_lib
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
)

# ---- gRPC generation (manual, but correct)
get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)

set(GRPC_SRCS)
set(GRPC_HDRS)

foreach(proto ${PROTO_FILES})
    get_filename_component(proto_name ${proto} NAME_WE)

    list(APPEND GRPC_SRCS
        ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc
    )
    list(APPEND GRPC_HDRS
        ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
    )

    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
        COMMAND protobuf::protoc
        ARGS
            --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            -I ${PROTO_DIR}
            ${proto}
        DEPENDS ${proto}
    )
endforeach()

target_sources(proto_lib
    PRIVATE
        ${GRPC_SRCS}
        ${GRPC_HDRS}
)

target_link_libraries(proto_lib
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

target_include_directories(proto_lib
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/proto
)


# ---- Server
add_executable(server 
    src/server.cpp
    src/GameSession.cpp
    src/ProtoToDomain.cpp)
target_link_libraries(server PRIVATE proto_lib)

target_include_directories(server
    PRIVATE
        "${CMAKE_SOURCE_DIR}/inc"
        "${CMAKE_SOURCE_DIR}/../CardsGameEngine/inc"
        "${CMAKE_SOURCE_DIR}/../CardsGameEngine/inc/EngineBase"
        "${CMAKE_SOURCE_DIR}/../CardsGameEngine/inc/Tressette"
        "${CMAKE_SOURCE_DIR}/../CardsGameEngine/inc/Briscola"
        "${CMAKE_SOURCE_DIR}/../CardsGameEngine/inc/Events"
)

target_link_libraries(server
    PRIVATE
        "${CMAKE_SOURCE_DIR}/../CardsGameEngine/build/libcard_engine.dylib"
)

# ---- Client
add_executable(client src/client.cpp)
target_link_libraries(client PRIVATE proto_lib)
target_include_directories(client
    PRIVATE
        "${CMAKE_SOURCE_DIR}/inc"
)

