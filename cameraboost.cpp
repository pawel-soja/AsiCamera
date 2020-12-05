#include "cameraboost.h"
#include <thread>
#include <functional>
#include <unistd.h>
#include <cassert>
#include "ASICamera2Headers.h"


static void **find_pointer_address(void *container, size_t size, const void *pointer)
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

CameraBoost::CameraBoost()
    : mStarted(false)
{
    fprintf(stderr, "[CameraBoost::CameraBoost]: created\n");
}

void CameraBoost::init(libusb_device_handle *dev, unsigned char endpoint, size_t bufferSize)
{
    mBufferSize = bufferSize;

    // extend buffer size - better damaged frame than retransmission
    bufferSize  = (bufferSize + MaximumTransferChunkSize - 1) / MaximumTransferChunkSize;
    bufferSize *= MaximumTransferChunkSize;

    mBuffersBusy.clear();
    mBuffersReady.clear();
    mBuffersFree.clear();

    // Lazy load
    std::thread([this, bufferSize](){
        for (std::vector<unsigned char> &buffer: mBuffer)
        {
            buffer.resize(bufferSize);
            if (buffer.size() != bufferSize)
            {
                fprintf(stderr, "[CameraBoost::init]: allocation fail\n");
                break;
            }
            mBuffersFree.push(buffer.data());
        }
    }).detach();

    
    for (LibUsbChunkedBulkTransfer &transfer: mTransfer)
        transfer = LibUsbChunkedBulkTransfer(dev, endpoint, NULL, bufferSize, MaximumTransferChunkSize);
}

size_t CameraBoost::bufferSize() const
{
    return mBufferSize;
}

void CameraBoost::start(uint timeout)
{
    if (mStarted.exchange(true))
        return;

    if (mThread.joinable())
        mThread.join();

    mThread = std::thread([this, timeout](){
        
        unsigned char *buffer = nullptr;

        for (LibUsbChunkedBulkTransfer &transfer: mTransfer)
            transfer.setBuffer(nullptr);
        
        //pthread_mutex_lock(&mtx_usb);

        struct Cleanup {
            std::function<void()> f;
            ~Cleanup() { f(); }
        } cleanup{[&](){
            fprintf(stderr, "[CameraBoost::thread]: cleanup\n");
            mStarted = false;
            mBuffersFree.push(buffer);
            //pthread_mutex_unlock(&mtx_usb);
        }};

        while(true)
        {
            for (LibUsbChunkedBulkTransfer &transfer: mTransfer)
            {
                transfer.wait(1000);
                buffer = reinterpret_cast<unsigned char*>(transfer.buffer());
                if (!mStarted)
                {
                    fprintf(stderr, "[CameraBoost::thread]: exit triggered...\n");
                    return;
                }
                
                if (buffer != nullptr && transfer.actualLength() != mBufferSize)
                {
                    fprintf(stderr, "[CameraBoost::thread]: invalid buffer size %d != %d\n", transfer.actualLength(), mBufferSize);
                    if (transfer.actualLength() == 0) // fatal error
                        return;
                    
                    mBuffersFree.push(buffer);
                }
                else
                {
                    mBuffersReady.push(buffer);
                }
                
                buffer = mBuffersFree.pop(100);
                if (buffer == nullptr)
                {
                    fprintf(stderr, "[CameraBoost::thread]: buffer timeout, exiting...\n");
                    return;
                }
                transfer.setBuffer(buffer).submit();
            }
        }
    });
}

void CameraBoost::stop()
{
    // TODO remove buffers ?
    fprintf(stderr, "[CameraBoost::stop]\n");
    mStarted = false;
    for (LibUsbChunkedBulkTransfer &transfer: mTransfer)
        transfer.cancel();
    mThread.join();
}

bool CameraBoost::isStarted() const
{
    return mStarted;
}

unsigned char *CameraBoost::get()
{
    unsigned char *buffer = mBuffersReady.pop(-1);     // take ready buffer
    mBuffersFree.push(mBuffersBusy.pop(0)); // move processed buffer to free buffer
    mBuffersBusy.push(buffer);                         // add current buffer to busy queue
    return buffer;
}

unsigned char *CameraBoost::peek()
{
    return mBuffersReady.peek(1000);
}


// reimplement

void CameraBoost::initAsyncXfer(int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer)
{
    fprintf(stderr, "[CCameraFX3::initAsyncXfer]: init CameraBoost device %p, endpoint 0x%x, buffer size %d\n", mDeviceHandle, endpoint, bufferSize);
    
    resetDeviceNeeded = false;
    init(mDeviceHandle, endpoint, bufferSize);

    usbBuffer = find_pointer_address(mCCameraBase, 0x600, buffer);
    realUsbBuffer = buffer;

}

void CameraBoost::startAsyncXfer(uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size)
{
    if (resetDeviceNeeded)
        return;

    start(timeout1);
    //fprintf(stderr, "[CCameraFX3::startAsyncXfer]: timeout %d %d\n", timeout1, timeout2);
    if (peek() != nullptr)
    {
        *bytesRead = size;
    }
    else
    {
        fprintf(stderr, "[CCameraFX3::startAsyncXfer]: reset device needed\n");
        resetDeviceNeeded = true;
    }
}

void CameraBoost::releaseAsyncXfer()
{
    fprintf(stderr, "[CameraBoost::releaseAsyncXfer]\n");
    stop();

    *usbBuffer = realUsbBuffer;
    *imageBuffer = realImageBuffer;
}

int CameraBoost::ReadBuff(unsigned char* buffer, uint size, uint timeout)
{
    if (imageBuffer == nullptr)
    {
        imageBuffer = find_pointer_address(mCCameraBase, 0x600, buffer);
        realImageBuffer = buffer;
        assert(("[CirBuf::ReadBuff]: cannot find image buffer", imageBuffer != nullptr));
    }

    unsigned char *p = get();
    fprintf(stderr, "[CirBuf::ReadBuff]: %d %d %p\n", *(short*)&p[0x00000000 + 2], *(short*)&p[0x006242f0 + 12], buffer);

    *imageBuffer = p;

    return 1;
}

int CameraBoost::InsertBuff(uchar *buffer, int i1, ushort v1, int i2, ushort v2, int a5, int a6, int a7)
{
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
    return 0;
}

void CameraBoost::ResetDevice()
{
    fprintf(stderr, "[CCameraFX3::ResetDevice]\n");
    resetDeviceNeeded = false;
}
