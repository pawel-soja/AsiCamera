#include <stdio.h>
#include <stdlib.h>

#include <limits>
#include <vector>
#include <numeric>
#include <cstring>
#include <vector>

#include <unistd.h>

#include "deltatime.h"

#include "asicamera.h"
#include "asicamerainfo.h"

#include "redirecttofile.h"

extern bool g_bDebugPrint;

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

void setArguments(AsiCamera &camera, int argc, char *argv[])
{
    for(int i=1; i<(argc-1); i += 2)
    {
        auto control = camera[std::string(argv[i])];
        if (!control.isValid())
        {
            printf("Unknown option: %s\n", argv[i]);
            continue;
        }

        if (!strcmp(argv[i+1], "auto"))
        {
            if (control.setAutoControl() == false)
                printf("Cannot set '%s' to auto mode\n", argv[i]);
        }
        else
        {
            if (control.set(atoi(argv[i+1])) == false)
            {
                printf("Cannot set '%s' to '%s'\n", argv[i], argv[i+1]);
                continue;
            }
        }

        printf("Set '%s' to %s\n", argv[i], argv[i+1]);
    }   
}

extern bool gBoostCameraEnabled;

int main(int argc, char *argv[])
{
    //g_bDebugPrint = true;

    RedirectToFile stdErr(STDERR_FILENO, "debug.log");
    gBoostCameraEnabled = false;
    
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
            continue;
        }

        setArguments(camera, argc, argv);

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


        printf("\nFind best bandwidth\n");
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");
        printf("| BW [%] |                                            Frame duration [ms]                                                         |\n");
        printf("|---------------------------------------------------------------------------------------------------------------------------------|\n");

        std::vector<uint8_t> frame(cameraInfo.maxWidth() * cameraInfo.maxHeight() * 2);

        void *buffer = frame.data();
        size_t bufferSize = frame.size();
        
        long bestBandWidth = 40;
        double bestTime = std::numeric_limits<double>::max();

        camera.startVideoCapture();

        for(int bandWidth = 40; bandWidth <= 100; bandWidth += 5)
        {

            printf("|  %3d%%  |", bandWidth);
        
            camera["BandWidth"] = bandWidth;
            double totalTime = 0;
            for(int i=0; i<20; ++i)
            {
                DeltaTime deltaTime;
#if 1
                bool ok = camera.getVideoData(buffer, bufferSize);
#else
                bool ok = camera.getVideoDataPointer(&buffer);
#endif
                double diff = deltaTime.stop();
                totalTime += diff;
                printf("%5.0f ", diff * 1000);
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
        
        camera.stopVideoCapture();
        camera.close();
    }
    return 0;
}
