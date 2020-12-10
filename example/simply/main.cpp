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

static void setArguments(int iCameraID, int argc, char *argv[], int *exposure = nullptr)
{
    std::map<std::string, ASI_CONTROL_TYPE> options;
    options["Exposure"]      = ASI_EXPOSURE;
    options["HighSpeedMode"] = ASI_HIGH_SPEED_MODE;
    options["BandWidth"]     = ASI_BANDWIDTHOVERLOAD;
    options["Gain"]          = ASI_GAIN;

    for(int i=1; i<(argc-1); i += 2)
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
            continue;
        }
        else
        {
            int rc = ASISetControlValue(iCameraID, option->second, atoi(argv[i+1]), ASI_FALSE);
            if (rc != ASI_SUCCESS)
            {
                printf("Cannot set '%s' to '%s'\n", argv[i], argv[i+1]);
                continue;
            }
        }

        if (!strcmp(argv[i], "Exposure") && exposure)
            *exposure = atoi(argv[i+1]);

        printf("Set '%s' to %s\n", argv[i], argv[i+1]);        
    }
}

extern bool g_bDebugPrint;       // libASICamera2
extern bool gCameraBoostEnable;  // libASICamera2Boost

//#define BOOSTCAMERA_DISABLE

int main(int argc, char *argv[])
{
    //g_bDebugPrint = true;

#ifdef BOOSTCAMERA_DISABLE
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
    for(int i=0; i<20; ++i)
    {
        ASI_ERROR_CODE rc;
#ifdef BOOSTCAMERA_DISABLE
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
