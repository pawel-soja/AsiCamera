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
static std::unordered_map<int,          CameraBoost*>  gCameraBoost;
static std::unordered_map<CCameraBase*, CameraBoost*> gCameraBoost_CCameraBase;
static std::unordered_map<CirBuf*,      CameraBoost*> gCameraBoost_CirBuf;


static const char *toString(ASI_CONTROL_TYPE value)
{
    switch (int(value))
    {
    case ASI_GAIN: return "ASI_GAIN";
    case ASI_EXPOSURE: return "ASI_EXPOSURE";
    case ASI_GAMMA: return "ASI_GAMMA";
    case ASI_WB_R: return "ASI_WB_R";
    case ASI_WB_B: return "ASI_WB_B";
    case ASI_OFFSET: return "ASI_OFFSET";
    case ASI_BANDWIDTHOVERLOAD: return "ASI_BANDWIDTHOVERLOAD";
    case ASI_OVERCLOCK: return "ASI_OVERCLOCK";
    case ASI_TEMPERATURE: return "ASI_TEMPERATURE";
    case ASI_FLIP: return "ASI_FLIP";
    case ASI_AUTO_MAX_GAIN: return "ASI_AUTO_MAX_GAIN";
    case ASI_AUTO_MAX_EXP: return "ASI_AUTO_MAX_EXP";
    case ASI_AUTO_TARGET_BRIGHTNESS: return "ASI_AUTO_TARGET_BRIGHTNESS";
    case ASI_HARDWARE_BIN: return "ASI_HARDWARE_BIN";
    case ASI_HIGH_SPEED_MODE: return "ASI_HIGH_SPEED_MODE";
    case ASI_COOLER_POWER_PERC: return "ASI_COOLER_POWER_PERC";
    case ASI_TARGET_TEMP: return "ASI_TARGET_TEMP";
    case ASI_COOLER_ON: return "ASI_COOLER_ON";
    case ASI_MONO_BIN: return "ASI_MONO_BIN";
    case ASI_FAN_ON: return "ASI_FAN_ON";
    case ASI_PATTERN_ADJUST: return "ASI_PATTERN_ADJUST";
    case ASI_ANTI_DEW_HEATER: return "ASI_ANTI_DEW_HEATER";
    case 128: return "BOOST_ASI_CAMERA_BOOST_DEBUG";
    case 129: return "BOOST_ASI_CAMERA_DEBUG";
    case 130: return "BOOST_ASI_MAX_CHUNK_SIZE";
    case 131: return "BOOST_ASI_CHUNKED_TRANSFERS";
    case 132: return "BOOST_ASI_USBFS_MEMORY_MB";
    }
    return "Unknown";
}

static CameraBoost *newCameraBoost(int iCameraID)
{
    std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    CameraBoost *cameraBoost = new CameraBoost();
    cameraBoost->mCameraID = iCameraID;
    gCameraBoost[iCameraID] = cameraBoost;
    return cameraBoost;
}

static void delCameraBoost(int iCameraID)
{
    std::lock_guard<std::mutex> lock(gBoostCameraDataMutex);
    delete gCameraBoost[iCameraID];
    gCameraBoost[iCameraID] = nullptr;
}

static CameraBoost *getCameraBoost(int iCameraID)
{
    return gCameraBoost[iCameraID];
}

static CameraBoost *& getCameraBoost(CCameraFX3 *ccameraFX3)
{
    CCameraBase *ccameraBase = static_cast<CCameraBase*>(ccameraFX3);
    // dbg_printf("CCameraFX3 %p", ccameraFX3);
    return gCameraBoost_CCameraBase[ccameraBase];
}

static CameraBoost *& getCameraBoost(CirBuf *cirBuf)
{
    // dbg_printf("CirBuf %p", cirBuf);
    return gCameraBoost_CirBuf[cirBuf];
}

