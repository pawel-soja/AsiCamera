#pragma once

#include <vector>
#include "queue.h"
#include <atomic>
#include <libusb-cpp/libusb.h>
#include <thread>

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
    void init(libusb_device_handle *dev, unsigned char endpoint, size_t bufferSize);

    size_t bufferSize() const;

    void start(uint timeout = 1000);
    void stop();

    bool isStarted() const;

    unsigned char *get();
    unsigned char *peek();

protected:
    std::vector<unsigned char> mBuffer[Buffers];
    
    Queue<unsigned char*> mBuffersFree;
    Queue<unsigned char*> mBuffersReady;
    Queue<unsigned char*> mBuffersBusy;

    std::atomic<bool> mStarted;

    LibUsbChunkedBulkTransfer  mTransfer[Transfers];

    std::thread mThread;

    size_t mBufferSize;
};
