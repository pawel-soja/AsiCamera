
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


extern bool g_bDebugPrint;

extern libusb_context *g_usb_context;
extern pthread_mutex_t mtx_usb;


struct override_callback_data_t
{
    uint done;
    uint size;
};

static void override_callback(libusb_transfer *transfer)
{
    struct override_callback_data_t *data = (struct override_callback_data_t*)transfer->user_data;
    if (data)
    {
        ++data->done;
        data->size += transfer->actual_length;
    }
    transfer->user_data = NULL;
}

extern "C" void __wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi(void *ctx, uint timeout1, uint timeout2, int *param_3, bool *, int size)
{
    libusb_transfer **transfer = (libusb_transfer **)(*(long *)((long)ctx + 0x58));
    uint transferCount = *(int *)((long)ctx + 0x48);

    //fprintf(stderr, "\n__wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi %d %d\n", timeout1, timeout2);
    /*

    Exposure Timeout1 Timeout2
    1000000   1000     111
    100000    1100     111
    10000     213      111
    1000      213      111
    */

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    bool ok = true;
    
    override_callback_data_t data;
    data.done = 0;
    data.size = 0;

    pthread_mutex_lock(&mtx_usb);

    // submit transfers
    for(int i=0; i<transferCount; ++i)
    {
        transfer[i]->callback  = override_callback;
        transfer[i]->user_data = &data;
        transfer[i]->timeout = timeout1;
        int rc = libusb_submit_transfer(transfer[i]);
        if (rc != LIBUSB_SUCCESS)
        {
            ok = false;
            fprintf(stderr, "libusb_submit_transfer[%d]: %s\n", i, libusb_error_name(rc));
        }
    }

    // wait for all done
    while (data.done < transferCount && ok)
    {
        int rc = libusb_handle_events_timeout(NULL, &tv);
        if (rc != LIBUSB_SUCCESS)
        {
            fprintf(stderr, "libusb_handle_events_timeout fail: %s\n", libusb_error_name(rc));
            break;
        }
    }

    // check transfers, cancel broken
    if (data.done != transferCount)
    {
        fprintf(stderr, "fail, got %d/%d transfers\n", data.done, transferCount);
        for(int i=0; i<transferCount; ++i)
        {
            if (transfer[i]->user_data == NULL)
                continue;

            int rc = libusb_cancel_transfer(transfer[i]);
            if (rc != LIBUSB_SUCCESS)
            {
                fprintf(stderr, "cannot cancel transfer %d: %s\n", i, libusb_error_name(rc));
                continue;
            }

            while(transfer[i]->user_data != NULL)
                libusb_handle_events_timeout(NULL, &tv);
        }
    }

    pthread_mutex_unlock(&mtx_usb);
    *param_3 = size;
}
