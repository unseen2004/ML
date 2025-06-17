cmake_minimum_required(VERSION 3.10)
project(TicTacToe5x5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add optimization flags
add_compile_options(-W -pedantic -O3)

# Find GSL (for random bot)
find_package(GSL REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add executables
add_executable(game_server game_server.cpp)
add_executable(game_client game_client.cpp)
add_executable(game_random_bot game_random_bot.cpp)

# Link GSL to random bot
target_link_libraries(game_random_bot ${GSL_LIBRARIES})