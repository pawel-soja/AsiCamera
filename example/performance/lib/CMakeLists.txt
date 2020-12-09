cmake_minimum_required (VERSION 2.8)

project (AsiCameraLib CXX C)

add_library (ASICamera2Boost STATIC
    asicamera.cpp
    asicameracontrol.cpp
    asicamerainfo.cpp
)

target_compile_options (ASICamera2Boost PRIVATE -Wall -Werror)


#target_include_directories(ASICamera2Boost PUBLIC ../libasicamera2/include include)


target_link_libraries(ASICamera2Boost PRIVATE pthread usb-1.0)

string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

IF(uppercase_CMAKE_BUILD_TYPE MATCHES DEBUG)
  message("Debug build.")
  target_link_libraries(ASICamera2Boost PRIVATE -Wl,--wrap=memcpy)
ENDIF()

include(GNUInstallDirs)

set (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

install (TARGETS ASICamera2Boost LIBRARY DESTINATION "${LIB_INSTALL_DIR}${LIB_POSTFIX}")
install (FILES include/ASICamera2Boost.h DESTINATION include/libasi)
