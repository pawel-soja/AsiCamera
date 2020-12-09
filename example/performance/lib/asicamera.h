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
#include <list>
#include <memory>

//class AsiCameraControl;
#include "asicameracontrol.h" // added for easy use this header

class AsiCameraInfo;
class AsiCameraPrivate;
class AsiCamera
{
public:
    AsiCamera();
    virtual ~AsiCamera();

public:
    bool setCamera(int cameraId);
    bool setCamera(const AsiCameraInfo &cameraInfo);

    bool open();

    void close();

    bool isOpen() const;

    int errorCode() const;

public:
    bool startVideoCapture();
    void stopVideoCapture();

    bool getVideoData(void *data, size_t size);
    bool getVideoDataPointer(void **data);

public:
    AsiCameraControl control(const std::string &name) const;

    std::list<AsiCameraControl> controls() const;

public: // fast access to controls value
    AsiCameraControl operator[](const std::string &name) const
    {
        return control(name);
    }

protected:
    std::shared_ptr<AsiCameraPrivate> d_ptr;

private:
    AsiCameraPrivate * d_func() const;
    AsiCamera(AsiCameraPrivate &dd);

    AsiCamera(const AsiCamera &) = delete;
    AsiCamera &operator=(const AsiCamera &) = delete;
};
