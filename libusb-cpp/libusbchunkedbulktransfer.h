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
