cmake_minimum_required (VERSION 2.8)

PROJECT(AsiCameraTest)

add_executable(AsiCameraTest main.cpp)

link_directories(${CMAKE_BINARY_DIR}/lib/x64/)
target_link_libraries(AsiCameraTest ASICamera2Boost)
