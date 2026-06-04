cmake_minimum_required(VERSION 3.16)
project(ChessEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release builds are much faster - use for actual play
# Debug builds have assertions and are safer while developing
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra")

# Main engine executable
add_executable(chess-engine
    src/main.cpp
    src/board.cpp
)
target_include_directories(chess-engine PRIVATE src)

# Perft test executable (Phase 3)
# add_executable(perft tests/perft.cpp src/board.cpp src/movegen.cpp)
# target_include_directories(perft PRIVATE src)
