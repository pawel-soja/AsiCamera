#pragma once

#include <vector>
#include "queue.h"
#include <atomic>
#include <libusb-cpp/libusb.h>

#include "stypes.h"

class CCameraFX3;
class CirBuf;
class CCameraBase;
class CameraBoost
{
public:
    enum // constants
    {
        Buffers = 8,                         // TODO initial count of buffers
        Transfers = 3,                       // TODO limit to maximum possible transfers
        MaximumTransferChunkSize = 1024*1024 // 
    };

public:
    CameraBoost();
    size_t bufferSize() const;

    uchar *get();
    uchar *peek();

public: // reimplement
    void initAsyncXfer(int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer);
    void startAsyncXfer(uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size);
    void releaseAsyncXfer();

    void ResetDevice();

    int ReadBuff(uchar* buffer, uint size, uint timeout);
    int InsertBuff(uchar *buffer, int i1, ushort v1, int i2, ushort v2, int a5, int a6, int a7);

protected:
    std::vector<uchar> mBuffer[Buffers];
    
    Queue<uchar*> mBuffersFree;
    Queue<uchar*> mBuffersReady;
    Queue<uchar*> mBuffersBusy;

    uchar *mCurrentBuffer = nullptr;

    LibUsbChunkedBulkTransfer  mTransfer[Transfers];
    int mTransferIndex = 0;

    size_t mBufferSize;

public: // taken from the library
    int                   mCameraID;
    libusb_device_handle *mDeviceHandle = nullptr;
    CCameraBase          *mCCameraBase = nullptr;
    CirBuf               *mCirBuf = nullptr;

protected:
    bool resetDeviceNeeded = false;
    void **usbBuffer = nullptr;
    void **imageBuffer = nullptr;

    void *realUsbBuffer = nullptr;
    void *realImageBuffer = nullptr;
};
