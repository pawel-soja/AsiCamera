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

#ifdef NDEBUG
# define dbg_printf(...)
#else
# define dbg_printf(f_, ...) fprintf(stderr, ("[%s]: " f_ "\n"), __FUNCTION__, ## __VA_ARGS__)
#endif

# define err_printf(f_, ...) fprintf(stderr, ("[%s]: " f_ "\n"), __FUNCTION__, ## __VA_ARGS__)


class CCameraFX3;
class CirBuf;
class CCameraBase;
class CameraBoost
{
public:
    enum // constants
    {
        Transfers = 3,   // TODO limit to maximum possible transfers
        InitialBuffers = Transfers + 1,
        MaximumTransferChunkSize = 1024*1024 // 
    };

public:
    CameraBoost();
    size_t bufferSize() const;

    uchar *get();
    uchar *peek();

public:
    bool grow();
    void initialBuffers();

public: // reimplement
    void initAsyncXfer(int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer);
    void startAsyncXfer(uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size);
    void releaseAsyncXfer();

    void ResetDevice();

    int ReadBuff(uchar* buffer, uint size, uint timeout);
    int InsertBuff(uchar *buffer, int i1, ushort v1, int i2, ushort v2, int a5, int a6, int a7);

protected:
    std::deque<std::vector<uchar>> mBuffer;
    std::mutex mBufferMutex;
    
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
