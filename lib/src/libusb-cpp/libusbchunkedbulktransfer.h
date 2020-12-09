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

#include <libusb-1.0/libusb.h>
#include <memory>

class LibUsbChunkedBulkTransferPrivate;
class LibUsbChunkedBulkTransfer
{
public:
    LibUsbChunkedBulkTransfer();
    LibUsbChunkedBulkTransfer(libusb_device_handle *dev, uint8_t endpoint, void *buffer, size_t length, size_t chunk, uint timeout = 100 * 1000);
    virtual ~LibUsbChunkedBulkTransfer();

    LibUsbChunkedBulkTransfer(LibUsbChunkedBulkTransfer &&other);
    LibUsbChunkedBulkTransfer &operator=(LibUsbChunkedBulkTransfer &&other);
    
public: // action
    LibUsbChunkedBulkTransfer& submit();
    LibUsbChunkedBulkTransfer& cancel();
    LibUsbChunkedBulkTransfer& wait(uint timeout = 1000);
    LibUsbChunkedBulkTransfer& transfer();

public: // getters
    uint actualLength() const;

    void *buffer();

public:
    LibUsbChunkedBulkTransfer& setBuffer(void *buffer);
    LibUsbChunkedBulkTransfer& setDevice(libusb_device_handle *dev);

protected:
    //LibUsbChunkedBulkTransferPrivate *d_ptr;
    std::unique_ptr<LibUsbChunkedBulkTransferPrivate> d_ptr;

    LibUsbChunkedBulkTransferPrivate *d_func() const;
    LibUsbChunkedBulkTransfer(LibUsbChunkedBulkTransferPrivate && dd);

private:
    LibUsbChunkedBulkTransfer(const LibUsbChunkedBulkTransfer &) = delete;
    LibUsbChunkedBulkTransfer &operator=(const LibUsbChunkedBulkTransfer &) = delete;
};
