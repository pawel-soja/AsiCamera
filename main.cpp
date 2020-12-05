#include <stdio.h>
#include <stdlib.h>

#include <limits>
#include <vector>
#include <chrono>
#include <numeric>
#include <cstring>
#include <ASICamera2.h>
#include <vector>

#include <unistd.h>

extern bool g_bDebugPrint;

extern "C" {
void * __real_memcpy ( void * destination, const void * source, size_t num) __attribute__((weak));
void * __real_memcpy ( void * destination, const void * source, size_t num);
void * __wrap_memcpy ( void * destination, const void * source, size_t num)
{
    if (source != destination)
    {
        if (num >= 1024*1024)
            fprintf(stderr, "[memcpy] Big copy detected: %d Bytes\n", num);
        
        //fprintf(stderr, "[memcpy]: copy %d\n", num);
    }
    return __real_memcpy(destination, source, num);
}
}

extern "C" ASICAMERA_API  ASI_ERROR_CODE ASIGetVideoDataPointer(int iCameraID, unsigned char** pBuffer, int iWaitms);

int main(int argc, char *argv[])
{
    g_bDebugPrint = true;


    if (ASIGetNumOfConnectedCameras() == 0)
    {
        fprintf(stderr, "No camera found\n");
        return -1;
    }
    int exposure = 1000;
    int bandWidth = 100;
    int highSpeedMode = 1;
    int gain = 100;
    std::vector<unsigned char> buffer(1024*1024*6*2, 0);
    std::vector<unsigned char> bufferCopy(1024*1024*6*2, 0);
    ASIOpenCamera(0);
    //return 0 ;
    ASIInitCamera(0);
    

    for(int i=1; i<(argc-1); i += 2)
    {
        if (!strcmp(argv[i], "Exposure"))
            exposure = atoi(argv[i+1]);
        else
        if (!strcmp(argv[i], "Gain"))
            gain = atoi(argv[i+1]);
        else
        if (!strcmp(argv[i], "BandWidth"))
            bandWidth = atoi(argv[i+1]);
        else
        if (!strcmp(argv[i], "HighSpeedMode"))
            highSpeedMode = atoi(argv[i+1]);
        else
            continue;

        printf("Set '%s' to %s\n", argv[i], argv[i+1]);
    }  

    ASISetControlValue(0, ASI_EXPOSURE,          exposure, ASI_FALSE);
    ASISetControlValue(0, ASI_HIGH_SPEED_MODE,   highSpeedMode, ASI_FALSE);
    ASISetControlValue(0, ASI_BANDWIDTHOVERLOAD, bandWidth, ASI_FALSE);
    
    
    ASISetControlValue(0, ASI_GAIN, gain,  ASI_FALSE);
    
    
    
    ASIStartVideoCapture(0);


    std::chrono::duration<double> diff;
    std::chrono::high_resolution_clock::time_point start, end;

    unsigned char *ptr = buffer.data();
    size_t size = buffer.size();
    FILE *f = NULL;
    //f = fopen("image.raw", "w");

    unsigned char *pbuffer;
    for(int i=0; i<100; ++i)
    {
        start = std::chrono::high_resolution_clock::now();
        ASI_ERROR_CODE rc;
#if 0
        rc = ASIGetVideoData(
            0,
            ptr,
            size,
            exposure * 2 + 500 + 100 * 1000
        );
#else
        rc = ASIGetVideoDataPointer(0, &pbuffer, exposure * 2 + 500 + 100 * 1000);
#endif
        if (f != NULL)
        {
            fwrite(ptr, 1, size, f);
            fclose(f);
            f = NULL;
        }
        //continue;
        
        //fprintf(stderr, "ASIGetVideoData: 0x%x\n", *(int*)ptr);
        //continue;
        end = std::chrono::high_resolution_clock::now();
        diff = end - start;
        printf("timeframe: %5.0fms\n", diff.count() * 1000);
    }
    ASIStopVideoCapture(0);

    ASICloseCamera(0);
    
    return 0;
}
