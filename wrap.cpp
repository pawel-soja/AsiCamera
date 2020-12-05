#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <mutex>
#include <unordered_map>
#include <atomic>

#include "ASICamera2Headers.h"
#include "cameraboost.h"


void make_fake_buffer(void *dst)
{
    unsigned char *p = reinterpret_cast<unsigned char*>(dst);
    static int index = 1;
    p[0x00000000 + 0]  = 0x7e;
    p[0x00000000 + 1]  = 0x5a;
    *(short*)&p[0x00000000 + 2]  = index;

    *(short*)&p[0x006242f0 + 12] = index;
    p[0x006242f0 + 14] = 0xf0;
    p[0x006242f0 + 15] = 0x3c;

    ++index;
}

bool gBoostCameraEnabled = true;

static std::mutex gBoostCameraDataMutex;
static std::unordered_map<int,          CameraBoost>  gCameraBoost;
static std::unordered_map<CCameraBase*, CameraBoost*> gCameraBoost_CCameraBase;
static std::unordered_map<CirBuf*,      CameraBoost*> gCameraBoost_CirBuf;
static int gActiveCameraID = -1;

static CameraBoost *getCameraBoost(int iCameraID = gActiveCameraID)
{
    std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    if (iCameraID == -1)
    {
        fprintf(stderr, "[getCameraBoost]: invalid iCameraID!\n");
        return nullptr;
    }
    //fprintf(stderr, "[getBoostCameraData]: iCameraID %d\n", iCameraID);
    return &gCameraBoost[iCameraID];
}

static CameraBoost *& getCameraBoost(CCameraBase *ccameraBase)
{
    //std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    //fprintf(stderr, "[getBoostCameraData]: CCameraBase %p\n", ccameraBase);
    return gCameraBoost_CCameraBase[ccameraBase];
}

static CameraBoost *& getCameraBoost(CCameraFX3 *ccameraFX3)
{
    //std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    CCameraBase *ccameraBase = static_cast<CCameraBase*>(ccameraFX3);
    //fprintf(stderr, "[getBoostCameraData]: CCameraFX3 %p\n", ccameraFX3);
    return gCameraBoost_CCameraBase[ccameraBase];
}

static CameraBoost *& getCameraBoost(CirBuf *cirBuf)
{
    //std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    //fprintf(stderr, "[getBoostCameraData]: CirBuf %p\n", cirBuf);
    return gCameraBoost_CirBuf[cirBuf];
}

// #1
extern "C" int __real_ASIOpenCamera(int iCameraID);
extern "C" int __wrap_ASIOpenCamera(int iCameraID)
{
    fprintf(stderr, "[ASIOpenCamera]: grab CameraID %d\n", iCameraID);
    gActiveCameraID = iCameraID;
    getCameraBoost()->mCameraID = iCameraID;
    return __real_ASIOpenCamera(iCameraID);
}

// #2
extern "C" void __real__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase);
extern "C" void __wrap__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase)
{
    fprintf(stderr, "[CCameraBase::InitVariable]: grab CCameraBase %p\n", ccameraBase);
    CameraBoost *cameraBoost = getCameraBoost();
    cameraBoost->mCCameraBase = ccameraBase;
    gCameraBoost_CCameraBase[ccameraBase] = cameraBoost;
    __real__ZN11CCameraBase12InitVariableEv(ccameraBase);
}

// #3
extern "C" int __real_libusb_open(libusb_device *dev, libusb_device_handle **devh);
extern "C" int __wrap_libusb_open(libusb_device *dev, libusb_device_handle **devh)
{
    int rc = __real_libusb_open(dev, devh);
    fprintf(stderr, "[libusb_open]: grab libusb_device_handle %p\n", *devh);
    getCameraBoost()->mDeviceHandle = *devh;
    return rc;
}

// #4
extern "C" void __real__ZN6CirBufC1El(CirBuf *cirBuf, long size);
extern "C" void __wrap__ZN6CirBufC1El(CirBuf *cirBuf, long size)
{
    CameraBoost *cameraBoost = getCameraBoost();
    cameraBoost->mCirBuf = cirBuf;
    gCameraBoost_CirBuf[cirBuf] = cameraBoost;
    gActiveCameraID = -1; // done

    __real__ZN6CirBufC1El(cirBuf, size);
}




extern "C"
{

// CCameraFX3::ResetDevice
int __real__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3);
int __wrap__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3)
{
    if (gBoostCameraEnabled)
    {
        getCameraBoost(ccameraFX3)->ResetDevice();
    }
    return __real__ZN10CCameraFX311ResetDeviceEv(ccameraFX3);
}

// CCameraFX3::initAsyncXfer
void __real__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, unsigned char endpoint, unsigned char *buffer);
void __wrap__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, unsigned char endpoint, unsigned char *buffer)
{
    if (!gBoostCameraEnabled)
        return __real__ZN10CCameraFX313initAsyncXferEiiihPh(ccameraFX3, bufferSize, transferCount, chunkSize, endpoint, buffer);

    getCameraBoost(ccameraFX3)->initAsyncXfer(bufferSize, transferCount, chunkSize, endpoint, buffer);
}

// CCameraFX3::startAsyncXfer
void __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size);
void __wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size)
{
    if (!gBoostCameraEnabled)
        __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(ccameraFX3, timeout1, timeout2, bytesRead, stop, size);

    getCameraBoost(ccameraFX3)->startAsyncXfer(timeout1, timeout2, bytesRead, stop, size);
}

// CirBuf::InsertBuff
int __real__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int a1, ushort a2, int a3, ushort a4, int a5, int a6, int a7);
int __wrap__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int i1, ushort v1, int i2, ushort v2, int a5, int a6, int a7)
{
    if (!gBoostCameraEnabled)
        return __real__ZN6CirBuf10InsertBuffEPhititiii(cirBuf, buffer, i1, v1, i2, v2, a5, a6, a7);

    return getCameraBoost(cirBuf)->InsertBuff(buffer, i1, v1, i2, v2, a5, a6, a7);
}

// CirBuf::ReadBuff
int __real__ZN6CirBuf8ReadBuffEPhii(CirBuf *cirBuf, unsigned char* buffer, uint size, uint timeout);
int __wrap__ZN6CirBuf8ReadBuffEPhii(CirBuf *cirBuf, unsigned char* buffer, uint size, uint timeout)
{
    if (!gBoostCameraEnabled)
        return __real__ZN6CirBuf8ReadBuffEPhii(cirBuf, buffer, size, timeout);

    return getCameraBoost(cirBuf)->ReadBuff(buffer, size, timeout);
}

void __real__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3);
void __wrap__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3)
{
    if (!gBoostCameraEnabled)
        return __real__ZN10CCameraFX316releaseAsyncXferEv(ccameraFX3);

    getCameraBoost(ccameraFX3)->releaseAsyncXfer();
}

ASICAMERA_API  ASI_ERROR_CODE ASIGetVideoDataPointer(int iCameraID, unsigned char** pBuffer, int iWaitms)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);

    unsigned char * p = cameraBoost->peek();
    if (p == nullptr)
        return ASI_ERROR_TIMEOUT;

    //fprintf(stderr, "[ASIGetVideoDataPointer]: buffer size %d\n", data->cameraBoost.bufferSize);
    ASIGetVideoData(iCameraID, p, cameraBoost->bufferSize(), iWaitms);
    *pBuffer = p;
    
    return ASI_SUCCESS;
}

}
