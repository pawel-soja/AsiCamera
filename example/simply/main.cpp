#include <stdio.h>
#include <stdlib.h>

#include <limits>
#include <vector>
#include <numeric>
#include <cstring>
#include <vector>

#include <map>
#include <string>

#include <unistd.h>

#include <ASICamera2.h>
#include "deltatime.h"

#include <dlfcn.h>

const char *toString(ASI_ERROR_CODE rc)
{
    switch (rc)
    {
    case ASI_SUCCESS: return "ASI_SUCCESS";
    case ASI_ERROR_INVALID_INDEX: return "ASI_ERROR_INVALID_INDEX";
    case ASI_ERROR_INVALID_ID: return "ASI_ERROR_INVALID_ID";
    case ASI_ERROR_INVALID_CONTROL_TYPE: return "ASI_ERROR_INVALID_CONTROL_TYPE";
    case ASI_ERROR_CAMERA_CLOSED: return "ASI_ERROR_CAMERA_CLOSED";
    case ASI_ERROR_CAMERA_REMOVED: return "ASI_ERROR_CAMERA_REMOVED";
    case ASI_ERROR_INVALID_PATH: return "ASI_ERROR_INVALID_PATH";
    case ASI_ERROR_INVALID_FILEFORMAT: return "ASI_ERROR_INVALID_FILEFORMAT";
    case ASI_ERROR_INVALID_SIZE: return "ASI_ERROR_INVALID_SIZE";
    case ASI_ERROR_INVALID_IMGTYPE: return "ASI_ERROR_INVALID_IMGTYPE";
    case ASI_ERROR_OUTOF_BOUNDARY: return "ASI_ERROR_OUTOF_BOUNDARY";
    case ASI_ERROR_TIMEOUT: return "ASI_ERROR_TIMEOUT";
    case ASI_ERROR_INVALID_SEQUENCE: return "ASI_ERROR_INVALID_SEQUENCE";
    case ASI_ERROR_BUFFER_TOO_SMALL: return "ASI_ERROR_BUFFER_TOO_SMALL";
    case ASI_ERROR_VIDEO_MODE_ACTIVE: return "ASI_ERROR_VIDEO_MODE_ACTIVE";
    case ASI_ERROR_EXPOSURE_IN_PROGRESS: return "ASI_ERROR_EXPOSURE_IN_PROGRESS";
    case ASI_ERROR_GENERAL_ERROR: return "ASI_ERROR_GENERAL_ERROR";
    case ASI_ERROR_INVALID_MODE: return "ASI_ERROR_INVALID_MODE";
    case ASI_ERROR_END: return "ASI_ERROR_END";
    default: return "Unknown";
    }
}

static bool setImageFormat(int iCameraID, ASI_IMG_TYPE type)
{
    int w, h, bin, rc;
    ASI_IMG_TYPE orgType;

    rc = ASIGetROIFormat(0, &w, &h, &bin, &orgType);
    if (rc != ASI_SUCCESS)
    {
        printf("ASIGetROIFormat error: %d\n", rc);
        return false;
    }
    rc = ASISetROIFormat(0, w, h, bin, type);
    if (rc != ASI_SUCCESS)
    {
        printf("ASISetROIFormat error: %d\n", rc);
        return false;
    }
    return true;
}

static void setArguments(int iCameraID, int argc, char *argv[], int *exposure = nullptr)
{
    std::map<std::string, ASI_CONTROL_TYPE> options;
    options["Exposure"]      = ASI_EXPOSURE;
    options["HighSpeedMode"] = ASI_HIGH_SPEED_MODE;
    options["BandWidth"]     = ASI_BANDWIDTHOVERLOAD;
    options["Gain"]          = ASI_GAIN;

    for(int i=1; i<(argc-1); i += 2)
    {
        bool ok = false;
        if (!strcmp(argv[i], "Exposure") && exposure)
            *exposure = atoi(argv[i+1]);

        if (!strcmp(argv[i], "Format"))
        {
            if (!strcmp(argv[i+1], "8bit"))
                ok = setImageFormat(iCameraID, ASI_IMG_RAW8);
            else if (!strcmp(argv[i+1], "16bit"))
                ok = setImageFormat(iCameraID, ASI_IMG_RAW16);
            else
            {
                printf("Unknown format: %s\n", argv[i+1]);
                continue;
            }
        }
        else
        {
            auto option = options.find(argv[i]);
            if (option == options.end())
            {
                printf("Unknown option: %s\n", argv[i]);
                continue;
            }

            if (!strcmp(argv[i+1], "auto"))
            {
                printf("Auto option is not supported\n");
                ok = false;
            }
            else
            {
                ok = (ASISetControlValue(iCameraID, option->second, atoi(argv[i+1]), ASI_FALSE) == ASI_SUCCESS);
            }
        }

        printf("%s '%s' to %s\n", ok ? "Set" : "Cannot set", argv[i], argv[i+1]);
    }
}

