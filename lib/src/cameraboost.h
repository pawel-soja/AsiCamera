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
#pragma once

#include <vector>
#include <atomic>
#include <deque>

#include <libusb-cpp/libusb.h>

#include "queue.h"
#include "stypes.h"

extern bool gCameraBoostDebug;
extern bool gCameraBoostEnable;

# define dbg_printf(f_, ...) \
do { \
    if (gCameraBoostDebug) fprintf(stderr, ("[boost %s]: " f_ "\n"), __FUNCTION__, ## __VA_ARGS__); \
} while(0)

# define err_printf(f_, ...) fprintf(stderr, ("[boost %s]: " f_ "\n"), __FUNCTION__, ## __VA_ARGS__)

#ifndef UNUSED
# define UNUSED(x) (void)x
#endif

class CCameraFX3;
class CirBuf;
class CCameraBase;
class CameraBoost
{
public:
    enum {
        DefaultChunkSize = 128 * 1024 * 1024,
        DefaultChunkedTransfers = 2
    };
public:
    CameraBoost();
    size_t bufferSize() const;

    uchar *get(uint timeout);
    uchar *peek(uint timeout);

public:
    bool grow();
    void initialBuffers();
    void release();

    void* prepareToRead(uint timeout);

public: // reimplement
    void initAsyncXfer(int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer);
    void startAsyncXfer(uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size);
    void releaseAsyncXfer();

    void ResetDevice();

    int ReadBuff(uchar* buffer, uint size, uint timeout);
    int InsertBuff(uchar *buffer, int i1, ushort v1, int i2, ushort v2, int a5, int a6, int a7);

    unsigned int getMaxChunkSize() const;
    unsigned int getChunkedTransfers() const;

public: // export in wrap functions
    bool setMaxChunkSize(unsigned int value);
    bool setChunkedTransfers(unsigned int value);

private:
    bool submitTransfer(LibUsbChunkedBulkTransfer &transfer, uint timeout);

protected:
    std::deque<std::vector<uchar>> mBuffer;
    std::mutex mBufferMutex;
    
    Queue<uchar*> mBuffersFree;
    Queue<uchar*> mBuffersReady;
    Queue<uchar*> mBuffersBusy;

    uchar *mCurrentBuffer = nullptr;

    std::vector<LibUsbChunkedBulkTransfer>  mTransfer;
    int mChunkedTransferIndex = 0;

    uint mMaxChunkSize;
    uint mChunkedTransfers;

    std::atomic<bool> mIsRunning;

    uint mInvalidDataFrames = 0;
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
