cmake_minimum_required(VERSION 3.15)
project(CardsGameTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Dependencies
# ============================================================

# GoogleTest – fetched automatically if not found on system
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Engine library (shared with main build)
if(NOT TARGET card_engine)
    add_subdirectory(
        ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine
        ${CMAKE_CURRENT_BINARY_DIR}/CardsGameEngine-build
    )
endif()

# gRPC + proto library (needed for block tests)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC     CONFIG REQUIRED)

set(PROTO_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/../gRPC/proto)
set(PROTO_FILES ${PROTO_DIR}/cardsGame.proto)

add_library(proto_lib_test STATIC)

protobuf_generate(
    TARGET proto_lib_test
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
    IMPORT_DIRS ${PROTO_DIR}
)

set(GRPC_SRCS)
set(GRPC_HDRS)
foreach(proto ${PROTO_FILES})
    get_filename_component(proto_name ${proto} NAME_WE)
    list(APPEND GRPC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc)
    list(APPEND GRPC_HDRS ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h)
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc
                ${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h
        COMMAND protobuf::protoc
        ARGS    --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
                --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                -I ${PROTO_DIR} ${proto}
        DEPENDS ${proto} gRPC::grpc_cpp_plugin
    )
endforeach()

target_sources(proto_lib_test PRIVATE ${GRPC_SRCS} ${GRPC_HDRS})
target_link_libraries(proto_lib_test PUBLIC protobuf::libprotobuf gRPC::grpc++)
target_include_directories(proto_lib_test PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/proto
)

# ============================================================
# GameSession object files (reused by block tests, no main)
# ============================================================

add_library(gamesession_lib OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/../gRPC/src/GameSession.cpp
)
target_link_libraries(gamesession_lib PUBLIC proto_lib_test card_engine)
target_include_directories(gamesession_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../gRPC/inc
)

# ============================================================
# Unit tests – pure engine logic, no gRPC
# ============================================================

add_executable(unit_tests
    unit/test_briscola_rules.cpp
    unit/test_tressette_rules.cpp
    unit/test_deck_and_cards.cpp
)
target_link_libraries(unit_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        card_engine
)
target_include_directories(unit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/Briscola
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/Tressette
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/EngineBase
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/Events
)
gtest_discover_tests(unit_tests)

# ============================================================
# Block tests – GameSession event translation layer
# Requires adding "friend class TestableGameSession;" to GameSession.h
# Enable with: cmake -DBUILD_BLOCK_TESTS=ON
# ============================================================

option(BUILD_BLOCK_TESTS "Build block tests (requires GameSession.h modification)" OFF)

if(BUILD_BLOCK_TESTS)
    add_executable(block_tests
        block/test_game_session_events.cpp
    )
target_link_libraries(block_tests
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        gamesession_lib
        proto_lib_test
        card_engine
)
target_include_directories(block_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../gRPC/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/Briscola
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/EngineBase
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/Events
    ${CMAKE_CURRENT_SOURCE_DIR}/../CardsGameEngine/inc/Tressette
)
    gtest_discover_tests(block_tests)
endif()

# ============================================================
# Template client (documentation artifact – not a test)
# ============================================================

add_executable(client_template
    ${CMAKE_CURRENT_SOURCE_DIR}/../doc/src/client_template.cpp
)
target_link_libraries(client_template PRIVATE proto_lib_test)
target_include_directories(client_template PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../gRPC/inc
)
