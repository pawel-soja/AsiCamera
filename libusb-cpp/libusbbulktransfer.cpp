#include "libusbbulktransfer.h"

#include <libusb-1.0/libusb.h>
#include <stdio.h>

#include <algorithm>

class LibUsbBulkTransferPrivate
{
public:
    libusb_transfer *transfer;
    int completed;

    LibUsbBulkTransferPrivate(libusb_device_handle *dev, uint8_t endpoint, void *buffer, size_t length, uint timeout)
        : transfer(libusb_alloc_transfer(0))
        , completed(0)
    {
        libusb_fill_bulk_transfer(transfer, dev, endpoint, reinterpret_cast<unsigned char*>(buffer), length, &callback, this, timeout);
    }

    virtual ~LibUsbBulkTransferPrivate()
    {
        libusb_free_transfer(transfer);
    }

    static void callback(libusb_transfer *transfer)
    {
        LibUsbBulkTransferPrivate *d = reinterpret_cast<LibUsbBulkTransferPrivate*>(transfer->user_data);
        d->completed = 1;
    }
};


LibUsbBulkTransfer::LibUsbBulkTransfer()
    : d_ptr(new LibUsbBulkTransferPrivate(NULL, 0, NULL, 0, 0))
{

}

LibUsbBulkTransfer::LibUsbBulkTransfer(libusb_device_handle *dev, uint8_t endpoint, void *buffer, size_t length, uint timeout)
    : d_ptr(new LibUsbBulkTransferPrivate(dev, endpoint, buffer, length, timeout))
{
}

LibUsbBulkTransfer::~LibUsbBulkTransfer()
{
}

LibUsbBulkTransfer::LibUsbBulkTransfer(LibUsbBulkTransfer &&other)
    : d_ptr(new LibUsbBulkTransferPrivate(NULL, 0, NULL, 0, 0))
{
    std::swap(d_ptr, other.d_ptr);
}

LibUsbBulkTransfer &LibUsbBulkTransfer::operator=(LibUsbBulkTransfer &&other)
{
    std::swap(d_ptr, other.d_ptr);
    return *this;
}

LibUsbBulkTransfer& LibUsbBulkTransfer::submit()
{
    LibUsbBulkTransferPrivate *d = d_func();
    d->completed = 0;
    libusb_submit_transfer(d->transfer);
    return *this;
}

LibUsbBulkTransfer& LibUsbBulkTransfer::cancel()
{
    LibUsbBulkTransferPrivate *d = d_func();
    libusb_cancel_transfer(d->transfer);
    return *this;
}

LibUsbBulkTransfer& LibUsbBulkTransfer::wait()
{
    LibUsbBulkTransferPrivate *d = d_func();
    //libusb_context *ctx = HANDLE_CTX(mTransfer->dev_handle);
    while(!d->completed)
    {
        int rc = libusb_handle_events_completed(NULL, &d->completed);
        if (rc < 0) {
            if (rc == LIBUSB_ERROR_INTERRUPTED)
                continue;
            fprintf(stderr, "libusb_handle_events failed: %s, cancelling transfer and retrying", libusb_error_name(rc));
            cancel();
            continue;
        }
        if (d->transfer->dev_handle == NULL) {
            /* transfer completion after libusb_close() */
            d->transfer->status = LIBUSB_TRANSFER_NO_DEVICE;
            d->completed = 1;
        }
    }
    return *this;
}

LibUsbBulkTransfer& LibUsbBulkTransfer::transfer()
{
    submit();
    wait();
    return *this;
}

uint LibUsbBulkTransfer::actualLength() const
{
    return d_func()->transfer->actual_length;
}


LibUsbBulkTransferPrivate *LibUsbBulkTransfer::d_func() const
{
    return reinterpret_cast<LibUsbBulkTransferPrivate*>(d_ptr.get());
}

LibUsbBulkTransfer::LibUsbBulkTransfer(LibUsbBulkTransferPrivate && dd)
    : d_ptr(&dd)
{

}
