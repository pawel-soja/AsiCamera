cmake_minimum_required (VERSION 2.8)

PROJECT(AsiCameraSimply)

add_executable(AsiCameraSimply
    main.cpp
)

link_directories(${CMAKE_BINARY_DIR}/lib/x64/)
target_link_libraries(AsiCameraSimply ASICamera2Boost)
