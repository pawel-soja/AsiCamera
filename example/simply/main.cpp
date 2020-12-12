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

#include <ASICamera2Boost.h>
#include "deltatime.h"

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
        else if (!strcmp(argv[i], "MaxChunkSize"))
        {
            ok = (ASISetMaxChunkSize(iCameraID, atoi(argv[i+1])) == ASI_SUCCESS);
        }
        else if (!strcmp(argv[i], "ChunkedTransfers"))
        {
            ok = (ASISetChunkedTransfers(iCameraID, atoi(argv[i+1])) == ASI_SUCCESS);
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

int main(int argc, char *argv[])
{
    //g_bDebugPrint = true;
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
    
    ASIOpenCamera(iCameraID);
    ASIInitCamera(iCameraID);

    // default values
    int exposure = 100000;
    ASISetControlValue(iCameraID, ASI_EXPOSURE,          exposure, ASI_FALSE);
    ASISetControlValue(iCameraID, ASI_HIGH_SPEED_MODE,   1,   ASI_FALSE);
    ASISetControlValue(iCameraID, ASI_BANDWIDTHOVERLOAD, 100, ASI_FALSE);
    ASISetControlValue(iCameraID, ASI_GAIN,              100, ASI_FALSE);
    
    setArguments(iCameraID, argc, argv, &exposure);

    ASI_CAMERA_INFO info;
    ASIGetCameraProperty(&info, iCameraID);

    std::vector<unsigned char> buffer(info.MaxWidth * info.MaxWidth * 2, 0);
    unsigned char *ptr = buffer.data();
    size_t size = buffer.size();

    ASIStartVideoCapture(iCameraID);

    DeltaTime deltaTime;
    for(int i=0; i<40; ++i)
    {
        ASI_ERROR_CODE rc;
#ifdef CAMERABOOST_DISABLE
        rc = ASIGetVideoData(
            0,
            ptr,
            size,
            exposure * 2 + 500
        );
#else
        rc = ASIGetVideoDataPointer(iCameraID, &ptr, exposure * 2 + 500);
#endif

        printf("timeframe: %5.0fms, status: %d\n", deltaTime.stop() * 1000, rc);
    }

    ASIStopVideoCapture(iCameraID);
    ASICloseCamera(iCameraID);
    
    return 0;
}
