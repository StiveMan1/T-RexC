cmake_minimum_required(VERSION 3.29)
project(T_RexC C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(T_RexC
    dino.c
    game_objects.h)
