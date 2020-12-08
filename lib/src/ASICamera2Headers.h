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
#pragma once

#include <libusb-1.0/libusb.h>
#include <pthread.h>
#include "ASICamera2.h"

extern bool g_bDebugPrint;

extern libusb_context *g_usb_context;
extern pthread_mutex_t mtx_usb;

class CCameraFX3
{
    char dummy[1];
public:
    CCameraFX3();
    ~CCameraFX3();

public:
    /* ... */
};

class CCameraBase: public CCameraFX3
{
    char dummy[1];
public:
    CCameraBase();
    virtual ~CCameraBase(); // important virtual

public:
    void InitVariable();
    /* ... */
};

class CirBuf
{
    char dummy[1];
public:
    CirBuf(long size);
    ~CirBuf();

public:
    void StartInstBufThr();
    int InsertBuff(
        unsigned char *buffer,   int bufferSize,
        unsigned short tagBegin, int tagBeginIndex,
        unsigned short tagEnd,   int tagEndIndex,
        int tagCmpIndex1,        int tagCmpIndex2
    );
    int ReadBuff(unsigned char *buffer, int bufferSize, int timeout);
    /* ... */
};
