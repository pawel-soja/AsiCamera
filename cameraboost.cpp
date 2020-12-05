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
{
    fprintf(stderr, "[CameraBoost::CameraBoost]: created\n");
}

size_t CameraBoost::bufferSize() const
{
    return mBufferSize;
}

uchar *CameraBoost::get()
{
    uchar *buffer = mBuffersReady.pop(-1);  // take ready buffer, TODO timeout
    mBuffersFree.push(mBuffersBusy.pop(0)); // move processed buffer to free buffer queue
    mBuffersBusy.push(buffer);              // add current buffer to busy queue
    return buffer;
}

uchar *CameraBoost::peek()
{
    return mBuffersReady.peek(1000); // TODO timeout
}

// reimplement

void CameraBoost::initAsyncXfer(int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer)
{
    fprintf(stderr, "[CCameraFX3::initAsyncXfer]: init CameraBoost device %p, endpoint 0x%x, buffer size %d\n", mDeviceHandle, endpoint, bufferSize);
    
    resetDeviceNeeded = false;
    mBufferSize = bufferSize;

    // extend buffer size - better damaged frame than retransmission
#if 0
    bufferSize  = (bufferSize + MaximumTransferChunkSize - 1) / MaximumTransferChunkSize;
    bufferSize *= MaximumTransferChunkSize;
#endif
    mBuffersBusy.clear();
    mBuffersReady.clear();
    mBuffersFree.clear();

    // Lazy load
    std::thread([this, bufferSize](){
        for (std::vector<uchar> &buffer: mBuffer)
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
        transfer = LibUsbChunkedBulkTransfer(mDeviceHandle, endpoint, NULL, bufferSize, MaximumTransferChunkSize);

    usbBuffer = find_pointer_address(mCCameraBase, 0x600, buffer);
    realUsbBuffer = buffer;
}

void CameraBoost::startAsyncXfer(uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size)
{
    if (resetDeviceNeeded)
        return;

    LibUsbChunkedBulkTransfer &transfer = mTransfer[mTransferIndex];

    // at the beginning, transfer has null buffer
    mCurrentBuffer = reinterpret_cast<uchar*>(transfer.wait().buffer()); // TODO timeout

    if (mCurrentBuffer != nullptr)
    {
        if (transfer.actualLength() == 0)
        {
            fprintf(stderr, "[CameraBoost::startAsyncXfer]: null transfer length, reset device needed\n");
            resetDeviceNeeded = true;
            mBuffersFree.push(mCurrentBuffer);
            mCurrentBuffer = nullptr;
            return;
        }
        if (transfer.actualLength() != mBufferSize)
        {
            fprintf(stderr, "[CameraBoost::startAsyncXfer]: invalid transfer length %d != %d\n", transfer.actualLength(), mBufferSize);
            mBuffersFree.push(mCurrentBuffer);
            mCurrentBuffer = nullptr;
        }
    }

    uchar *buffer = mBuffersFree.pop(100);
    if (buffer == nullptr)
    {
        fprintf(stderr, "[CameraBoost::startAsyncXfer]: buffer timeout, exiting...\n");
        assert(true); // TODO
        return;
    }
    transfer.setBuffer(buffer).submit();


    // You can validate the buffer size here.
    // resetDeviceNeeded

    *bytesRead = size; // trigger 'OK'

    mTransferIndex = (mTransferIndex + 1) % Transfers;
}

int CameraBoost::InsertBuff(uchar *buffer, int bufferSize, ushort v1, int i1, ushort v2, int i2, int i3, int i4)
{
    if (mCurrentBuffer == nullptr)
        return 0;

    // This is where you can validate the data in the buffer.
    // resetDeviceNeeded
    ushort * b16 = reinterpret_cast<ushort *>(mCurrentBuffer);

    if (
        (v1 != 0 && b16[i1] != v1) ||
        (v2 != 0 && b16[i2] != v2)
    )
    {
        mBuffersFree.push(mCurrentBuffer);
        fprintf(stderr, "[CameraBoost::InsertBuff]: [%d]=%02x (%02x), [%d]=%02x (%02x)\n", i1, b16[i1], v1, i2, b16[i2], v2);
        return 0;
    }

    if (i3 != 0 && i4 != 0 && b16[i3] != b16[i4])
    {
        fprintf(stderr, "[CameraBoost::InsertBuff]: [%d]=%02x, [%d]=%02x\n", i3, b16[i3], i4, b16[i4]);
        mBuffersFree.push(mCurrentBuffer);
        //return 2;
        return 0;
    }

    mBuffersReady.push(mCurrentBuffer);

    return 0;
}

void CameraBoost::ResetDevice()
{
    fprintf(stderr, "[CameraBoost::ResetDevice]: catched\n");
    resetDeviceNeeded = false;
    for(LibUsbChunkedBulkTransfer &transfer: mTransfer)
    {
        transfer.cancel();
        mBuffersFree.push(reinterpret_cast<uchar*>(transfer.buffer()));
        transfer.setBuffer(nullptr);
    }
}

void CameraBoost::releaseAsyncXfer()
{
    fprintf(stderr, "[CameraBoost::releaseAsyncXfer]\n");
    for(LibUsbChunkedBulkTransfer &transfer: mTransfer)
    {
        transfer.cancel();
        mBuffersFree.push(reinterpret_cast<uchar*>(transfer.buffer()));
        transfer.setBuffer(nullptr);
    }

    *usbBuffer = realUsbBuffer;
    *imageBuffer = realImageBuffer;
}

int CameraBoost::ReadBuff(uchar* buffer, uint size, uint timeout)
{
    if (imageBuffer == nullptr)
    {
        imageBuffer = find_pointer_address(mCCameraBase, 0x600, buffer);
        realImageBuffer = buffer;
        assert(("[CirBuf::ReadBuff]: cannot find image buffer", imageBuffer != nullptr));
    }

    uchar *p = get(); // TODO timeout
    //fprintf(stderr, "[CirBuf::ReadBuff]: %d %d %p\n", *(short*)&p[0x00000000 + 2], *(short*)&p[0x006242f0 + 12], buffer);

    *imageBuffer = p;

    return 1;
}
