#include <stdio.h>
#include <stdlib.h>

#include "asicamera.h"
#include "asicamerainfo.h"

#include <limits>
#include <vector>
#include <chrono>
#include <numeric>
#include <cstring>

#include <unistd.h>


#include <stdio.h>
#include <stdlib.h>

#include "asicamera.h"
#include "asicamerainfo.h"

#include <limits>
#include <vector>
#include <chrono>
#include <numeric>
#include <cstring>

#include <unistd.h>

using std::to_string;
std::string to_string(const std::string &str)
{
    return str;
}
template <typename T>
std::string joinAsString(const T &arg, const std::string &separator = ", ")
{
    using element_type_t = std::remove_reference_t<decltype(*std::begin(std::declval<T&>()))>;

    return std::begin(arg) == std::end(arg) ? "" : std::accumulate(
        std::next(std::begin(arg)),
        std::end(arg),
        to_string(*std::begin(arg)),
        [&](std::string a, element_type_t b) { return std::move(a) + separator + to_string(b); }
    );
}

#include <libusb-1.0/libusb.h>
#include <stdio.h>

#include "symtab_memory.h"

extern "C"
{

extern libusb_context *g_usb_context;

static int  * p_lin_XferLen = NULL;
static bool * p_lin_XferCallbacked = NULL;

static int * p_len_get = NULL;

#if 0
int __real_libusb_cancel_transfer(struct libusb_transfer *);
//int __attribute__((weak)) __real_libusb_cancel_transfer(struct libusb_transfer *) { return 0; }

int __wrap_libusb_cancel_transfer(struct libusb_transfer *transfer)
{
    int retval = __real_libusb_cancel_transfer(transfer);
    if (retval == LIBUSB_ERROR_NOT_FOUND)
    {
        fprintf(stderr, "__wrap_libusb_cancel_transfer: got LIBUSB_ERROR_NOT_FOUND, fixing...\n");
        #if 0
        if (p_lin_XferLen != nullptr)
            *p_lin_XferLen = -1;

        if (p_lin_XferCallbacked != nullptr)
            *p_lin_XferCallbacked = true;
        #endif
    }
    else
    {
        //fprintf(stderr, "__wrap_libusb_cancel_transfer: OK\n");
    }

    return retval;
}

int __real_libusb_submit_transfer(struct libusb_transfer *tr);
int __wrap_libusb_submit_transfer(struct libusb_transfer *tr)
{
    fprintf(stderr, "libusb_submit_transfer: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_submit_transfer(tr);
}

int __real_libusb_wait_for_event(libusb_context *ctx, struct timeval *tv);
int __wrap_libusb_wait_for_event(libusb_context *ctx, struct timeval *tv)
{
    fprintf(stderr, "libusb_wait_for_event: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_wait_for_event(ctx, tv);
}


int __real_libusb_handle_events_completed(libusb_context *ctx, int *completed);
int __wrap_libusb_handle_events_completed(libusb_context *ctx, int *completed)
{
    fprintf(stderr, "libusb_handle_events_completed: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_handle_events_completed(ctx, completed);
}

//libusb_handle_events_timeout

int __real_libusb_handle_events_timeout_completed(libusb_context *ctx, struct timeval *tv, int *completed);
int __wrap_libusb_handle_events_timeout_completed(libusb_context *ctx, struct timeval *tv, int *completed)
{
    fprintf(stderr, "libusb_handle_events_timeout_completed: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_handle_events_timeout_completed(ctx, tv, completed);
}

int __real_libusb_handle_events_timeout(libusb_context *ctx, struct timeval *tv);
int __wrap_libusb_handle_events_timeout(libusb_context *ctx, struct timeval *tv)
{
    fprintf(stderr, "libusb_handle_events_timeout: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_handle_events_timeout(ctx, tv);
}
#endif
static void fix_asicam()
{
    void *result[3];
    const char *names[] = {"_ZL11lin_XferLen", "_ZL18lin_XferCallbacked", "_ZL7len_get", NULL};
    symtab_memory(names, result);
    p_lin_XferLen = reinterpret_cast<int *>(result[0]);
    p_lin_XferCallbacked = reinterpret_cast<bool *>(result[1]);
    p_len_get = reinterpret_cast<int *>(result[2]);

}

struct callback_data_t
{
    int pending;
    int bytesRead;
};

static void override_callback(libusb_transfer *transfer)
{
    //callback_data_t *callback_data = (callback_data_t *)transfer->user_data;

    if (callback_data == NULL)
        return;

    *(libusb_transfer **)transfer->user_data = transfer;

    //callback_data->bytesRead += transfer->actual_length;
}


extern pthread_mutex_t mtx_usb;
// poc
void __wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi(void *ctx, uint timeout1, uint timeout2, int *param_3, bool *, int size)
{
    libusb_transfer **transfer = (libusb_transfer **)(*(long *)((long)ctx + 0x58));
    uint transferCount = *(int *)((long)ctx + 0x48);

    fprintf(stderr, "\n__wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi %d %d\n", timeout1, timeout2);
    /*

    Exposure Timeout1 Timeout2
    1000000   1000     111
    100000    1100     111
    10000     213      111
    1000      213      111
    */

    struct callback_data_t callback_data;

    callback_data.bytesRead = 0;

    uint32_t pending = 0;

    libusb_transfer *current = NULL;

    pthread_mutex_lock(&mtx_usb);
    for(int i=0; i<transferCount; ++i)
    {
        transfer[i]->callback = override_callback;
        transfer[i]->user_data = &current;
        //transfer[i]->user_data = &callback_data;
        //transfer[i]->timeout = 20*1000;
        int rc = libusb_submit_transfer(transfer[i]);
        if (rc == LIBUSB_SUCCESS)
        {
            pending |= 1<<i;
        }
        else
        {
            fprintf(stderr, "libusb_submit_transfer[%d]: %s\n", i, libusb_error_name(rc));
            usleep(10*1000);
        }
    }

    timeval tv;
    timeout1 /= transferCount;
    tv.tv_sec = timeout1 / 1000;
    tv.tv_usec = (timeout1 % 1000) * 1000;

    for(int i=0; i<transferCount; ++i)
    {
        //if (pending & (1<<i))
        {
            current = NULL;
            int rc = libusb_handle_events_timeout(NULL, &tv);
            if (rc != LIBUSB_SUCCESS)
            {
                fprintf(stderr, "libusb_handle_events_timeout[%d]: %s\n", i, libusb_error_name(rc));                
            }
            else
            {
                pending &= ~(1<<i);
            }
            
        }
    }
#if 0

    int rc = libusb_cancel_transfer(transfer[i]);
    if (rc == LIBUSB_SUCCESS)
    {
        libusb_handle_events(NULL);
    }
#endif

    for(int i=0; i<transferCount; ++i)
    {
        //if (pending & (1<<i))
        {
            int rc = libusb_cancel_transfer(transfer[i]);
            fprintf(stderr, "libusb_cancel_transfer[%d]: %s\n", i, libusb_error_name(rc));
            if (rc == LIBUSB_SUCCESS)
            {
                rc = libusb_handle_events(NULL);
                fprintf(stderr, "1 libusb_handle_events[%d]: %s\n", i, libusb_error_name(rc));
                rc = libusb_handle_events(NULL);
                fprintf(stderr, "2 libusb_handle_events[%d]: %s\n", i, libusb_error_name(rc));
            }
        }
    }

    pthread_mutex_unlock(&mtx_usb);
    *param_3 = size;
}

}

extern bool g_bDebugPrint;


int main(int argc, char *argv[])
{
    fix_asicam();
    g_bDebugPrint = true;

    auto cameraInfoList = AsiCameraInfo::availableCameras();
    for(auto &cameraInfo: cameraInfoList)
    {
        printf("Camera Found!\n\n");
        printf("|-----------------------------------------------------------|\n");
        printf("|        Parameter       |     Value                        |\n");
        printf("|-----------------------------------------------------------|\n");
        printf("| Name                   | %-32s |\n", cameraInfo.name().data());
        printf("| Camera Id              | %-32d |\n", cameraInfo.cameraId());
        printf("| Max Height             | %-32d |\n", cameraInfo.maxHeight());
        printf("| Max Width              | %-32d |\n", cameraInfo.maxWidth());
        printf("| Is Color               | %-32s |\n", cameraInfo.isColor() ? "yes": "no");
        printf("| Bayer Pattern          | %-32s |\n", cameraInfo.bayerPatternAsString().data());
        printf("| Supported Bins         | %-32s |\n", joinAsString(cameraInfo.supportedBins()).data());
        printf("| Supported Video Format | %-32s |\n", joinAsString(cameraInfo.supportedVideoFormatAsStringList()).data());
        printf("| Pixel Size             | %-32g |\n", cameraInfo.pixelSize());
        printf("| Mechanical Shutter     | %-32s |\n", cameraInfo.mechanicalShutter() ? "yes" : "no");
        printf("| ST4 Port               | %-32s |\n", cameraInfo.st4Port() ? "yes" : "no");
        printf("| Is Cooled Camera       | %-32s |\n", cameraInfo.isCooled() ? "yes" : "no");
        printf("| Is USB3 Host           | %-32s |\n", cameraInfo.isUsb3Host() ? "yes" : "no");
        printf("| Is USB3 Camera         | %-32s |\n", cameraInfo.isUsb3Camera() ? "yes" : "no");
        printf("| Elec Per Adu           | %-32g |\n", cameraInfo.elecPerADU());
        printf("| Bit Depth              | %-32d |\n", cameraInfo.bitDepth());
        printf("| Is Trigger Camera      | %-32s |\n", cameraInfo.isTriggerCamera() ? "yes" : "no");
        printf("|-----------------------------------------------------------|\n\n");
        
        AsiCamera camera;
        camera.setCamera(cameraInfo);

        if (!camera.open())
        {
            printf("Cannot open camera: %d\n", camera.errorCode());
        }

        for(int i=1; i<(argc-1); i += 2)
        {
            auto control = camera[std::string(argv[i])];
            if (!control.isValid())
                printf("Control not found: %s\n", argv[i]);

            if (!strcmp(argv[i+1], "auto"))
            {
                if (control.setAutoControl() == false)
                    printf("Cannot set '%s' to auto mode\n", argv[i]);
            }
            else
            {
                if (control.set(atoi(argv[i+1])) == false)
                    printf("Cannot set '%s' to '%s'\n", argv[i], argv[i+1]);
            }

            printf("Set '%s' to %s\n", argv[i], argv[i+1]);
        }   

        printf("\n");
        printf("Camera Options\n");
        printf("|------------------------------------------------------------------------------------------------------------------------------|\n");
        printf("|        Option  Name      |  Value |   Min  |     Max    | Default  | Auto |                Description                       |\n");
        printf("|------------------------------------------------------------------------------------------------------------------------------|\n");
        for(auto &option: camera.controls())
        {
            bool isAutoControl;
            long value = option.get(&isAutoControl);
            printf(
                "| %-24s | %-6d | %-6d | %-10d | %-8d | %-4s | %-48s |\n",
                option.name().data(),
                value,
                option.min(),
                option.max(),
                option.defaultValue(),
                isAutoControl ? "yes": "no",
                option.description().data()
            );
        }
        printf("|------------------------------------------------------------------------------------------------------------------------------|\n");

        //libusb_set_debug(g_usb_context, 21)
        //libusb_set_option(g_usb_context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
        //libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

        printf("\nFind best bandwidth\n");
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");
        printf("| BW [%] |                                            Frame duration [ms]                                                         |\n");
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");

        std::vector<uint8_t> frame(cameraInfo.maxWidth() * cameraInfo.maxHeight() * 2 * 2);

        void *buffer = frame.data();
        size_t bufferSize = frame.size();
        
        camera.startVideoCapture();

        long frames = 0;
        long failFrames = 0;
        
        std::chrono::duration<double> diff;
        std::chrono::high_resolution_clock::time_point start, end;

        long bestBandWidth = 40;
        double bestTime = std::numeric_limits<double>::max();

        for(int bandWidth = 40; bandWidth <= 100; bandWidth += 5)
        {
            printf("|  %3d%%  |", bandWidth);
            camera["BandWidth"] = bandWidth;
            double totalTime = 0;
            for(int i=0; i<20; ++i)
            {
                fprintf(stderr, "\n");
                start = std::chrono::high_resolution_clock::now();
                bool ok = camera.getVideoData(buffer, bufferSize);
                end = std::chrono::high_resolution_clock::now();
                diff = end - start;
                totalTime += diff.count();
                printf("%5.0f ", diff.count() * 1000);
                fflush(stdout);
            }
            if (bestTime > totalTime)
            {
                bestTime = totalTime;
                bestBandWidth = bandWidth;
            }
            printf("|\n");
        }
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");

        printf("\nBest bandwidth is %d%%, it has %.1f FPS\n", bestBandWidth, 20 / bestTime);
        camera["BandWidth"] = bestBandWidth;
#if 0
        for(int i=0; i<5; ++i) camera.getVideoData(buffer, bufferSize); // flush

        printf("\nFrames Per Second Test\n");
        start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; ++i)
        {
            ++frames;
            if (camera.getVideoData(buffer, bufferSize) == false)
                ++failFrames;
            end = std::chrono::high_resolution_clock::now();
            diff = end - start;
            if (diff.count() > 1)
                break;
        }
        printf("Fail %d/%d FPS %.1f\n", failFrames, frames, double(frames) / diff.count());
#endif
        camera.stopVideoCapture();
        camera.close();

    }

    return 0;
}


using std::to_string;
std::string to_string(const std::string &str)
{
    return str;
}
template <typename T>
std::string joinAsString(const T &arg, const std::string &separator = ", ")
{
    using element_type_t = std::remove_reference_t<decltype(*std::begin(std::declval<T&>()))>;

    return std::begin(arg) == std::end(arg) ? "" : std::accumulate(
        std::next(std::begin(arg)),
        std::end(arg),
        to_string(*std::begin(arg)),
        [&](std::string a, element_type_t b) { return std::move(a) + separator + to_string(b); }
    );
}

#include <libusb-1.0/libusb.h>
#include <stdio.h>

#include "symtab_memory.h"

extern "C"
{

extern libusb_context *g_usb_context;

static int  * p_lin_XferLen = NULL;
static bool * p_lin_XferCallbacked = NULL;

static int * p_len_get = NULL;

#if 0
int __real_libusb_cancel_transfer(struct libusb_transfer *);
//int __attribute__((weak)) __real_libusb_cancel_transfer(struct libusb_transfer *) { return 0; }

int __wrap_libusb_cancel_transfer(struct libusb_transfer *transfer)
{
    int retval = __real_libusb_cancel_transfer(transfer);
    if (retval == LIBUSB_ERROR_NOT_FOUND)
    {
        fprintf(stderr, "__wrap_libusb_cancel_transfer: got LIBUSB_ERROR_NOT_FOUND, fixing...\n");
        #if 0
        if (p_lin_XferLen != nullptr)
            *p_lin_XferLen = -1;

        if (p_lin_XferCallbacked != nullptr)
            *p_lin_XferCallbacked = true;
        #endif
    }
    else
    {
        //fprintf(stderr, "__wrap_libusb_cancel_transfer: OK\n");
    }

    return retval;
}

int __real_libusb_submit_transfer(struct libusb_transfer *tr);
int __wrap_libusb_submit_transfer(struct libusb_transfer *tr)
{
    fprintf(stderr, "libusb_submit_transfer: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_submit_transfer(tr);
}

int __real_libusb_wait_for_event(libusb_context *ctx, struct timeval *tv);
int __wrap_libusb_wait_for_event(libusb_context *ctx, struct timeval *tv)
{
    fprintf(stderr, "libusb_wait_for_event: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_wait_for_event(ctx, tv);
}


int __real_libusb_handle_events_completed(libusb_context *ctx, int *completed);
int __wrap_libusb_handle_events_completed(libusb_context *ctx, int *completed)
{
    fprintf(stderr, "libusb_handle_events_completed: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_handle_events_completed(ctx, completed);
}

//libusb_handle_events_timeout

int __real_libusb_handle_events_timeout_completed(libusb_context *ctx, struct timeval *tv, int *completed);
int __wrap_libusb_handle_events_timeout_completed(libusb_context *ctx, struct timeval *tv, int *completed)
{
    fprintf(stderr, "libusb_handle_events_timeout_completed: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_handle_events_timeout_completed(ctx, tv, completed);
}

int __real_libusb_handle_events_timeout(libusb_context *ctx, struct timeval *tv);
int __wrap_libusb_handle_events_timeout(libusb_context *ctx, struct timeval *tv)
{
    fprintf(stderr, "libusb_handle_events_timeout: %d %d %d\n", *p_lin_XferLen, *p_len_get, *p_lin_XferCallbacked);
    return __real_libusb_handle_events_timeout(ctx, tv);
}
#endif
static void fix_asicam()
{
    void *result[3];
    const char *names[] = {"_ZL11lin_XferLen", "_ZL18lin_XferCallbacked", "_ZL7len_get", NULL};
    symtab_memory(names, result);
    p_lin_XferLen = reinterpret_cast<int *>(result[0]);
    p_lin_XferCallbacked = reinterpret_cast<bool *>(result[1]);
    p_len_get = reinterpret_cast<int *>(result[2]);

}

struct callback_data_t
{
    int pending;
    int bytesRead;
};

static void override_callback(libusb_transfer *transfer)
{
    callback_data_t *callback_data = (callback_data_t *)transfer->user_data;

    if (callback_data == NULL)
        return;

    callback_data->bytesRead += transfer->actual_length;
}


extern pthread_mutex_t mtx_usb;
// poc
void __wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi(void *ctx, uint timeout1, uint timeout2, int *param_3, bool *, int size)
{
    libusb_transfer **transfer = (libusb_transfer **)(*(long *)((long)ctx + 0x58));
    uint transferCount = *(int *)((long)ctx + 0x48);

    fprintf(stderr, "\n__wrap__ZN10CCameraFX314startAsyncXferEjjPiPbi %d %d\n", timeout1, timeout2);
    /*

    Exposure Timeout1 Timeout2
    1000000   1000     111
    100000    1100     111
    10000     213      111
    1000      213      111
    */

    struct callback_data_t callback_data;

    callback_data.bytesRead = 0;

    uint32_t pending = 0;

    pthread_mutex_lock(&mtx_usb);
    for(int i=0; i<transferCount; ++i)
    {
        transfer[i]->callback = override_callback;
        //transfer[i]->user_data = &callback_data;
        //transfer[i]->timeout = 20*1000;
        int rc = libusb_submit_transfer(transfer[i]);
        if (rc == LIBUSB_SUCCESS)
            pending |= 1<<i;
        else
            fprintf(stderr, "libusb_submit_transfer[%d]: %s\n", i, libusb_error_name(rc));
    }

    timeval tv;
    timeout1 /= transferCount;
    tv.tv_sec = timeout1 / 1000;
    tv.tv_usec = (timeout1 % 1000) * 1000;

    for(int i=0; i<transferCount; ++i)
    {
        if (pending & (1<<i))
        {
            int rc = libusb_handle_events_timeout(NULL, &tv);
            if (rc != LIBUSB_SUCCESS)
            {
                fprintf(stderr, "libusb_handle_events_timeout[%d]: %s\n", i, libusb_error_name(rc));
                int rc = libusb_cancel_transfer(transfer[i]);
                if (rc == LIBUSB_SUCCESS)
                {
                    libusb_handle_events(NULL);
                }
                
            }
        }
        else
        {
            fprintf(stderr, "libusb_handle_events_timeout[%d]: invalid\n", i);
            int rc = libusb_cancel_transfer(transfer[i]);
            if (rc == LIBUSB_SUCCESS)
            {
                libusb_handle_events(NULL);
            }
            
        }
    }
    pthread_mutex_unlock(&mtx_usb);
    *param_3 = size;
}

}

extern bool g_bDebugPrint;


int main(int argc, char *argv[])
{
    fix_asicam();
    g_bDebugPrint = true;

    auto cameraInfoList = AsiCameraInfo::availableCameras();
    for(auto &cameraInfo: cameraInfoList)
    {
        printf("Camera Found!\n\n");
        printf("|-----------------------------------------------------------|\n");
        printf("|        Parameter       |     Value                        |\n");
        printf("|-----------------------------------------------------------|\n");
        printf("| Name                   | %-32s |\n", cameraInfo.name().data());
        printf("| Camera Id              | %-32d |\n", cameraInfo.cameraId());
        printf("| Max Height             | %-32d |\n", cameraInfo.maxHeight());
        printf("| Max Width              | %-32d |\n", cameraInfo.maxWidth());
        printf("| Is Color               | %-32s |\n", cameraInfo.isColor() ? "yes": "no");
        printf("| Bayer Pattern          | %-32s |\n", cameraInfo.bayerPatternAsString().data());
        printf("| Supported Bins         | %-32s |\n", joinAsString(cameraInfo.supportedBins()).data());
        printf("| Supported Video Format | %-32s |\n", joinAsString(cameraInfo.supportedVideoFormatAsStringList()).data());
        printf("| Pixel Size             | %-32g |\n", cameraInfo.pixelSize());
        printf("| Mechanical Shutter     | %-32s |\n", cameraInfo.mechanicalShutter() ? "yes" : "no");
        printf("| ST4 Port               | %-32s |\n", cameraInfo.st4Port() ? "yes" : "no");
        printf("| Is Cooled Camera       | %-32s |\n", cameraInfo.isCooled() ? "yes" : "no");
        printf("| Is USB3 Host           | %-32s |\n", cameraInfo.isUsb3Host() ? "yes" : "no");
        printf("| Is USB3 Camera         | %-32s |\n", cameraInfo.isUsb3Camera() ? "yes" : "no");
        printf("| Elec Per Adu           | %-32g |\n", cameraInfo.elecPerADU());
        printf("| Bit Depth              | %-32d |\n", cameraInfo.bitDepth());
        printf("| Is Trigger Camera      | %-32s |\n", cameraInfo.isTriggerCamera() ? "yes" : "no");
        printf("|-----------------------------------------------------------|\n\n");
        
        AsiCamera camera;
        camera.setCamera(cameraInfo);

        if (!camera.open())
        {
            printf("Cannot open camera: %d\n", camera.errorCode());
        }

        for(int i=1; i<(argc-1); i += 2)
        {
            auto control = camera[std::string(argv[i])];
            if (!control.isValid())
                printf("Control not found: %s\n", argv[i]);

            if (!strcmp(argv[i+1], "auto"))
            {
                if (control.setAutoControl() == false)
                    printf("Cannot set '%s' to auto mode\n", argv[i]);
            }
            else
            {
                if (control.set(atoi(argv[i+1])) == false)
                    printf("Cannot set '%s' to '%s'\n", argv[i], argv[i+1]);
            }

            printf("Set '%s' to %s\n", argv[i], argv[i+1]);
        }   

        printf("\n");
        printf("Camera Options\n");
        printf("|------------------------------------------------------------------------------------------------------------------------------|\n");
        printf("|        Option  Name      |  Value |   Min  |     Max    | Default  | Auto |                Description                       |\n");
        printf("|------------------------------------------------------------------------------------------------------------------------------|\n");
        for(auto &option: camera.controls())
        {
            bool isAutoControl;
            long value = option.get(&isAutoControl);
            printf(
                "| %-24s | %-6d | %-6d | %-10d | %-8d | %-4s | %-48s |\n",
                option.name().data(),
                value,
                option.min(),
                option.max(),
                option.defaultValue(),
                isAutoControl ? "yes": "no",
                option.description().data()
            );
        }
        printf("|------------------------------------------------------------------------------------------------------------------------------|\n");

        //libusb_set_debug(g_usb_context, 21)
        //libusb_set_option(g_usb_context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
        //libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

        printf("\nFind best bandwidth\n");
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");
        printf("| BW [%] |                                            Frame duration [ms]                                                         |\n");
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");

        std::vector<uint8_t> frame(cameraInfo.maxWidth() * cameraInfo.maxHeight() * 2 * 2);

        void *buffer = frame.data();
        size_t bufferSize = frame.size();
        
        camera.startVideoCapture();

        long frames = 0;
        long failFrames = 0;
        
        std::chrono::duration<double> diff;
        std::chrono::high_resolution_clock::time_point start, end;

        long bestBandWidth = 40;
        double bestTime = std::numeric_limits<double>::max();

        for(int bandWidth = 40; bandWidth <= 100; bandWidth += 5)
        {
            printf("|  %3d%%  |", bandWidth);
            camera["BandWidth"] = bandWidth;
            double totalTime = 0;
            for(int i=0; i<20; ++i)
            {
                fprintf(stderr, "\n");
                start = std::chrono::high_resolution_clock::now();
                bool ok = camera.getVideoData(buffer, bufferSize);
                end = std::chrono::high_resolution_clock::now();
                diff = end - start;
                totalTime += diff.count();
                printf("%5.0f ", diff.count() * 1000);
                fflush(stdout);
            }
            if (bestTime > totalTime)
            {
                bestTime = totalTime;
                bestBandWidth = bandWidth;
            }
            printf("|\n");
        }
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");

        printf("\nBest bandwidth is %d%%, it has %.1f FPS\n", bestBandWidth, 20 / bestTime);
        camera["BandWidth"] = bestBandWidth;
#if 0
        for(int i=0; i<5; ++i) camera.getVideoData(buffer, bufferSize); // flush

        printf("\nFrames Per Second Test\n");
        start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<10000; ++i)
        {
            ++frames;
            if (camera.getVideoData(buffer, bufferSize) == false)
                ++failFrames;
            end = std::chrono::high_resolution_clock::now();
            diff = end - start;
            if (diff.count() > 1)
                break;
        }
        printf("Fail %d/%d FPS %.1f\n", failFrames, frames, double(frames) / diff.count());
#endif
        camera.stopVideoCapture();
        camera.close();

    }

    return 0;
}
