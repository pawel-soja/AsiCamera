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
#include "asicamera.h"
#include "asicamera_p.h"

#include "asicameracontrol.h"
#include "asicameracontrol_p.h"

#include "asicamerainfo.h"

#include "ASICamera2.h"
#include "ASICamera2Boost.h"

AsiCameraPrivate::AsiCameraPrivate()
{ }

AsiCameraPrivate::~AsiCameraPrivate()
{ }

AsiCamera::AsiCamera()
    : d_ptr(new AsiCameraPrivate)
{ }

AsiCamera::AsiCamera(AsiCameraPrivate &dd)
    : d_ptr(&dd)
{ }

AsiCamera::~AsiCamera()
{
    stopVideoCapture();
    close();
}

bool AsiCamera::setCamera(int cameraId)
{
    AsiCameraPrivate *d = d_func();

    if (d->isOpen)
        return false;

    d->cameraId = cameraId;
    return true;
}

bool AsiCamera::setCamera(const AsiCameraInfo &cameraInfo)
{
    if (!cameraInfo.isValid())
        return false;

    return setCamera(cameraInfo.cameraId());
}

bool AsiCamera::open()
{
    AsiCameraPrivate *d = d_func();

    if (d->cameraId < 0)
        return false;
    
    if (d->isOpen)
        return true;
    
    d->errorCode = ASIOpenCamera(d->cameraId);
    if (d->errorCode != ASI_SUCCESS)
        return false;

    d->errorCode = ASIInitCamera(d->cameraId);
    if (d->errorCode != ASI_SUCCESS)
        return false;

    int numberOfControls = 0;
    d->errorCode = ASIGetNumOfControls(d->cameraId, &numberOfControls);
    if (d->errorCode != ASI_SUCCESS)
    {
        ASICloseCamera(d->cameraId);
        return false;
    }

    d->controls.clear();
    for(int i=0; i<numberOfControls; ++i)
    {
        AsiCameraControlPrivate *priv = new AsiCameraControlPrivate;

        d->errorCode = ASIGetControlCaps(d->cameraId, i, &priv->control);
        if (d->errorCode != ASI_SUCCESS)
        {
            delete priv;
            break;
        }

        priv->camera = std::static_pointer_cast<AsiCameraPrivate>(d_ptr);

        d->controls.push_back(*priv);
    }

    d->isOpen = true;
    return true;
}

void AsiCamera::close()
{
    AsiCameraPrivate *d = d_func();

    if (d->isOpen == false)
        return;

    d->errorCode = ASICloseCamera(d->cameraId);
    d->isOpen = false;
}

bool AsiCamera::isOpen() const
{
    const AsiCameraPrivate *d = d_func();
    return d->isOpen;
}

int AsiCamera::errorCode() const
{
    const AsiCameraPrivate *d = d_func();
    return d->errorCode;
}

bool AsiCamera::setImageFormat(ImageFormat imageFormat)
{
    AsiCameraPrivate *d = d_func();
    int w, h, bin;
    ASI_IMG_TYPE imgType;

    d->errorCode = ASIGetROIFormat(d->cameraId, &w, &h, &bin, &imgType);
    if (d->errorCode != ASI_SUCCESS)
        return false;

    d->errorCode = ASISetROIFormat(d->cameraId, w, h, bin, static_cast<ASI_IMG_TYPE>(imageFormat));
    if (d->errorCode != ASI_SUCCESS)
        return false;

    return true;
}

bool AsiCamera::setMaxChunkSize(uint maxChunkSize)
{
    AsiCameraPrivate *d = d_func();
    return ASISetMaxChunkSize(d->cameraId, maxChunkSize) == ASI_SUCCESS;
}

bool AsiCamera::setChunkedTransfers(uint chunkedTransferCount)
{
    AsiCameraPrivate *d = d_func();
    return ASISetChunkedTransfers(d->cameraId, chunkedTransferCount) == ASI_SUCCESS;
}

#if 0
AsiCameraControl AsiCamera::control(const std::string &name)
{
    AsiCameraPrivate *d = d_func();
    for(auto &control: d->controlCaps)
        if (control.Name == name)
            return AsiCameraControl(this, &control);

    return AsiCameraControl(this, nullptr);
}
#endif

AsiCameraControl AsiCamera::control(const std::string &name) const
{
    const AsiCameraPrivate *d = d_func();
    for(auto &item: d->controls)
        if (item.name() == name)
            return item;

    return AsiCameraControl();
}

std::list<AsiCameraControl> AsiCamera::controls() const
{
    const AsiCameraPrivate *d = d_func();
    return d->controls;
}

bool AsiCamera::startVideoCapture()
{
    AsiCameraPrivate *d = d_func();

    if (!isOpen())
    {
        // setStatus(CameraIsNotOpen);
        return false;
    }

    AsiCameraControl exposure = control("Exposure");


    d->exposure = exposure.get();

    if (exposure.status() != AsiCameraControl::Ok)
    {
        // setStatus(CannotReadExposure)
        return false;
    }

    d->errorCode = ASIStartVideoCapture(d->cameraId);
    if (d->errorCode == ASI_SUCCESS)
    {
        d->isVideoCapture = true;
        // setStatus(Ok);
        return true;
    }
    else
    {
        return false;
    }
}

void AsiCamera::stopVideoCapture()
{
    AsiCameraPrivate *d = d_func();

    if (!isOpen() || d->isVideoCapture == false)
    {
        // setStatus(CameraIsNotOpen);
        return;
    }

    d->errorCode = ASIStopVideoCapture(d->cameraId);
    if (d->errorCode == ASI_SUCCESS)
    {
        d->isVideoCapture = false;
    }
}

bool AsiCamera::getVideoData(void *data, size_t size)
{
    AsiCameraPrivate *d = d_func();

    if (!isOpen())
    {
        // setStatus(CameraIsNotOpen);
        return false;
    }

    if (d->isVideoCapture == false)
    {
        //setStatus(CameraIsNotVideoCapture);
        return false;
    }
    
    d->errorCode = ASIGetVideoData(
        d->cameraId,
        static_cast<unsigned char*>(data),
        size,
        d->exposure * 2 + 500
    );

    return d->errorCode == ASI_SUCCESS;
}

bool AsiCamera::getVideoDataPointer(void **data)
{
    AsiCameraPrivate *d = d_func();

    if (!isOpen())
    {
        // setStatus(CameraIsNotOpen);
        return false;
    }

    if (d->isVideoCapture == false)
    {
        //setStatus(CameraIsNotVideoCapture);
        return false;
    }
    
    d->errorCode = ASIGetVideoDataPointer(
        d->cameraId,
        reinterpret_cast<unsigned char**>(data),
        d->exposure * 2 + 500
    );

    return d->errorCode == ASI_SUCCESS;
}

AsiCameraPrivate * AsiCamera::d_func() const
{
    return static_cast<AsiCameraPrivate*>(d_ptr.get());
}