extern "C"
{

ASI_ERROR_CODE __real_ASICloseCamera(int iCameraID);
ASI_ERROR_CODE ASICloseCamera(int iCameraID)
{
    dbg_printf("delete CameraID %d", iCameraID);
    delCameraBoost(iCameraID);
    return __real_ASICloseCamera(iCameraID);
}

// #0
static int sOpeningCameraID = -1;;

// #1
ASI_ERROR_CODE __real_ASIOpenCamera(int iCameraID);
ASI_ERROR_CODE ASIOpenCamera(int iCameraID)
{
    if (gCameraBoostEnable)
    {
        dbg_printf("grab CameraID %d", iCameraID);
        sOpeningCameraID = iCameraID;
        newCameraBoost(iCameraID);
    }
    return __real_ASIOpenCamera(iCameraID);
}

// #2
void __real__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase);
void __wrap__ZN11CCameraBase12InitVariableEv(CCameraBase * ccameraBase)
{
    CameraBoost *cameraBoost = getCameraBoost(sOpeningCameraID);
    if (cameraBoost)
    {
        dbg_printf("grab CCameraBase %p", ccameraBase);
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
    CameraBoost *cameraBoost = getCameraBoost(sOpeningCameraID);

    if (cameraBoost)
    {
        dbg_printf("grab libusb_device_handle %p", *devh);
        cameraBoost->mDeviceHandle = *devh;
    }

    return rc;
}

// #4
void __real__ZN6CirBufC1Eli(CirBuf *cirBuf, long size, int v1);
void __wrap__ZN6CirBufC1Eli(CirBuf *cirBuf, long size, int v1)
{
    CameraBoost *cameraBoost = getCameraBoost(sOpeningCameraID);
    if (cameraBoost)
    {
        dbg_printf("grab CirBuf %p", cirBuf);
        cameraBoost->mCirBuf = cirBuf;
        gCameraBoost_CirBuf[cirBuf] = cameraBoost;
        sOpeningCameraID = -1; // done
    }

    __real__ZN6CirBufC1Eli(cirBuf, size, v1);
}

// CameraBase::CloseCamera
void __real__ZN11CCameraBase11CloseCameraEv(CCameraBase * ccameraBase);
void __wrap__ZN11CCameraBase11CloseCameraEv(CCameraBase * ccameraBase)
{
    CameraBoost *cameraBoost = getCameraBoost(ccameraBase);
    if (cameraBoost)
        cameraBoost->release();

    __real__ZN11CCameraBase11CloseCameraEv(ccameraBase);
}

// CCameraFX3::ResetDevice
int __real__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3);
int __wrap__ZN10CCameraFX311ResetDeviceEv(CCameraFX3 *ccameraFX3)
{
    CameraBoost *cameraBoost = getCameraBoost(ccameraFX3);
    if (cameraBoost)
        cameraBoost->ResetDevice();

    return __real__ZN10CCameraFX311ResetDeviceEv(ccameraFX3);
}

// CCameraFX3::initAsyncXfer
void __real__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer);
void __wrap__ZN10CCameraFX313initAsyncXferEiiihPh(CCameraFX3 *ccameraFX3, int bufferSize, int transferCount, int chunkSize, uchar endpoint, uchar *buffer)
{
    CameraBoost *cameraBoost = getCameraBoost(ccameraFX3);
    if (cameraBoost)
        cameraBoost->initAsyncXfer(bufferSize, transferCount, chunkSize, endpoint, buffer);
    else
        __real__ZN10CCameraFX313initAsyncXferEiiihPh(ccameraFX3, bufferSize, transferCount, chunkSize, endpoint, buffer);
}

// CCameraFX3::startAsyncXfer
void __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, unsigned int timeout1, unsigned int timeout2, int *bytesRead, bool *stop, int size);
void __wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi(CCameraFX3 *ccameraFX3, unsigned int timeout1, unsigned int timeout2, int *bytesRead, bool *stop, int size)
{
    CameraBoost *cameraBoost = getCameraBoost(ccameraFX3);
    if (cameraBoost)
        cameraBoost->startAsyncXfer(timeout1, timeout2, bytesRead, stop, size);
    else
        __real__ZN10CCameraFX314startAsyncXferEjjPiPbi(ccameraFX3, timeout1, timeout2, bytesRead, stop, size);
}

// CirBuf::InsertBuff
int __real__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int size, ushort v1, int i1, ushort v2, int i2, int i3, int i4);
int __wrap__ZN6CirBuf10InsertBuffEPhititiii(CirBuf *cirBuf, uchar *buffer, int size, ushort v1, int i1, ushort v2, int i2, int i3, int i4)
{
    CameraBoost *cameraBoost = getCameraBoost(cirBuf);
    if (cameraBoost)
        return cameraBoost->InsertBuff(buffer, size, v1, i1, v2, i2, i3, i4);
    else
        return __real__ZN6CirBuf10InsertBuffEPhititiii(cirBuf, buffer, size, v1, i1, v2, i2, i3, i4);
}

// CirBuf::ReadBuff
int __real__ZN6CirBuf8ReadBuffEPhiiPS0_(CirBuf *cirBuf, uchar* buffer, unsigned int size, unsigned int timeout, unsigned char **v1);
int __wrap__ZN6CirBuf8ReadBuffEPhiiPS0_(CirBuf *cirBuf, uchar* buffer, unsigned int size, unsigned int timeout, unsigned char **v1)
{
    CameraBoost *cameraBoost = getCameraBoost(cirBuf);
    if (cameraBoost)
        return cameraBoost->ReadBuff(buffer, size, timeout);
    else
        return __real__ZN6CirBuf8ReadBuffEPhiiPS0_(cirBuf, buffer, size, timeout, v1);
}

