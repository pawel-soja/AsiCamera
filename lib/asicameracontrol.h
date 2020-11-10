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

#include <string>
#include <memory>

class AsiCameraControlPrivate;
class AsiCameraControl
{
public:
    enum Status
    {
        Ok,
        Fail,
        OverRange,
        CameraNotExists,
        NotWriteable
    };

public:
    AsiCameraControl();
    virtual ~AsiCameraControl();

public:
    Status status() const;
    std::string statusString() const;
    void setStatus(Status newStatus);

    bool isValid() const;

    std::string name() const;

    std::string description() const;

    long max() const;

    long min() const;

    long defaultValue() const;

    bool isAutoSupported() const;

    bool isWriteable() const;
    
public:
    long get(bool *isAutoControl = nullptr);
    bool set(long value, bool autoControl = false);

public:
    AsiCameraControl &operator=(long value)
    {
        set(value);
        return *this;
    }

    operator long()
    {
        return get();
    }

protected:
    std::shared_ptr<AsiCameraControlPrivate> d_ptr;

private:
    AsiCameraControl(AsiCameraControlPrivate &dd);
    AsiCameraControlPrivate * d_func() const;
    
    friend class AsiCamera;
};
