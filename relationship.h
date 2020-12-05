#pragma once

class CirBuf;
class CCameraBase;
struct libusb_device_handle;

CCameraBase          * getCCameraBase(int iCameraID);
CCameraBase          * getCCameraBase(CirBuf *);
libusb_device_handle * getDeviceHandle(CCameraBase *);


void **find_pointer_address(void *container, size_t size, const void *pointer);
