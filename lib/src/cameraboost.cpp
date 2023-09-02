/*
    Copyright (C) 2020 by Pawel Soja <kernel32.pl@gmail.com>

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

bool gCameraBoostDebug = false;
bool gCameraBoostEnable = true;

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

#include <map>
#include <fstream>
#include <sstream>
class ConfigFile
{
    std::map<std::string, std::string> values;
public:
    ConfigFile(const std::string &fileName)
    {
        std::ifstream fileStream(fileName);
        std::string line;
        while (std::getline(fileStream, line))
        {
            std::istringstream sline(line);
            std::string key, value;
            if (std::getline(sline, key, '=') && key[0] != '#' && std::getline(sline, value))
                    values[key] = value;
        }
    }

    std::string value(const std::string &key, const std::string &defaultValue = "") const
    {
        const auto value = values.find(key);
        return value == values.end() ? defaultValue : value->second;
    }

    int value(const std::string &key, int defaultValue)
    {
        const auto value = values.find(key);
        if (value == values.end())
            return defaultValue;
        try {
            return std::stoi(value->second);
        } catch (const std::exception&) {
            return defaultValue;
        }
    }
};
CameraBoost::CameraBoost()
{
    dbg_printf("created, load default values or from '/etc/asicamera2boost.conf' file...");
    ConfigFile configFile("/etc/asicamera2boost.conf");

    mIsRunning = false;
    setMaxChunkSize(configFile.value("max_chunk_size", DefaultChunkSize));
    setChunkedTransfers(configFile.value("chunked_transfers", DefaultChunkedTransfers));
}

size_t CameraBoost::bufferSize() const
{
    return mBufferSize;
}

uchar *CameraBoost::get(unsigned int timeout)
{
    uchar *buffer = mBuffersReady.pop(timeout);  // take ready buffer, TODO timeout
    mBuffersFree.push(mBuffersBusy.pop(0)); // move processed buffer to free buffer queue
    mBuffersBusy.push(buffer);              // add current buffer to busy queue
    return buffer;
}

uchar *CameraBoost::peek(unsigned int timeout)
{
    return mBuffersReady.peek(timeout);
}

bool CameraBoost::grow()
{
    std::vector<uchar> newBuffer;
    try {
        newBuffer.resize(mBufferSize);
    }
    catch (const std::bad_alloc&)
    {
        err_printf("Out of memory");
        return false;
    }
    mBuffersFree.push(newBuffer.data());

    std::lock_guard<std::mutex> lock(mBufferMutex);
    mBuffer.push_back(std::move(newBuffer));
    return true;
}

void CameraBoost::initialBuffers()
{
    const unsigned int initialBuffers = mChunkedTransfers + 1;
#ifdef CAMERABOOST_LAZYLOAD
    std::thread([this](){
#endif
        while (mBuffer.size() < initialBuffers && grow());
#ifdef CAMERABOOST_LAZYLOAD
    }).detach();
#endif
}

void CameraBoost::release()
{
    if (usbBuffer && realUsbBuffer)
    {
        dbg_printf("restore usb   buffer: *%p <- %p", usbBuffer, realUsbBuffer);
        *usbBuffer = realUsbBuffer;
    }

    if (imageBuffer && realImageBuffer)
    {
        dbg_printf("restore image buffer: *%p <- %p", imageBuffer, realImageBuffer);
        *imageBuffer = realImageBuffer;
    }
    realUsbBuffer = nullptr;
    realImageBuffer = nullptr;

}

// reimplement

void CameraBoost::initAsyncXfer(int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer)
{
    dbg_printf("device %p, endpoint 0x%x, buffer size %d", mDeviceHandle, endpoint, bufferSize);
    mIsRunning = true;

    resetDeviceNeeded = false;
    mBufferSize = bufferSize;

    // extend buffer size - better damaged frame than retransmission
#if 0
    bufferSize  = (bufferSize + mMaxChunkSize - 1) / mMaxChunkSize;
    bufferSize *= mMaxChunkSize;
#endif
    mBuffersBusy.clear();
    mBuffersReady.clear();
    mBuffersFree.clear();

    // Lazy load
    initialBuffers();

    if (realUsbBuffer == nullptr)
    {
        usbBuffer = find_pointer_address(mCCameraBase, 0x600, buffer);
        dbg_printf("find usb buffer: *%p == %p", usbBuffer, buffer);
        dbg_printf("usb buffer offset: 0x%04x", static_cast<uint>(reinterpret_cast<ptrdiff_t>(usbBuffer) - reinterpret_cast<ptrdiff_t>(mCCameraBase)));
        if (usbBuffer == nullptr)
        {
            err_printf("cannot find usb buffer");
            abort();
        }
        realUsbBuffer = *usbBuffer;

        imageBuffer = usbBuffer - 2; // trust me...
        realImageBuffer = *imageBuffer;

        //delete[] static_cast<uint16_t*>(realUsbBuffer);
        //delete[] static_cast<uint16_t*>(realImageBuffer);
    }

    mInvalidDataFrames = 0;

    dbg_printf("create chunked bulk transfer, bufferSize: %d, chunkSize: %d", bufferSize, mMaxChunkSize);
    mTransfer.resize(mChunkedTransfers);

    for (auto &transfer: mTransfer)
    {
        transfer = LibUsbChunkedBulkTransfer(mDeviceHandle, endpoint, NULL, bufferSize, mMaxChunkSize);
#ifndef CAMERABOOST_DISABLE_PRETRANSFER
        submitTransfer(transfer, -1);
#endif
    }
#ifndef CAMERABOOST_DISABLE_PRETRANSFER
    mChunkedTransferIndex = 0;
#else
    mChunkedTransferIndex = -1;
#endif
}

bool CameraBoost::submitTransfer(LibUsbChunkedBulkTransfer &transfer, unsigned int timeout)
{
    uchar *buffer;
    while ((buffer = mBuffersFree.pop(2)) == nullptr)
    {
        if (grow() == false)
            return false;
    }
    transfer.setBuffer(buffer).submit();
    return true;
}

void CameraBoost::startAsyncXfer(unsigned int timeout1, unsigned int timeout2, int *bytesRead, bool *stop, int size)
{
    if (resetDeviceNeeded)
    {
        mInvalidDataFrames = 0;
        return;
    }

    // I suggest at least a second because of the start of the first frame.
    unsigned int timeout = std::max(timeout1, uint(1000)); // minimum second.

    // is first call?
    if (mChunkedTransferIndex < 0)
    {
        mChunkedTransferIndex = 0;
        // Submit all transfers - required for long exposure.
        for(auto &transfer: mTransfer)
            if(submitTransfer(transfer, timeout) == false)
                return;
    }

    LibUsbChunkedBulkTransfer &transfer = mTransfer[mChunkedTransferIndex];

    mCurrentBuffer = reinterpret_cast<uchar*>(transfer.wait(timeout).buffer());

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

    if(submitTransfer(transfer, timeout) == false)
        return;

    *bytesRead = size;
    *usbBuffer = mCurrentBuffer;
    dbg_printf("swap usb   buffer: *%p <- %p", usbBuffer, mCurrentBuffer);
    mChunkedTransferIndex = (mChunkedTransferIndex + 1) % mChunkedTransfers;
}

int CameraBoost::InsertBuff(uchar *buffer, int bufferSize, ushort v1, int i1, ushort v2, int i2, int i3, int i4)
{
    UNUSED(buffer);

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
        if (++mInvalidDataFrames >= 2)
        {
            dbg_printf("invalid frame tokens, reset device needed");
            resetDeviceNeeded = true;
        }
        return 0;
    }

    if (i3 != 0 && i4 != 0 && b16[i3] != b16[i4])
    {
        dbg_printf("[%d]=%02x, [%d]=%02x", i3, b16[i3], i4, b16[i4]);
        mBuffersFree.push(mCurrentBuffer);
        if (++mInvalidDataFrames >= 2)
        {
            dbg_printf("invalid frame index, reset device needed");
            resetDeviceNeeded = true;
        }
        //return 2;
        return 0;
    }

    dbg_printf("ready: %p", mCurrentBuffer);
    mBuffersReady.push(mCurrentBuffer);

    return 0;
}

void CameraBoost::ResetDevice()
{
    dbg_printf("catched");
    for(LibUsbChunkedBulkTransfer &transfer: mTransfer)
    {
        transfer.cancel();
        mBuffersFree.push(reinterpret_cast<uchar*>(transfer.buffer()));
        transfer.setBuffer(nullptr);
    }
    mChunkedTransferIndex = -1;
    resetDeviceNeeded = false;
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

    mIsRunning = false;
}

void* CameraBoost::prepareToRead(unsigned int timeout)
{
    timeout = std::max(timeout, uint(1000)); // minimum second.
    uchar *p = get(timeout);

    if (p == nullptr)
    {
        dbg_printf("timeout");
        return nullptr;
    }

    *imageBuffer = p;
    dbg_printf("swap image buffer: *%p <- %p", imageBuffer, p);

    return p;
}

int CameraBoost::ReadBuff(uchar* buffer, unsigned int size, unsigned int timeout)
{
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(timeout);

    dbg_printf("stub");
    // the timeout value comes from the user (iWaitms).
    // ASIGetVideoData(int iCameraID, unsigned char* pBuffer, long lBuffSize, int iWaitms);
    // quotation:
    //    int iWaitms, this API will block and wait iWaitms to get one image.
    //    the unit is ms -1 means wait forever. this value is recommend set to exposure*2+500ms
    //
    // this is not true. The camera is also limited to the maximum FPS.
    // For example, for 32us exposure, the frame rate will not be several thousand.
    // I suggest at least a second because of the start of the first frame.
    //
    // see bool CameraBoost::prepareToRead(unsigned int timeout)
    return 1;
}

unsigned int CameraBoost::getMaxChunkSize() const
{
    return mMaxChunkSize;
}

unsigned int CameraBoost::getChunkedTransfers() const
{
    return mChunkedTransfers;
}

bool CameraBoost::setMaxChunkSize(unsigned int value)
{
    if (mIsRunning)
        return false;

    mMaxChunkSize = value;
    dbg_printf("max_chunk_size %d", mMaxChunkSize);
    return true;
}

bool CameraBoost::setChunkedTransfers(unsigned int value)
{
    if (mIsRunning)
        return false;

    mChunkedTransfers = value;
    dbg_printf("chunked_transfers %d", mChunkedTransfers);
    return true;
}
