
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
#include "relationship.h"
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

struct BoostCameraData
{
    CameraBoost cameraBoost;
    bool resetDeviceNeeded;
    void **usbBuffer;
    void **imageBuffer;

    void *realUsbBuffer;
    void *realImageBuffer;
};

static std::unordered_map<CCameraBase *, BoostCameraData> gBoostCameraData;
static std::mutex gBoostCameraDataMutex;

static BoostCameraData *getBoostCameraData(CCameraBase *ccameraBase)
{
    std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    //fprintf(stderr, "[getBoostCameraData]: CCameraBase %p\n", ccameraBase);
    return &gBoostCameraData[ccameraBase];
}

static BoostCameraData *getBoostCameraData(CCameraFX3 *ccameraFX3)
{
    std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    CCameraBase *ccameraBase = static_cast<CCameraBase*>(ccameraFX3);
    //fprintf(stderr, "[getBoostCameraData]: CCameraFX3 %p\n", ccameraFX3);
    return &gBoostCameraData[ccameraBase];
}

static BoostCameraData *getBoostCameraData(CirBuf *cirBuf)
{
    std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    CCameraBase *ccameraBase = getCCameraBase(cirBuf);
    //fprintf(stderr, "[getBoostCameraData]: CirBuf %p\n", cirBuf);
    return &gBoostCameraData[ccameraBase];
}


extern "C"
{

// CCameraFX3::ResetDevice
int __real__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3);
int __wrap__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3)
{
    if (gBoostCameraEnabled)
    {
        fprintf(stderr, "[CCameraFX3::ResetDevice]\n");

        BoostCameraData *data = getBoostCameraData(ccameraFX3);
        
        data->resetDeviceNeeded = false;
    }
    return __real__ZN10CCameraFX311ResetDeviceEv(ccameraFX3);
}

// CCameraFX3::initAsyncXfer
void __real__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, unsigned char endpoint, unsigned char *buffer);
void __wrap__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, unsigned char endpoint, unsigned char *buffer)
{
    if (!gBoostCameraEnabled)
        return __real__ZN10CCameraFX313initAsyncXferEiiihPh(ccameraFX3, bufferSize, transferCount, chunkSize, endpoint, buffer);

    CCameraBase *ccameraBase = static_cast<CCameraBase*>(ccameraFX3);
    BoostCameraData *data = getBoostCameraData(ccameraBase);
    
    libusb_device_handle *device_handle = getDeviceHandle(ccameraBase);
    
    fprintf(stderr, "[CCameraFX3::initAsyncXfer]: init CameraBoost device %p, endpoint 0x%x, buffer size %d\n", device_handle, endpoint, bufferSize);
    
    data->resetDeviceNeeded = false;
    data->cameraBoost.init(device_handle, endpoint, bufferSize);

    data->usbBuffer = find_pointer_address(ccameraBase, 0x600, buffer);
    data->realUsbBuffer = buffer;
}

// CCameraFX3::startAsyncXfer
void __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size);
void __wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size)
{
    if (!gBoostCameraEnabled)
        __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(ccameraFX3, timeout1, timeout2, bytesRead, stop, size);

    BoostCameraData *data = getBoostCameraData(ccameraFX3);

    if (data->resetDeviceNeeded)
        return;

    data->cameraBoost.start(timeout1);
    //fprintf(stderr, "[CCameraFX3::startAsyncXfer]: timeout %d %d\n", timeout1, timeout2);
    if (data->cameraBoost.peek() != nullptr)
    {
        *bytesRead = size;
    }
    else
    {
        fprintf(stderr, "[CCameraFX3::startAsyncXfer]: reset device needed\n");
        data->resetDeviceNeeded = true;
    }
}

typedef unsigned char uchar;

// CirBuf::InsertBuff
int __real__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int a1, ushort a2, int a3, ushort a4, int a5, int a6, int a7);
int __wrap__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int i1, ushort v1, int i2, ushort v2, int a5, int a6, int a7)
{
    if (!gBoostCameraEnabled)
        return __real__ZN6CirBuf10InsertBuffEPhititiii(cirBuf, buffer, i1, v1, i2, v2, a5, a6, a7);
#if 0
    CCameraBase * ccameraBase = getCCameraBase(cirBuf);
    
    BoostCameraData *data = boostCameraData(circBuf);

    if (data->resetDeviceNeeded)
        return 2;
    
    if (data->cameraBoost.isStarted() == false)
    {
        fprintf(stderr, "[CirBuf::InsertBuff]: reset device needed\n");
        data->resetDeviceNeeded = true;
        return 2; // fail frame
    }
#endif
    // TODO check buffer
    //fprintf(stderr, "[%s]: buffer: %p\n", __FUNCTION__, buffer);
    return 0;
}

// CirBuf::ReadBuff
int __real__ZN6CirBuf8ReadBuffEPhii(CirBuf *cirBuf, unsigned char* buffer, uint size, uint timeout);
int __wrap__ZN6CirBuf8ReadBuffEPhii(CirBuf *cirBuf, unsigned char* buffer, uint size, uint timeout)
{
    if (!gBoostCameraEnabled)
        return __real__ZN6CirBuf8ReadBuffEPhii(cirBuf, buffer, size, timeout);
    
    CCameraBase *ccameraBase = getCCameraBase(cirBuf);
    BoostCameraData *data = getBoostCameraData(cirBuf);

    if (data->imageBuffer == nullptr)
    {
        data->imageBuffer = find_pointer_address(ccameraBase, 0x600, buffer);
        data->realImageBuffer = buffer;
        assert(("[CirBuf::ReadBuff]: cannot find image buffer", data->imageBuffer != nullptr));
    }

    unsigned char *p = data->cameraBoost.get();
    //fprintf(stderr, "[CirBuf::ReadBuff]: %d %d %p\n", *(short*)&p[0x00000000 + 2], *(short*)&p[0x006242f0 + 12], buffer);

    *data->imageBuffer = p;
    return 1;
}

void __real__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3);
void __wrap__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3)
{
    if (!gBoostCameraEnabled)
        return __real__ZN10CCameraFX316releaseAsyncXferEv(ccameraFX3);

    BoostCameraData *data = getBoostCameraData(ccameraFX3);

    data->cameraBoost.stop();

    *data->usbBuffer = data->realUsbBuffer;
    *data->imageBuffer = data->realImageBuffer;
}

#if 1
ASICAMERA_API  ASI_ERROR_CODE ASIGetVideoDataPointer(int iCameraID, unsigned char** pBuffer, int iWaitms)
{
    CCameraBase *ccameraBase = getCCameraBase(iCameraID);
    BoostCameraData *data = getBoostCameraData(ccameraBase);

    //pBuffer = reinterpret_cast<unsigned char **>(data->imageBuffer);
    unsigned char * p = data->cameraBoost.peek();
    if (p == nullptr)
        return ASI_ERROR_TIMEOUT;
    //fprintf(stderr, "[ASIGetVideoDataPointer]: buffer size %d\n", data->cameraBoost.bufferSize);
    ASIGetVideoData(iCameraID, p, data->cameraBoost.bufferSize(), iWaitms);
    *pBuffer = p;
    //gCameraBoost[iCameraID]->dst
    return ASI_SUCCESS;
}
#endif

}
