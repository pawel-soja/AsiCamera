cmake_minimum_required (VERSION 2.8)

PROJECT(AsiCameraPerformance)

add_executable(AsiCameraPerformance
    main.cpp
    lib/asicamera.cpp
    lib/asicameracontrol.cpp
    lib/asicamerainfo.cpp
)
include_directories(lib/)
#add_subdirectory (lib)

link_directories(${CMAKE_BINARY_DIR}/lib/x64/)
target_link_libraries(AsiCameraPerformance ASICamera2Boost)
