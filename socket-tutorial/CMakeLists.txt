cmake_minimum_required(VERSION 3.10)
# set the project name
project(Socket-Tutorial)
set(CMAKE_BUILD_TYPE Debug)

# add the executable
add_executable(client client.c)
add_executable(server server.c)
add_executable(server2 server2.c)
add_executable(server3 server3.c)