extern bool g_bDebugPrint;       // libASICamera2
extern bool gCameraBoostEnable;  // libASICamera2Boost
extern bool gCameraBoostDebug;   // libASICamera2Boost debug mode
//#define CAMERABOOST_DISABLE

ASICAMERA_API ASI_ERROR_CODE (*pASIGetVideoDataPointer)(int iCameraID, unsigned char** pBuffer, int iWaitms) = nullptr;

static bool ASIGetVideoDataPointerInit()
{
    void * handle = dlopen(NULL, RTLD_LAZY);
    pASIGetVideoDataPointer = reinterpret_cast<typeof(pASIGetVideoDataPointer)>(dlsym(handle, "ASIGetVideoDataPointer"));

    fprintf(stderr, "ASIGetVideoDataPointer: %s\n", pASIGetVideoDataPointer == nullptr ? "not found" : "found");

    return pASIGetVideoDataPointer != nullptr;
}

#define ERROR_CHECK(x) do { \
    ASI_ERROR_CODE rc = x; \
    if (rc != ASI_SUCCESS) \
        fprintf(stderr, "ERROR: %s: %s\n", #x, toString(rc)); \
} while(0)

int main(int argc, char *argv[])
{
    g_bDebugPrint = true;
    gCameraBoostDebug = true;

#ifdef CAMERABOOST_DISABLE
    gCameraBoostEnable = false;
#endif

    if (ASIGetNumOfConnectedCameras() == 0)
    {
        fprintf(stderr, "No camera found\n");
        return -1;
    }

    int iCameraID = 0;
    
    ASIGetVideoDataPointerInit();
    ERROR_CHECK(ASIOpenCamera(iCameraID));
    ERROR_CHECK(ASIInitCamera(iCameraID));

    // default values
    int exposure = 100000;
    ERROR_CHECK(ASISetControlValue(iCameraID, ASI_EXPOSURE,          exposure, ASI_FALSE));
    ERROR_CHECK(ASISetControlValue(iCameraID, ASI_HIGH_SPEED_MODE,   1,   ASI_FALSE));
    ERROR_CHECK(ASISetControlValue(iCameraID, ASI_BANDWIDTHOVERLOAD, 100, ASI_FALSE));
    ERROR_CHECK(ASISetControlValue(iCameraID, ASI_GAIN,              100, ASI_FALSE));
    
    setArguments(iCameraID, argc, argv, &exposure);

    ASI_CAMERA_INFO info;
    ERROR_CHECK(ASIGetCameraProperty(&info, iCameraID));

    std::vector<unsigned char> buffer(info.MaxWidth * info.MaxWidth * 2, 0);
    unsigned char *ptr = buffer.data();
    size_t size = buffer.size();

    printf("\nVideo Capture\n");

    ERROR_CHECK(ASIStartVideoCapture(iCameraID));

    DeltaTime deltaTime;
    for(int i=0; i<5; ++i)
    {
        ASI_ERROR_CODE rc;
        if (pASIGetVideoDataPointer == nullptr)
            rc = ASIGetVideoData(
                iCameraID,
                ptr,
                size,
                exposure * 2 + 500
            );
        else
            rc = pASIGetVideoDataPointer(
                iCameraID,
                &ptr,
                exposure * 2 + 500
            );

        printf("timeframe: %5.0fms, status: %d\n", deltaTime.stop() * 1000, rc);
    }

    ERROR_CHECK(ASIStopVideoCapture(iCameraID));

    printf("\nExposure\n");

    ERROR_CHECK(ASIStartExposure(iCameraID, ASI_FALSE));
    ASI_EXPOSURE_STATUS expStatus;
    ASI_ERROR_CODE rc;
    do
    {
        rc = ASIGetExpStatus(iCameraID, &expStatus);
    }
    while (rc == ASI_SUCCESS && expStatus != ASI_EXP_SUCCESS);

    ERROR_CHECK(ASIGetDataAfterExp(iCameraID, buffer.data(), buffer.size()));

    ERROR_CHECK(ASICloseCamera(iCameraID));
    
    return 0;
}
