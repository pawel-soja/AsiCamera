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
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <fstream>

#include "ASICamera2Headers.h"
#include "cameraboost.h"

static std::mutex gBoostCameraDataMutex;
static std::unordered_map<int,          CameraBoost>  gCameraBoost;
static std::unordered_map<CCameraBase*, CameraBoost*> gCameraBoost_CCameraBase;
static std::unordered_map<CirBuf*,      CameraBoost*> gCameraBoost_CirBuf;
static int gActiveCameraID = -1;

static CameraBoost *getCameraBoost(int iCameraID = gActiveCameraID)
{
    std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    if (iCameraID == -1)
    {
        err_printf("invalid iCameraID!");
        abort();
    }
    // dbg_printf("iCameraID %d", iCameraID);
    return &gCameraBoost[iCameraID];
}

static CameraBoost *& getCameraBoost(CCameraFX3 *ccameraFX3)
{
    //std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    CCameraBase *ccameraBase = static_cast<CCameraBase*>(ccameraFX3);
    // dbg_printf("CCameraFX3 %p", ccameraFX3);
    return gCameraBoost_CCameraBase[ccameraBase];
}

static CameraBoost *& getCameraBoost(CirBuf *cirBuf)
{
    //std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    // dbg_printf("CirBuf %p", cirBuf);
    return gCameraBoost_CirBuf[cirBuf];
}

extern "C"
{

// #1
ASI_ERROR_CODE __real_ASIOpenCamera(int iCameraID);
ASI_ERROR_CODE ASIOpenCamera(int iCameraID)
{
    if (gCameraBoostEnable)
    {
        dbg_printf("grab CameraID %d", iCameraID);
        gActiveCameraID = iCameraID;
        getCameraBoost()->mCameraID = iCameraID;
    }
    return __real_ASIOpenCamera(iCameraID);
}

// #2
void __real__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase);
void __wrap__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase)
{
    if (gCameraBoostEnable)
    {
        dbg_printf("grab CCameraBase %p", ccameraBase);
        CameraBoost *cameraBoost = getCameraBoost();
        cameraBoost->mCCameraBase = ccameraBase;
        gCameraBoost_CCameraBase[ccameraBase] = cameraBoost;
    }
    __real__ZN11CCameraBase12InitVariableEv(ccameraBase);
}

// #3
int __real_libusb_open(libusb_device *dev, libusb_device_handle **devh);
int __wrap_libusb_open(libusb_device *dev, libusb_device_handle **devh)
{
    int rc = __real_libusb_open(dev, devh);
    if (gCameraBoostEnable)
    {
        if (gActiveCameraID != -1)
        {
            dbg_printf("grab libusb_device_handle %p", *devh);
            getCameraBoost()->mDeviceHandle = *devh;
        }
        else
        {
            dbg_printf("grab libusb_device_handle %p, skipping", *devh);
        }
    }

    return rc;
}

// #4
void __real__ZN6CirBufC1El(CirBuf *cirBuf, long size);
void __wrap__ZN6CirBufC1El(CirBuf *cirBuf, long size)
{
    if (gCameraBoostEnable)
    {
        CameraBoost *cameraBoost = getCameraBoost();
        cameraBoost->mCirBuf = cirBuf;
        gCameraBoost_CirBuf[cirBuf] = cameraBoost;
        gActiveCameraID = -1; // done
    }

    __real__ZN6CirBufC1El(cirBuf, size);
}

// CCameraFX3::ResetDevice
int __real__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3);
int __wrap__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3)
{
    if (gCameraBoostEnable)
        getCameraBoost(ccameraFX3)->ResetDevice();

    return __real__ZN10CCameraFX311ResetDeviceEv(ccameraFX3);
}

// CCameraFX3::initAsyncXfer
void __real__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer);
void __wrap__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer)
{
    if (!gCameraBoostEnable)
        return __real__ZN10CCameraFX313initAsyncXferEiiihPh(ccameraFX3, bufferSize, transferCount, chunkSize, endpoint, buffer);

    getCameraBoost(ccameraFX3)->initAsyncXfer(bufferSize, transferCount, chunkSize, endpoint, buffer);
}

// CCameraFX3::startAsyncXfer
void __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size);
void __wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, uint timeout1, uint timeout2, int *bytesRead, bool *stop, int size)
{
    if (!gCameraBoostEnable)
        return __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(ccameraFX3, timeout1, timeout2, bytesRead, stop, size);

    getCameraBoost(ccameraFX3)->startAsyncXfer(timeout1, timeout2, bytesRead, stop, size);
}

// CirBuf::InsertBuff
int __real__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int size, ushort v1, int i1, ushort v2, int i2, int i3, int i4);
int __wrap__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int size, ushort v1, int i1, ushort v2, int i2, int i3, int i4)
{
    if (!gCameraBoostEnable)
        return __real__ZN6CirBuf10InsertBuffEPhititiii(cirBuf, buffer, size, v1, i1, v2, i2, i3, i4);

    return getCameraBoost(cirBuf)->InsertBuff(buffer, size, v1, i1, v2, i2, i3, i4);
}

