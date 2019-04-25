cmake_minimum_required (VERSION 2.6)
project (Pyrkon)
add_executable(Pyrkon pyrkon.cpp)
ADD_LIBRARY(LibsModule
        Queue.cpp
        Packet.cpp
        )