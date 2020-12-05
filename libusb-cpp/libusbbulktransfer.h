#pragma once

#include <libusb-1.0/libusb.h>
#include <memory>

class LibUsbBulkTransferPrivate;
class LibUsbBulkTransfer
{
public:
    LibUsbBulkTransfer();
    LibUsbBulkTransfer(libusb_device_handle *dev, uint8_t endpoint, void *buffer, size_t length, uint timeout = 100 * 1000);
    virtual ~LibUsbBulkTransfer();

    LibUsbBulkTransfer(LibUsbBulkTransfer &&other);
    LibUsbBulkTransfer &operator=(LibUsbBulkTransfer &&other);

public: // action
    LibUsbBulkTransfer& submit();
    LibUsbBulkTransfer& cancel();
    LibUsbBulkTransfer& wait();
    LibUsbBulkTransfer& transfer();

public: // getters
    uint actualLength() const;

protected:
    std::unique_ptr<LibUsbBulkTransferPrivate> d_ptr;

    LibUsbBulkTransferPrivate *d_func() const;
    LibUsbBulkTransfer(LibUsbBulkTransferPrivate && dd);

private:
    LibUsbBulkTransfer(const LibUsbBulkTransfer &) = delete;
    LibUsbBulkTransfer &operator=(const LibUsbBulkTransfer &) = delete;
};
