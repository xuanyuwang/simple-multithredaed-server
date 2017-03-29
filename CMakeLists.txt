cmake_minimum_required(VERSION 3.6)
project(sws)

set(CMAKE_C_STANDARD 99)
file(GLOB SWSFILE network.c network.h RCB.c RCB.h sws.c)
ADD_EXECUTABLE(sws ${SWSFILE})

project(dllist)
file(GLOB DLLIST dllist.c)
ADD_EXECUTABLE(dllist ${DLLIST})