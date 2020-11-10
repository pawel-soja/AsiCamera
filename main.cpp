#include <stdio.h>
#include <stdlib.h>

#include "asicamera.h"
#include "asicamerainfo.h"

#include <vector>
#include <chrono>
#include <numeric>

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

int main()
{
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

        printf("Openning camera...\n");
        if (!camera.open())
        {
            printf("Cannot open camera: %d\n", camera.errorCode());
        }

        printf("Set exposure to minimum...\n");
        auto exposure = camera["Exposure"];
        if (!exposure.set(exposure.min()))
            printf("Cannot set exposure: %s\n", exposure.statusString().data());

        printf("\n");
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

        std::vector<uint8_t> frame(cameraInfo.maxWidth() * cameraInfo.maxHeight() * 2);

        camera.startVideoCapture();

        printf("\nSpeed Performance Test...\n");
        
        // warming up
        long frames = 0;
        std::chrono::duration<double> diff;
        std::chrono::high_resolution_clock::time_point start, end;
        for(int i=0; i<10; ++i)
        {
            start = std::chrono::high_resolution_clock::now();
            camera.getVideoData(frame.data(), frame.size());
            end = std::chrono::high_resolution_clock::now();
            diff = end - start;
            printf("Expose duration[ %2d ]: %.1f ms\n", i+1, diff.count() * 1000);
        }

        printf("\nLong 5s test...\n");
        start = std::chrono::high_resolution_clock::now();
        for(int i=0; i<1000; ++i)
        {
            ++frames;
            camera.getVideoData(frame.data(), frame.size());
            end = std::chrono::high_resolution_clock::now();
            diff = end - start;
            if (diff.count() > 5)
                break;
        }

        camera.stopVideoCapture();
        camera.close();

        printf("FPS: %f\n", double(frames) / diff.count());
    }

    return 0;
}
