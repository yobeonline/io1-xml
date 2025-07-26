cmake_minimum_required(VERSION 3.25)

project(test_io1-xml_install LANGUAGES CXX)

find_package(io1 REQUIRED COMPONENTS xml)

add_library(${PROJECT_NAME} test_install.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE io1::xml)