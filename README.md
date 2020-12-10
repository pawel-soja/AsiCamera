# ASI Camera Boost

A wrapper for libASICamera2 library that improves performance at higher FPS.
This library wrapper is under development.
The library will be prepared for work with [INDI Library](https://github.com/indilib/indi) and [INDI 3rd Party Drivers](https://github.com/pawel-soja/indi-3rdparty)

# What is the problem in the original library?
When running the library on Raspberry PI 4, the problem becomes reaching the declared maximum FPS, e.g. for the ASI178 camera.
The reasons are:
- copying the same frame several times
- synchronous communication between frames on USB
- problems with handling bad frames

# What has been improved?
After an in-depth analysis of the original library, frame copying has been completely removed in libASICamera2Boost. In addition, more bulks transfers to USB ensure more stable operation.

# What is the difference in use?
Just swap the libraries!
If you want to get rid of unnecessary data copying from buffer to buffer, be sure to use ASIGetVideoDataPointer function.
With this function, you will get an address to the frame buffer. The data is available until the next function call. You can edit the data in the buffer for image transformations.

# Building

TODO


## Install Pre-requisites

On Debian/Ubuntu:

```
sudo apt-get -y install ...TODO...
```

## Get the code and build
```
mkdir -p ~/Projects/build/AsiCamera-build

cd ~/Projects
git clone https://github.com/pawel-soja/AsiCamera

cd ~/Projects/build/AsiCamera-build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release ~/Projects/AsiCamera
make -j4
```

## Test your camera

## Example - performance
Sample program that tests all connected cameras for different BandWidth values.
In the arguments of the program, you can specify camera options, For example:
```
$ ./example/performance/AsiCameraPerformance Exposure 10000 HighSpeedMode 1 Gain 100
Camera Found!

|-----------------------------------------------------------|
|        Parameter       |     Value                        |
|-----------------------------------------------------------|
| Name                   | ZWO ASI178MM                     |
| Camera Id              | 0                                |
| Max Height             | 2080                             |
| Max Width              | 3096                             |
| Is Color               | no                               |
| Bayer Pattern          | RG                               |
| Supported Bins         | 1, 2, 3, 4                       |
| Supported Video Format | Raw8, Raw16                      |
| Pixel Size             | 2.4                              |
| Mechanical Shutter     | no                               |
| ST4 Port               | yes                              |
| Is Cooled Camera       | no                               |
| Is USB3 Host           | yes                              |
| Is USB3 Camera         | yes                              |
| Elec Per Adu           | 0.289665                         |
| Bit Depth              | 14                               |
| Is Trigger Camera      | no                               |
|-----------------------------------------------------------|

Set 'Exposure' to 10000
Set 'HighSpeedMode' to 1
Set 'Gain' to 100

Camera Options
|------------------------------------------------------------------------------------------------------------------------------|
|        Option  Name      |  Value |   Min  |     Max    | Default  | Auto |                Description                       |
|------------------------------------------------------------------------------------------------------------------------------|
| Gain                     | 100    | 0      | 510        | 210      | no   | Gain                                             |
| Exposure                 | 10000  | 32     | 2000000000 | 10000    | no   | Exposure Time(us)                                |
| Offset                   | 10     | 0      | 600        | 10       | no   | offset                                           |
| BandWidth                | 100    | 40     | 100        | 50       | no   | The total data transfer rate percentage          |
| Flip                     | 0      | 0      | 3          | 0        | no   | Flip: 0->None 1->Horiz 2->Vert 3->Both           |
| AutoExpMaxGain           | 255    | 0      | 510        | 255      | no   | Auto exposure maximum gain value                 |
| AutoExpMaxExpMS          | 30000  | 1      | 60000      | 100      | no   | Auto exposure maximum exposure value(unit ms)    |
| AutoExpTargetBrightness  | 100    | 50     | 160        | 100      | no   | Auto exposure target brightness value            |
| HardwareBin              | 0      | 0      | 1          | 0        | no   | Is hardware bin2:0->No 1->Yes                    |
| HighSpeedMode            | 1      | 0      | 1          | 0        | no   | Is high speed mode:0->No 1->Yes                  |
| Temperature              | 266    | -500   | 1000       | 20       | no   | Sensor temperature(degrees Celsius)              |
|------------------------------------------------------------------------------------------------------------------------------|

Find best bandwidth
|---------------------------------------------------------------------------------------------------------------------------------|
| BW [%] |                                            Frame duration [ms]                                                         |
|---------------------------------------------------------------------------------------------------------------------------------|
|   40%  |  309    48    37    41    41    44    40    41    42    44    40    43    40    43    41    42    40    44    41    43 |
|   45%  |   22    38    38    37    37    37    36    36    38    39    38    37    37    37    38    35    36    38    37    39 |
|   50%  |   18    35    33    32    36    32    38    36    27    34    33    32    32    34    34    32    33    36    35    33 |
|   55%  |   14    32    31    31    31    28    29    33    28    31    33    29    32    29    29    32    30    29    32    29 |
|   60%  |   14    29    30    25    31    29    24    28    27    31    28    26    28    28    28    29    29    27    27    28 |
|   65%  |    8    27    25    28    27    24    25    27    25    27    26    26    25    26    25    27    24    25    26    26 |
|   70%  |    9    26    23    25    23    24    21    24    25    24    25    22    24    24    24    24    23    23    24    23 |
|   75%  |    9    23    22    22    21    24    22    23    23    22    22    22    23    22    22    23    22    22    22    21 |
|   80%  |    7    24    21    21    20    22    21    21    20    22    18    21    20    23    20    21    21    21    20    23 |
|   85%  |    6    18    22    18    19    20    20    20    19    20    21    20    19    20    20    19    21    19    20    19 |
|   90%  |    3    21    14    19    18    19    17    21    22    16    17    19    16    21    17    19    21    15    19    18 |
|   95%  |    5    17    19    15    18    17    17    18    20    18    17    18    18    14    20    19    18    18    17    16 |
|  100%  |    2    17    17    16    15    17    17    18    17    16    15    17    16    18    17    16    15    16    20    14 |
|---------------------------------------------------------------------------------------------------------------------------------|

Best bandwidth is 100%, it has 63.1 FPS
```
The table shows the waiting time for the frame for the given BandWidth value. Based on these values, the FPS value is calculated.

## Example - simply
A simplified program that measures time frames for specific parameters.
If you used the original libASICamera2 library, nothing will surprise you here.
The program is very useful for debugging a library.
```
$ ./example/simply/AsiCameraSimply Exposure 10000 HighSpeedMode 1 Gain 100
[ASIOpenCamera]: grab CameraID 0
[CameraBoost]: created
[__wrap_libusb_open]: grab libusb_device_handle 0x557a57792090
[__wrap__ZN11CCameraBase12InitVariableEv]: grab CCameraBase 0x557a577ab8d0
Set 'Exposure' to 10000
Set 'HighSpeedMode' to 1
Set 'Gain' to 100
[ResetDevice]: catched
[initAsyncXfer]: device 0x557a57792090, endpoint 0x81, buffer size 6439680
timeframe:   258ms, status: 0
timeframe:    20ms, status: 0
timeframe:    13ms, status: 0
timeframe:    17ms, status: 0
timeframe:    17ms, status: 0
timeframe:    19ms, status: 0
timeframe:    14ms, status: 0
timeframe:    20ms, status: 0
timeframe:    14ms, status: 0
timeframe:    19ms, status: 0
timeframe:    14ms, status: 0
timeframe:    19ms, status: 0
timeframe:    15ms, status: 0
timeframe:    19ms, status: 0
timeframe:    14ms, status: 0
timeframe:    19ms, status: 0
timeframe:    14ms, status: 0
timeframe:    19ms, status: 0
timeframe:    15ms, status: 0
timeframe:    19ms, status: 0
[releaseAsyncXfer]: catched
```
