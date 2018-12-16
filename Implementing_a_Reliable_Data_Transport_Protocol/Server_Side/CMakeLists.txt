cmake_minimum_required(VERSION 3.9)
project(Network_Ass2 C)

set(CMAKE_C_STANDARD 99)
include_directories(../Client_Side)

add_executable(Network_Server Server.c)