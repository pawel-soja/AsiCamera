#include <unordered_map>
#include <ASICamera2Headers.h>

#include <libusb-1.0/libusb.h>

/* Relationship containers */
static std::unordered_map<CirBuf *, CCameraBase *>             gMap_CirBuf_CCameraBase;
static std::unordered_map<int, CCameraBase *>                  gMap_iCameraID_CCameraBase;
static std::unordered_map<CCameraBase*, libusb_device_handle*> gMap_CCameraBase_libusb_device_handle;


CCameraBase * getCCameraBase(int iCameraID)
{
    return gMap_iCameraID_CCameraBase[iCameraID];
}

CCameraBase * getCCameraBase(CirBuf *cirBuf)
{
    return gMap_CirBuf_CCameraBase[cirBuf];
}

libusb_device_handle * getDeviceHandle(CCameraBase *ccameraBase)
{
    return gMap_CCameraBase_libusb_device_handle[ccameraBase];
}

/* Save relationship between CirBuf/iCameraID/libusb_device_handle and CCameraBase */
static CCameraBase * gMap_CCameraBase = nullptr;
static int gMap_iCameraID = -1;
static libusb_device_handle * gMap_libusb_device_handle = nullptr;

// #1
extern "C" int __real_ASIOpenCamera(int iCameraID);
extern "C" int __wrap_ASIOpenCamera(int iCameraID)
{
    fprintf(stderr, "[ASIOpenCamera]: grab CameraID %d\n", iCameraID);
    gMap_iCameraID = iCameraID;
    return __real_ASIOpenCamera(iCameraID);
}

// #2
extern "C" void __real__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase);
extern "C" void __wrap__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase)
{
    fprintf(stderr, "[CCameraBase::InitVariable]: grab CCameraBase %p\n", ccameraBase);
    gMap_CCameraBase = ccameraBase;
    __real__ZN11CCameraBase12InitVariableEv(ccameraBase);
}

// #3
extern "C" int __real_libusb_open(libusb_device *dev, libusb_device_handle **devh);
extern "C" int __wrap_libusb_open(libusb_device *dev, libusb_device_handle **devh)
{
    int rc = __real_libusb_open(dev, devh);
    fprintf(stderr, "[libusb_open]: grab libusb_device_handle %p\n", *devh);
    gMap_libusb_device_handle = *devh;
    return rc;
}

// #4
extern "C" void __real__ZN6CirBufC1El(CirBuf *cirBuf, long size);
extern "C" void __wrap__ZN6CirBufC1El(CirBuf *cirBuf, long size)
{
    
    if (gMap_CCameraBase == nullptr)
    {
        fprintf(stderr, "[CirBuf::CirBuf]: cannot link CCameraBase %p and CirBuf %p\n", gMap_CCameraBase, cirBuf);
        goto fail;
    }

    if (gMap_libusb_device_handle == nullptr)
    {
        fprintf(stderr, "[CirBuf::CirBuf]: cannot link CCameraBase %p and libusb_device_handle %p\n", gMap_CCameraBase, gMap_libusb_device_handle);
        goto fail;
    }

    if (gMap_iCameraID == -1)
    {
        fprintf(stderr, "[CirBuf::CirBuf]: cannot link iCameraID %d and CCameraBase %p\n", gMap_iCameraID, gMap_CCameraBase);
        goto fail;
    }
    
    fprintf(stderr, "[CirBuf::CirBuf]: link CirBuf %p and CCameraBase %p\n", cirBuf, gMap_CCameraBase);
    gMap_CirBuf_CCameraBase[cirBuf] = gMap_CCameraBase;

    fprintf(stderr, "[CirBuf::CirBuf]: link CCameraBase %p and libusb_device_handle %p\n", gMap_CCameraBase, gMap_libusb_device_handle);
    gMap_CCameraBase_libusb_device_handle[gMap_CCameraBase] = gMap_libusb_device_handle;
    
    fprintf(stderr, "[CirBuf::CirBuf]: link iCameraID %d and CCameraBase %p\n", gMap_iCameraID, gMap_CCameraBase);
    gMap_iCameraID_CCameraBase[gMap_iCameraID] = gMap_CCameraBase;

fail:
    gMap_CCameraBase = nullptr;
    gMap_libusb_device_handle = nullptr;
    gMap_iCameraID = -1;

    __real__ZN6CirBufC1El(cirBuf, size);
}



void **find_pointer_address(void *container, size_t size, const void *pointer)
{
    while(size > 0)
    {
        if(*(const void **)container == pointer)
            return (void **)container;
        container = (void *)((ptrdiff_t)container + 2);
        size -= 2;
    }
    return NULL;
}
