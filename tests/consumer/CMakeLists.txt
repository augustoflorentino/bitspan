cmake_minimum_required(VERSION 3.16)
project(bitspan_consumer LANGUAGES C)

find_package(bitspan REQUIRED)

add_executable(consumer ${CMAKE_CURRENT_SOURCE_DIR}/../../examples/basic.c)
target_link_libraries(consumer PRIVATE bitspan::bitspan)