// CirBuf::ReadBuff
int __real__ZN6CirBuf8ReadBuffEPhii(CirBuf *cirBuf, uchar* buffer, uint size, uint timeout);
int __wrap__ZN6CirBuf8ReadBuffEPhii(CirBuf *cirBuf, uchar* buffer, uint size, uint timeout)
{
    if (!gCameraBoostEnable)
        return __real__ZN6CirBuf8ReadBuffEPhii(cirBuf, buffer, size, timeout);

    return getCameraBoost(cirBuf)->ReadBuff(buffer, size, timeout);
}

void __real__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3);
void __wrap__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3)
{
    if (!gCameraBoostEnable)
        return __real__ZN10CCameraFX316releaseAsyncXferEv(ccameraFX3);

    getCameraBoost(ccameraFX3)->releaseAsyncXfer();
}

ASICAMERA_API ASI_ERROR_CODE ASIGetVideoDataPointer(int iCameraID, unsigned char** pBuffer, int iWaitms)
{
    if (!gCameraBoostEnable)
        return ASI_ERROR_INVALID_MODE;

    CameraBoost *cameraBoost = getCameraBoost(iCameraID);

    unsigned char * p = cameraBoost->peek(iWaitms);
    if (p == nullptr)
        return ASI_ERROR_TIMEOUT;

    // dbg_printf("buffer size %u", uint(cameraBoost->bufferSize()));
    ASIGetVideoData(iCameraID, p, cameraBoost->bufferSize(), iWaitms);
    *pBuffer = p;
    
    return ASI_SUCCESS;
}

ASICAMERA_API ASI_ERROR_CODE ASISetMaxChunkSize(int iCameraID, unsigned int maxChunkSize)
{
    if (!gCameraBoostEnable)
        return ASI_ERROR_INVALID_MODE;

    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    return cameraBoost->setMaxChunkSize(maxChunkSize) ? ASI_SUCCESS : ASI_ERROR_INVALID_SEQUENCE;
}

ASICAMERA_API ASI_ERROR_CODE ASISetChunkedTransfers(int iCameraID, unsigned int chunkedTransfers)
{
    if (!gCameraBoostEnable)
        return ASI_ERROR_INVALID_MODE;

    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    return cameraBoost->setChunkedTransfers(chunkedTransfers) ? ASI_SUCCESS : ASI_ERROR_INVALID_SEQUENCE;
}


ASICAMERA_API ASI_ERROR_CODE __real_ASIGetNumOfControls(int iCameraID, int * piNumberOfControls);
ASICAMERA_API ASI_ERROR_CODE ASIGetNumOfControls(int iCameraID, int * piNumberOfControls)
{
    ASI_ERROR_CODE ret = __real_ASIGetNumOfControls(iCameraID, piNumberOfControls);

    if (ret != ASI_SUCCESS)
        return ret;

    if (piNumberOfControls)
    {
        *piNumberOfControls += 4;
        if(std::ifstream("/sys/module/usbcore/parameters/usbfs_memory_mb").good())
            *piNumberOfControls += 1;
    }
    return ret;
}

