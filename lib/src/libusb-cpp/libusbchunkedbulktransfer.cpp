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
#include "libusbchunkedbulktransfer.h"

#include <libusb-1.0/libusb.h>
#include <stdio.h>

#include <algorithm>

class LibUsbChunkedBulkTransferPrivate
{
public:
    int count;
    int done;
    int pending;
    int completed;
    int actualLength;
    libusb_transfer **transfer;
    
    LibUsbChunkedBulkTransferPrivate(libusb_device_handle *dev, uint8_t endpoint, void *buffer, size_t length, size_t chunk, uint timeout)
        : count((length + chunk - 1) / chunk)
        , done(0)
        , pending(0)
        , completed(1)
        , actualLength(0)
        , transfer(new libusb_transfer* [count])
    {
        unsigned char *pbuffer = reinterpret_cast<unsigned char *>(buffer);

        for(int i=0; i<count; ++i)
        {
            ssize_t size = std::min(chunk, length);
            transfer[i] = libusb_alloc_transfer(0);
            libusb_fill_bulk_transfer(transfer[i], dev, endpoint, pbuffer, size, &callback, this, timeout);
            transfer[i]->flags = 0;
            transfer[i]->status = LIBUSB_TRANSFER_ERROR;
            pbuffer += size;
            length -= size;
        }
    }

    virtual ~LibUsbChunkedBulkTransferPrivate()
    {
        for(int i=0; i<count; ++i)
            libusb_free_transfer(transfer[i]);

        delete [] transfer;
    }

    static void callback(libusb_transfer *transfer)
    {
        LibUsbChunkedBulkTransferPrivate *d = reinterpret_cast<LibUsbChunkedBulkTransferPrivate*>(transfer->user_data);
        
        if (transfer->status != LIBUSB_TRANSFER_COMPLETED)
        {
            //d->completed = 1;
            //return;
        }

        d->actualLength += transfer->actual_length;
        if (++d->done >= d->pending)
            d->completed = 1;
    }

    void cancel()
    {
        //fprintf(stderr, "[LibUsbChunkedBulkTransferPrivate::cancel]\n");
        for(int i=0; i<pending; ++i)
        {
            int rc = libusb_cancel_transfer(transfer[i]);
            if (rc != LIBUSB_SUCCESS)
                continue;
            //fprintf(stderr, "must wait! %d %d\n", i, transfer[i]->status);
        }
    }
};


LibUsbChunkedBulkTransfer::LibUsbChunkedBulkTransfer()
    : d_ptr(new LibUsbChunkedBulkTransferPrivate(NULL, 0, NULL, 0, 1, 0))
{

}

LibUsbChunkedBulkTransfer::LibUsbChunkedBulkTransfer(libusb_device_handle *dev, uint8_t endpoint, void *buffer, size_t length, size_t chunk, uint timeout)
    : d_ptr(new LibUsbChunkedBulkTransferPrivate(dev, endpoint, buffer, length, chunk, timeout))
{

}

LibUsbChunkedBulkTransfer::~LibUsbChunkedBulkTransfer()
{
}

LibUsbChunkedBulkTransfer::LibUsbChunkedBulkTransfer(LibUsbChunkedBulkTransfer &&other)
    : d_ptr(new LibUsbChunkedBulkTransferPrivate(NULL, 0, NULL, 0, 1, 0))
{
    std::swap(d_ptr, other.d_ptr);
}

LibUsbChunkedBulkTransfer &LibUsbChunkedBulkTransfer::operator=(LibUsbChunkedBulkTransfer &&other)
{
    std::swap(d_ptr, other.d_ptr);
    return *this;
}

#if 0
libusb_transfer *LibUsbChunkedBulkTransfer::handle()
{
    return d_func()->transfer;
}
#endif

LibUsbChunkedBulkTransfer& LibUsbChunkedBulkTransfer::submit()
{
    LibUsbChunkedBulkTransferPrivate *d = d_func();
    d->done = 0;
    d->completed = 0;
    d->actualLength = 0;
    d->pending = 0;
    for(int i=0; i<d->count; ++i)
    {
        int rc = libusb_submit_transfer(d->transfer[i]);
        if (rc != 0)
        {
            fprintf(stderr, "[LibUsbChunkedBulkTransfer::submit]: fail %s\n", libusb_error_name(rc));
            //d->completed = 1;
            cancel();
            return *this;
        } else
            ++d->pending;
    }
    return *this;
}

