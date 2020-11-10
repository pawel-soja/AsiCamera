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
#include "asicameracontrol.h"
#include "asicameracontrol_p.h"

#include "asicamera_p.h"

#include "asicamera.h"
#include "ASICamera2.h"

AsiCameraControlPrivate::AsiCameraControlPrivate()
    : status(AsiCameraControl::Ok)
{ }

AsiCameraControlPrivate::~AsiCameraControlPrivate()
{ }

AsiCameraControl::AsiCameraControl()
    : d_ptr(nullptr)
{ }

AsiCameraControl::~AsiCameraControl()
{ }
    
AsiCameraControl::AsiCameraControl(AsiCameraControlPrivate &dd)
    : d_ptr(&dd)
{ }

enum Status
    {
        Ok,
        Fail,
        OverRange,
        CameraNotExists,
        NotWriteable
    };

std::string AsiCameraControl::statusString() const
{
    const AsiCameraControlPrivate *d = d_func();

    if (d == nullptr)
        return "Camera Not Exists";

    switch(d->status)
    {
    case Ok: return "Ok";
    case Fail: return "Fail";
    case OverRange: return "Over Range";
    case CameraNotExists: return "Camera Not Exists";
    case NotWriteable: return "Not Writeable";
    default: return "Unknown";
    }
}

AsiCameraControl::Status AsiCameraControl::status() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->status : CameraNotExists;
}

void AsiCameraControl::setStatus(Status newStatus)
{
    AsiCameraControlPrivate *d = d_func();
    if (d != nullptr)
        d->status = newStatus;
}

bool AsiCameraControl::isValid() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr && d->camera->isOpen;
}

std::string AsiCameraControl::name() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->control.Name : "";
}

std::string AsiCameraControl::description() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->control.Description : "";
}

long AsiCameraControl::max() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->control.MaxValue : 0;
}

long AsiCameraControl::min() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->control.MinValue : 0;
}

long AsiCameraControl::defaultValue() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->control.DefaultValue : 0;
}

bool AsiCameraControl::isAutoSupported() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->control.IsAutoSupported : false;
}

bool AsiCameraControl::isWriteable() const
{
    const AsiCameraControlPrivate *d = d_func();
    return d != nullptr ? d->control.IsWritable : false;
}

long AsiCameraControl::get(bool *isAutoControl)
{
    const AsiCameraControlPrivate *d = d_func();
    long value = 0;
    ASI_BOOL autoControl;

    if (isValid() == false)
    {
        setStatus(CameraNotExists);
        return 0;
    }


    d->camera->errorCode = ASIGetControlValue(d->camera->cameraId, d->control.ControlType, &value, &autoControl);

    if (d->camera->errorCode == ASI_SUCCESS)
    {
        if (isAutoControl != nullptr)
            *isAutoControl = autoControl;
        setStatus(Ok);
    }
    else
    {
        setStatus(Fail);
    }

    return value;
}

bool AsiCameraControl::set(long value, bool autoControl)
{
    AsiCameraControlPrivate *d = d_func();

    if (isValid() == false)
    {
        setStatus(CameraNotExists);
        return false;
    }

    if (isWriteable() == false)
    {
        setStatus(NotWriteable);
        return false;
    }

    if (value < d->control.MinValue || value > d->control.MaxValue)
    {
        setStatus(OverRange);
        return false;
    }

    if (ASISetControlValue(
            d->camera->cameraId,
            d->control.ControlType,
            value,
            autoControl ? ASI_TRUE : ASI_FALSE
        ) == ASI_SUCCESS
    )
    {
        setStatus(Ok);
        return true;
    }
    else
    {
        setStatus(Fail);
        return false;
    }
}

AsiCameraControlPrivate * AsiCameraControl::d_func() const
{
    return static_cast<AsiCameraControlPrivate*>(d_ptr.get());
}
