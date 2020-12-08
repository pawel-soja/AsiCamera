/*
    Copyright (C) 2020 by Pawel Soja <kernel32.pl@gmail.com>
    FPS Meter
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
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
    dbg_printf("created");
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
    dbg_printf("device %p, endpoint 0x%x, buffer size %d", mDeviceHandle, endpoint, bufferSize);
    
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
            if (buffer.size() != size_t(bufferSize))
            {
                err_printf("allocation fail");
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
            dbg_printf("null transfer length, reset device needed");
            resetDeviceNeeded = true;
            mBuffersFree.push(mCurrentBuffer);
            mCurrentBuffer = nullptr;
            return;
        }
        if (transfer.actualLength() != mBufferSize)
        {
            dbg_printf("invalid transfer length %u != %u", transfer.actualLength(), uint(mBufferSize));
            mBuffersFree.push(mCurrentBuffer);
            mCurrentBuffer = nullptr;
        }
    }

    uchar *buffer = mBuffersFree.pop(100);
    if (buffer == nullptr)
    {
        dbg_printf("buffer timeout, exiting...");
        //abort();
        return;
    }
    transfer.setBuffer(buffer).submit();

    *bytesRead = size;

    mTransferIndex = (mTransferIndex + 1) % Transfers;
}

int CameraBoost::InsertBuff(uchar *buffer, int bufferSize, ushort v1, int i1, ushort v2, int i2, int i3, int i4)
{
    if (mCurrentBuffer == nullptr)
        return 0;

    ushort * b16 = reinterpret_cast<ushort *>(mCurrentBuffer);

    if (
        (v1 != 0 && b16[i1] != v1) ||
        (v2 != 0 && b16[i2] != v2)
    )
    {
        mBuffersFree.push(mCurrentBuffer);
        dbg_printf("[%d]=%02x (%02x), [%d]=%02x (%02x)", i1, b16[i1], v1, i2, b16[i2], v2);
        return 0;
    }

    if (i3 != 0 && i4 != 0 && b16[i3] != b16[i4])
    {
        dbg_printf("[%d]=%02x, [%d]=%02x", i3, b16[i3], i4, b16[i4]);
        mBuffersFree.push(mCurrentBuffer);
        //return 2;
        return 0;
    }

    mBuffersReady.push(mCurrentBuffer);

    return 0;
}

void CameraBoost::ResetDevice()
{
    dbg_printf("catched");
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
    dbg_printf("catched");
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
        if (imageBuffer == nullptr)
        {
            err_printf("cannot find image buffer");
            abort();
        }
        realImageBuffer = buffer;
    }

    uchar *p = get(); // TODO timeout

    *imageBuffer = p;

    return 1;
}