void __real__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3);
void __wrap__ZN10CCameraFX316releaseAsyncXferEv(CCameraFX3 * ccameraFX3)
{
    CameraBoost *cameraBoost = getCameraBoost(ccameraFX3);
    if (cameraBoost)
        cameraBoost->releaseAsyncXfer();
    else
        __real__ZN10CCameraFX316releaseAsyncXferEv(ccameraFX3);
}

ASICAMERA_API ASI_ERROR_CODE __real__ASIGetVideoData(int iCameraID, unsigned char* pBuffer, long lBuffSize, int iWaitms);
ASICAMERA_API ASI_ERROR_CODE ASIGetVideoData(int iCameraID, unsigned char* pBuffer, long lBuffSize, int iWaitms)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    if (cameraBoost)
        cameraBoost->prepareToRead(iWaitms);

    return __real__ASIGetVideoData(iCameraID, pBuffer, lBuffSize, iWaitms);
}

ASICAMERA_API  ASI_ERROR_CODE __real__ASIGetDataAfterExp(int iCameraID, unsigned char* pBuffer, long lBuffSize);
ASICAMERA_API  ASI_ERROR_CODE ASIGetDataAfterExp(int iCameraID, unsigned char* pBuffer, long lBuffSize)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    if (cameraBoost)
        cameraBoost->prepareToRead(100);

    return __real__ASIGetDataAfterExp(iCameraID, pBuffer, lBuffSize);
}

ASICAMERA_API ASI_ERROR_CODE ASIGetVideoDataPointer(int iCameraID, unsigned char** pBuffer, int iWaitms)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    if (!cameraBoost)
        return ASI_ERROR_INVALID_MODE;

    *pBuffer = reinterpret_cast<uchar*>(cameraBoost->prepareToRead(iWaitms));
    if (*pBuffer == nullptr)
        return ASI_ERROR_TIMEOUT;
    // dbg_printf("buffer size %u", uint(cameraBoost->bufferSize()));
    __real__ASIGetVideoData(iCameraID, *pBuffer, cameraBoost->bufferSize(), iWaitms);
    
    return ASI_SUCCESS;
}

ASICAMERA_API ASI_ERROR_CODE ASISetMaxChunkSize(int iCameraID, unsigned int maxChunkSize)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    if (!cameraBoost)
        return ASI_ERROR_INVALID_MODE;

    return cameraBoost->setMaxChunkSize(maxChunkSize) ? ASI_SUCCESS : ASI_ERROR_INVALID_SEQUENCE;
}

ASICAMERA_API ASI_ERROR_CODE ASISetChunkedTransfers(int iCameraID, unsigned int chunkedTransfers)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    if (!cameraBoost)
        return ASI_ERROR_INVALID_MODE;

    return cameraBoost->setChunkedTransfers(chunkedTransfers) ? ASI_SUCCESS : ASI_ERROR_INVALID_SEQUENCE;
}

ASICAMERA_API ASI_ERROR_CODE __real_ASIGetNumOfControls(int iCameraID, int * piNumberOfControls);
ASICAMERA_API ASI_ERROR_CODE ASIGetNumOfControls(int iCameraID, int * piNumberOfControls)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);

    ASI_ERROR_CODE ret = __real_ASIGetNumOfControls(iCameraID, piNumberOfControls);

    if (ret != ASI_SUCCESS || cameraBoost == nullptr)
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
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);

    int numberOfControls;
    ASI_ERROR_CODE ret = __real_ASIGetNumOfControls(iCameraID, &numberOfControls);
    if (ret != ASI_SUCCESS)
        return ret;

    if (cameraBoost)
    {
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
            break;
        case 1:
            strcpy(pControlCaps->Name, "CameraDebug");
            strcpy(pControlCaps->Description, "Enable debug mode for ASICamera2");
            pControlCaps->MaxValue = 1;
            pControlCaps->MinValue = 0;
            pControlCaps->DefaultValue = 0;
            pControlCaps->IsAutoSupported = ASI_FALSE;
            pControlCaps->IsWritable = ASI_TRUE;
            pControlCaps->ControlType = ASI_CONTROL_TYPE(129);
            break;
        case 2:
            strcpy(pControlCaps->Name, "MaxChunkSize");
            strcpy(pControlCaps->Description, "Size limit of a single USB transfer(MB)");
            pControlCaps->MaxValue = 256;
            pControlCaps->MinValue =   1;
            pControlCaps->DefaultValue = CameraBoost::DefaultChunkSize / 1024 / 1024;
            pControlCaps->IsAutoSupported = ASI_FALSE;
            pControlCaps->IsWritable = ASI_TRUE;
            pControlCaps->ControlType = ASI_CONTROL_TYPE(130);
            break;
        case 3:
            strcpy(pControlCaps->Name, "ChunkedTransfers");
            strcpy(pControlCaps->Description, "Number of chunked transfers");
            pControlCaps->MaxValue = 8;
            pControlCaps->MinValue =   1;
            pControlCaps->DefaultValue = CameraBoost::DefaultChunkedTransfers;
            pControlCaps->IsAutoSupported = ASI_FALSE;
            pControlCaps->IsWritable = ASI_TRUE;
            pControlCaps->ControlType = ASI_CONTROL_TYPE(131);
            break;
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
            break;
        }
        default:
            ret = __real_ASIGetControlCaps(iCameraID, iControlIndex, pControlCaps);
        }
    }
    else
    {
        ret = __real_ASIGetControlCaps(iCameraID, iControlIndex, pControlCaps);
    }

    if (ret == ASI_SUCCESS)
        dbg_printf("%s:%ld", toString(pControlCaps->ControlType), pControlCaps->DefaultValue);
    else
        dbg_printf("error:%d, index:%d", ret, iControlIndex);

    return ret;
}

