cmake_minimum_required(VERSION 3.16)

project(ReplicatedWAL
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WAL_SOURCES
    src/wal/Record.cpp
    src/wal/Wal.cpp
    src/server/WalWriter.cpp
    src/server/WalServer.cpp
    src/client/Client.cpp
)

add_library(wal_core
    ${WAL_SOURCES}
)

target_include_directories(wal_core PUBLIC
    src/wal
    src/server
    src/client
)

if (WIN32)
    target_link_libraries(wal_core PUBLIC ws2_32)
endif()

add_executable(wal_server_demo
    app/server_demo.cpp
)

add_executable(wal_client_demo
    app/client_demo.cpp
)

target_link_libraries(wal_server_demo PRIVATE wal_core)
target_link_libraries(wal_client_demo PRIVATE wal_core)

add_executable(wal_tests
    tests/wal_integration_tests.cpp
)

target_link_libraries(wal_tests PRIVATE wal_core)