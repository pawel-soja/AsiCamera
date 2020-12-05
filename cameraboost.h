#pragma once

#include <vector>
#include "queue.h"
#include <atomic>
#include <libusb-cpp/libusb.h>
#include <thread>

#include "stypes.h"

class CCameraFX3;
class CirBuf;
class CCameraBase;
class CameraBoost
{
public:
    enum // constants
    {
        Buffers = 16,
        Transfers = 2,
        MaximumTransferChunkSize = 1024*1024
    };

public:
    CameraBoost();

    size_t bufferSize() const;

    void start(uint timeout = 1000);
    void stop();

    bool isStarted() const;

    unsigned char *get();
    unsigned char *peek();

public: // reimplement
    void initAsyncXfer(int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer);
    void startAsyncXfer(uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size);
    void releaseAsyncXfer();

    void ResetDevice();

    int ReadBuff(unsigned char* buffer, uint size, uint timeout);
    int InsertBuff(uchar *buffer, int i1, ushort v1, int i2, ushort v2, int a5, int a6, int a7);

protected:
    std::vector<unsigned char> mBuffer[Buffers];
    
    Queue<unsigned char*> mBuffersFree;
    Queue<unsigned char*> mBuffersReady;
    Queue<unsigned char*> mBuffersBusy;

    std::atomic<bool> mStarted;

    LibUsbChunkedBulkTransfer  mTransfer[Transfers];

    std::thread mThread;

    size_t mBufferSize;

public: // taken from the library
    int mCameraID;
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
