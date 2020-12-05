#include "cameraboost.h"
#include <thread>
#include <functional>
#include <unistd.h>

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
