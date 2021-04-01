cmake_minimum_required (VERSION 2.8)

PROJECT(AsiCameraSimply)

add_executable(AsiCameraSimply
    main.cpp
)


IF (CMAKE_SYSTEM_PROCESSOR MATCHES "armv+")
    set(ASIARCH "armv6")
ELSEIF (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(ASIARCH "armv8")
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(ASIARCH "x64")
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    set(ASIARCH "x86")
ENDIF ()

link_directories(${CMAKE_BINARY_DIR}/lib/${ASIARCH}/)
target_link_libraries(AsiCameraSimply ASICamera2Boost ${CMAKE_DL_LIBS})