LibUsbChunkedBulkTransfer& LibUsbChunkedBulkTransfer::cancel()
{
    //fprintf(stderr, "[LibUsbChunkedBulkTransfer::cancel]\n");
    d_func()->cancel();
    wait();    
    // TODO event
    return *this;
}

#include <chrono>


LibUsbChunkedBulkTransfer& LibUsbChunkedBulkTransfer::wait(uint timeout)
{
    LibUsbChunkedBulkTransferPrivate *d = d_func();
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    //libusb_context *ctx = HANDLE_CTX(mTransfer->dev_handle);
    timeval tv;
    tv.tv_usec = 10 * 1000;
    tv.tv_sec = 0;
    if (d->pending == 0)
        return *this;
    while(!d->completed)
    {
        int rc = libusb_handle_events_timeout_completed(NULL, &tv, &d->completed);
        if (rc < 0) {
            if (rc == LIBUSB_ERROR_INTERRUPTED)
                continue;
            fprintf(stderr, "[LibUsbChunkedBulkTransfer::wait]: failed: %s, cancelling transfer", libusb_error_name(rc));
            d->cancel();
            continue;
        }
        std::chrono::duration<float> diff = std::chrono::high_resolution_clock::now() - start;
        if (diff.count() * 1000 > timeout)
        {
            fprintf(stderr, "[LibUsbChunkedBulkTransfer::wait]: timeout\n");
            d->cancel();
            continue;
        }
        // TODO
        #if 0
        if (d->transfer->dev_handle == NULL) {
            /* transfer completion after libusb_close() */
            d->transfer->status = LIBUSB_TRANSFER_NO_DEVICE;
            d->completed = 1;
        }
        #endif

    }
    d->pending = 0;
    return *this;
    if (d->done != d->count)
    {
        fprintf(stderr, "[LibUsbChunkedBulkTransfer::wait]: error was corrupted, %d/%d\n", d->done, d->count);
        //cancel();
    }
    for(int i=0; i<d->count; ++i)
    {
        if (d->transfer[i]->status != LIBUSB_TRANSFER_COMPLETED)
        {
            fprintf(stderr, "[LibUsbChunkedBulkTransfer::wait]: %d %d ...\n", i, d->transfer[i]->status);
            break;
        }
    }
    return *this;
}

LibUsbChunkedBulkTransfer& LibUsbChunkedBulkTransfer::transfer()
{
    submit();
    wait();
    return *this;
}

uint LibUsbChunkedBulkTransfer::actualLength() const
{
    return d_func()->actualLength;
}

void *LibUsbChunkedBulkTransfer::buffer()
{
    LibUsbChunkedBulkTransferPrivate *d = d_func();
    return d->transfer[0]->buffer;
}

LibUsbChunkedBulkTransfer& LibUsbChunkedBulkTransfer::setBuffer(void *buffer)
{
    LibUsbChunkedBulkTransferPrivate *d = d_func();
    unsigned char *pbuffer = reinterpret_cast<unsigned char*>(buffer);
    for(int i=0; i<d->count; ++i)
    {
        d->transfer[i]->buffer = pbuffer;
        pbuffer += d->transfer[i]->length;
    }

    return *this;
}

LibUsbChunkedBulkTransfer& LibUsbChunkedBulkTransfer::setDevice(libusb_device_handle *dev)
{
    LibUsbChunkedBulkTransferPrivate *d = d_func();
    for(int i=0; i<d->count; ++i)
    {
        d->transfer[i]->dev_handle = dev;
    }
    return *this;
}

LibUsbChunkedBulkTransferPrivate *LibUsbChunkedBulkTransfer::d_func() const
{
    return reinterpret_cast<LibUsbChunkedBulkTransferPrivate*>(d_ptr.get());
}

LibUsbChunkedBulkTransfer::LibUsbChunkedBulkTransfer(LibUsbChunkedBulkTransferPrivate && dd)
    : d_ptr(&dd)
{

}