ASICAMERA_API ASI_ERROR_CODE __real_ASIGetControlCaps(int iCameraID, int iControlIndex, ASI_CONTROL_CAPS * pControlCaps);
ASICAMERA_API ASI_ERROR_CODE ASIGetControlCaps(int iCameraID, int iControlIndex, ASI_CONTROL_CAPS * pControlCaps)
{
    int numberOfControls;
    ASI_ERROR_CODE ret = __real_ASIGetNumOfControls(iCameraID, &numberOfControls);
    if (ret != ASI_SUCCESS)
        return ret;

    switch(iControlIndex - numberOfControls)
    {
    case 0:
        strcpy(pControlCaps->Name, "CameraBoostDebug");
        strcpy(pControlCaps->Description, "Enable debug mode for Boost");
        pControlCaps->MaxValue = 1;
        pControlCaps->MinValue = 0;
        pControlCaps->DefaultValue = 0;
        pControlCaps->IsAutoSupported = ASI_FALSE;
        pControlCaps->IsWritable = ASI_TRUE;
        pControlCaps->ControlType = ASI_CONTROL_TYPE(128);
        return ASI_SUCCESS;
    case 1:
        strcpy(pControlCaps->Name, "CameraDebug");
        strcpy(pControlCaps->Description, "Enable debug mode for ASICamera2");
        pControlCaps->MaxValue = 1;
        pControlCaps->MinValue = 0;
        pControlCaps->DefaultValue = 0;
        pControlCaps->IsAutoSupported = ASI_FALSE;
        pControlCaps->IsWritable = ASI_TRUE;
        pControlCaps->ControlType = ASI_CONTROL_TYPE(129);
        return ASI_SUCCESS;
    case 2:
        strcpy(pControlCaps->Name, "MaxChunkSize");
        strcpy(pControlCaps->Description, "Size limit of a single USB transfer(MB)");
        pControlCaps->MaxValue = 256;
        pControlCaps->MinValue =   1;
        pControlCaps->DefaultValue = CameraBoost::DefaultChunkSize / 1024 / 1024;
        pControlCaps->IsAutoSupported = ASI_FALSE;
        pControlCaps->IsWritable = ASI_TRUE;
        pControlCaps->ControlType = ASI_CONTROL_TYPE(130);
        return ASI_SUCCESS;
    case 3:
        strcpy(pControlCaps->Name, "ChunkedTransfers");
        strcpy(pControlCaps->Description, "Number of chunked transfers");
        pControlCaps->MaxValue = 8;
        pControlCaps->MinValue =   1;
        pControlCaps->DefaultValue = CameraBoost::DefaultChunkedTransfers;
        pControlCaps->IsAutoSupported = ASI_FALSE;
        pControlCaps->IsWritable = ASI_TRUE;
        pControlCaps->ControlType = ASI_CONTROL_TYPE(131);
        return ASI_SUCCESS;
    case 4: {
        std::ifstream file("/sys/module/usbcore/parameters/usbfs_memory_mb");
        strcpy(pControlCaps->Name, "USBMemory");
        strcpy(pControlCaps->Description, "/sys/module/usbcore/parameters/usbfs_memory_mb");
        pControlCaps->MaxValue = 4096;
        pControlCaps->MinValue = 0;
        //pControlCaps->DefaultValue = 0;
        file >> pControlCaps->DefaultValue;
        pControlCaps->IsAutoSupported = ASI_FALSE;
        pControlCaps->IsWritable = ASI_TRUE;
        pControlCaps->ControlType = ASI_CONTROL_TYPE(132);
        return ASI_SUCCESS;
    }
    }

    return __real_ASIGetControlCaps(iCameraID, iControlIndex, pControlCaps);
}

ASICAMERA_API ASI_ERROR_CODE __real_ASIGetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long *plValue, ASI_BOOL *pbAuto);
ASICAMERA_API ASI_ERROR_CODE ASIGetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long *plValue, ASI_BOOL *pbAuto)
{
    const CameraBoost *cameraBoost = getCameraBoost(iCameraID);

    switch (int(ControlType))
    {
    case 128:
        if (plValue) *plValue = gCameraBoostDebug;
        if (pbAuto) *pbAuto = ASI_FALSE;
        return ASI_SUCCESS;

    case 129:
        if (plValue) *plValue = g_bDebugPrint;
        if (pbAuto) *pbAuto = ASI_FALSE;
        return ASI_SUCCESS;

    case 130:
        if (plValue) *plValue = cameraBoost->getMaxChunkSize() / 1024 / 1024;
        if (pbAuto) *pbAuto = ASI_FALSE;
        return ASI_SUCCESS;

    case 131:
        if (plValue) *plValue = cameraBoost->getChunkedTransfers();
        if (pbAuto) *pbAuto = ASI_FALSE;
        return ASI_SUCCESS;

    case 132:
        if (plValue) std::ifstream("/sys/module/usbcore/parameters/usbfs_memory_mb") >> *plValue;
        if (pbAuto) *pbAuto = ASI_FALSE;
    default:;
    }

    return __real_ASIGetControlValue(iCameraID, ControlType, plValue, pbAuto);
}

ASICAMERA_API ASI_ERROR_CODE __real_ASISetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long lValue, ASI_BOOL bAuto);
ASICAMERA_API ASI_ERROR_CODE ASISetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long lValue, ASI_BOOL bAuto)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);

    switch (int(ControlType))
    {
    case 128:
        gCameraBoostDebug = lValue;
        return ASI_SUCCESS;

    case 129:
        g_bDebugPrint = lValue;
        return ASI_SUCCESS;

    case 130:
        return cameraBoost->setMaxChunkSize(lValue * 1024 * 1024) ? ASI_SUCCESS : ASI_ERROR_INVALID_SEQUENCE;

    case 131:
        return cameraBoost->setChunkedTransfers(lValue) ? ASI_SUCCESS : ASI_ERROR_INVALID_SEQUENCE;

    case 132: {
        std::ofstream file("/sys/module/usbcore/parameters/usbfs_memory_mb");
        if (!file.good())
            return ASI_ERROR_GENERAL_ERROR;

        file << lValue;

        return ASI_SUCCESS;
    }
    default:;
    }

    return __real_ASISetControlValue(iCameraID, ControlType, lValue, bAuto);
}

#ifndef NDEBUG
void * __real_memcpy ( void * destination, const void * source, size_t num) __attribute__((weak));
void * __real_memcpy ( void * destination, const void * source, size_t num);
void * __wrap_memcpy ( void * destination, const void * source, size_t num)
{
    if (source != destination)
    {
        if (num >= 1024*1024)
            dbg_printf("Big copy detected: %u Bytes", uint(num));
    }
    return __real_memcpy(destination, source, num);
}
#endif

}