ASICAMERA_API ASI_ERROR_CODE __real_ASIGetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long *plValue, ASI_BOOL *pbAuto);
ASICAMERA_API ASI_ERROR_CODE ASIGetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long *plValue, ASI_BOOL *pbAuto)
{
    const CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    ASI_ERROR_CODE ret = ASI_SUCCESS;
    if (cameraBoost)
    {
        switch (int(ControlType))
        {
        case 128:
            if (plValue) *plValue = gCameraBoostDebug;
            if (pbAuto) *pbAuto = ASI_FALSE;
            break;
        case 129:
            if (plValue) *plValue = g_bDebugPrint;
            if (pbAuto) *pbAuto = ASI_FALSE;
            break;
        case 130:
            if (plValue) *plValue = cameraBoost->getMaxChunkSize() / 1024 / 1024;
            if (pbAuto) *pbAuto = ASI_FALSE;
            break;
        case 131:
            if (plValue) *plValue = cameraBoost->getChunkedTransfers();
            if (pbAuto) *pbAuto = ASI_FALSE;
            break;

        case 132:
            if (plValue) std::ifstream("/sys/module/usbcore/parameters/usbfs_memory_mb") >> *plValue;
            if (pbAuto) *pbAuto = ASI_FALSE;
            break;
        default:
            ret = __real_ASIGetControlValue(iCameraID, ControlType, plValue, pbAuto);
        }
    }
    else
    {
        ret = __real_ASIGetControlValue(iCameraID, ControlType, plValue, pbAuto);
    }

    if (ret == ASI_SUCCESS)
        dbg_printf("%s:%ld", toString(ControlType), plValue ? *plValue : -1);
    else
        dbg_printf("%s:error:%d", toString(ControlType), ret);

    return ret;
}

ASICAMERA_API ASI_ERROR_CODE __real_ASISetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long lValue, ASI_BOOL bAuto);
ASICAMERA_API ASI_ERROR_CODE ASISetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long lValue, ASI_BOOL bAuto)
{
    CameraBoost *cameraBoost = getCameraBoost(iCameraID);
    ASI_ERROR_CODE ret = ASI_SUCCESS;

    if (cameraBoost)
    {
        switch (int(ControlType))
        {
        case 128:
            gCameraBoostDebug = lValue;
            break;

        case 129:
            g_bDebugPrint = lValue;
            break;

        case 130:
            if (!cameraBoost->setMaxChunkSize(lValue * 1024 * 1024))
                ret = ASI_ERROR_INVALID_SEQUENCE;
            break;

        case 131:
            if (!cameraBoost->setChunkedTransfers(lValue))
                ret = ASI_ERROR_INVALID_SEQUENCE;
            break;

        case 132: {
            std::ofstream file("/sys/module/usbcore/parameters/usbfs_memory_mb");
            if (!file.good())
            {
                ret = ASI_ERROR_GENERAL_ERROR;
                break;
            }

            file << lValue;

            break;
        }
        default:
            ret = __real_ASISetControlValue(iCameraID, ControlType, lValue, bAuto);
        }
    }
    else
    {
        ret = __real_ASISetControlValue(iCameraID, ControlType, lValue, bAuto);
    }

    if (ret == ASI_SUCCESS)
        dbg_printf("%s:%ld, auto:%s", toString(ControlType), lValue, bAuto ? "true" : "false");
    else
        dbg_printf("%s:error:%d", toString(ControlType), ret);

    return ret;
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
