cmake_minimum_required (VERSION 2.8)

project (ASICamera2Boost CXX C)

set(ASIVERSION "1.30")

IF (CMAKE_SYSTEM_PROCESSOR MATCHES "armv+")
    set(ASIARCH "armv6")
ELSEIF (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(ASIARCH "armv8")
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(ASIARCH "x64")
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    set(ASIARCH "x86")
ENDIF ()

add_custom_command (
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libASICamera2_redefined.a
    COMMAND objcopy
        --redefine-sym ASICloseCamera=__real_ASICloseCamera
        --redefine-sym ASIOpenCamera=__real_ASIOpenCamera
        --redefine-sym ASIGetNumOfControls=__real_ASIGetNumOfControls
        --redefine-sym ASIGetControlCaps=__real_ASIGetControlCaps
        --redefine-sym ASIGetControlValue=__real_ASIGetControlValue
        --redefine-sym ASISetControlValue=__real_ASISetControlValue
        --redefine-sym ASIGetVideoData=__real__ASIGetVideoData
        --redefine-sym ASIGetDataAfterExp=__real__ASIGetDataAfterExp

        ${PROJECT_SOURCE_DIR}/../libasicamera2/${ASIVERSION}/lib/${ASIARCH}/libASICamera2.a ${CMAKE_CURRENT_BINARY_DIR}/libASICamera2_redefined.a
)

add_custom_target (generate-libASICamera2_redefined ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libASICamera2_redefined.a)

add_library (ASICamera2Boost SHARED
    src/wrap.cpp
    src/cameraboost.cpp
    src/libusb-cpp/libusbchunkedbulktransfer.cpp
)
add_dependencies (ASICamera2Boost generate-libASICamera2_redefined)

target_compile_options (ASICamera2Boost PRIVATE -Wall -Werror)

include_directories (src)
target_include_directories(ASICamera2Boost PUBLIC ../libasicamera2/${ASIVERSION}/include include)


target_link_libraries(ASICamera2Boost PRIVATE pthread usb-1.0)
target_link_libraries(ASICamera2Boost PRIVATE
    -Wl,--whole-archive
    -Wl,--wrap=_ZN6CirBuf8ReadBuffEPhiiPS0_
    -Wl,--wrap=_ZN6CirBuf10InsertBuffEPhititiii
    -Wl,--wrap=_ZN6CirBufC1Eli
    -Wl,--wrap=_ZN11CCameraBase12InitVariableEv
    -Wl,--wrap=_ZN11CCameraBase11CloseCameraEv
    -Wl,--wrap=_ZN10CCameraFX313initAsyncXferEiiihPh
    -Wl,--wrap=_ZN10CCameraFX314startAsyncXferEjjPiPbi
    -Wl,--wrap=_ZN10CCameraFX316releaseAsyncXferEv
    -Wl,--wrap=_ZN10CCameraFX311ResetDeviceEv
    -Wl,--wrap=libusb_open
    ${CMAKE_CURRENT_BINARY_DIR}/libASICamera2_redefined.a
    -Wl,--no-whole-archive
)


string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

IF(uppercase_CMAKE_BUILD_TYPE MATCHES DEBUG)
  message("Debug build.")
  target_link_libraries(ASICamera2Boost PRIVATE -Wl,--wrap=memcpy)
ENDIF()

include(GNUInstallDirs)

set (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

install (TARGETS ASICamera2Boost LIBRARY DESTINATION "${LIB_INSTALL_DIR}${LIB_POSTFIX}")
install (FILES include/ASICamera2Boost.h DESTINATION include/libasi)
