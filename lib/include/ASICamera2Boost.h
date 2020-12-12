#pragma once

#include <ASICamera2.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get the address to the frame buffer.
 * After the call, you will get the address to the buffer that contains data until the next function call.
 * You don't need to hurry to call functions as quickly as possible, as was the case with ASIGetVideoData.
 * The library provides a dynamic queue for frames.
 * You can safely receive data, process and save synchronously without worrying about losing the frame.
 * @param iCameraID index of connected camera
 * @param pBuffer address to the frame pointer
 * @param iWaitms this API will block and wait iWaitms to get one image. the unit is ms.
 * @return returns the same as the ASIGetVideoData function.
 */
ASICAMERA_API ASI_ERROR_CODE ASIGetVideoDataPointer(int iCameraID, unsigned char** pBuffer, int iWaitms);

/**
 * @brief set the limit of a single USB transfer
 * In the original library, the value is 1MB (1*1024*1024). This means that if the frame size is 7MB, it is split into 7 transfers.
 * If the buffer is split into more transfers, there are more delays and more dropped frames.
 * However, a high limit value may not match the kernel drivers or firmware of the camera.
 *
 * In this library, the default value is 256MB (256*1024*1024).
 * You can also set this value in the '/etc/asicamera2boost.conf' configuration file by typing:
 * max_chunk_size=268435456
 *
 * The configuration file is loaded when calling the ASIInitCamera function.
 * The value can be modified by calling the ASISetMaxChunkSize function.
 * @param iCameraID index of connected camera
 * @param maxChunkSize maximum USB transfer in bytes
 * @return
 *  ASI_SUCCESS success
 *  ASI_ERROR_INVALID_MODE library wrapper is disabled
 *  ASI_ERROR_INVALID_SEQUENCE stop capturing first
 */
ASICAMERA_API ASI_ERROR_CODE ASISetMaxChunkSize(int iCameraID, unsigned int maxChunkSize);

/**
 * @brief set the number of chunked transfers.
 * This parameter sets how many frame requests are to be active on USB.
 * In the original library, no such implementation (equals 1).
 * A value of 1 means that when a frame is received, it is put into a queue,
 * while there is no active request on the USB and it may cause a dropped frame.
 * My suggestion is a minimum value of 2 - When frame is putting into queue, at least one request for further data is active.
 * In the case of high-speed cameras on non-real time systems, it is necessary to provide more active requests.
 * Too much can be limited by the system and can cause errors.
 *
 * In this library, the default value is 3.
 * You can also set this value in the '/etc/asicamera2boost.conf' configuration file by typing:
 * chunked_transfers=3
 *
 * The configuration file is loaded when calling the ASIInitCamera function.
 * The value can be modified by calling the ASISetChunkedTransfers function.
 *
 * @param iCameraID index of connected camera
 * @param chunkedTransfers the number of active requests for whole frames
 * @return
 *  ASI_SUCCESS success
 *  ASI_ERROR_INVALID_MODE library wrapper is disabled
 *  ASI_ERROR_INVALID_SEQUENCE stop capturing first
 */
ASICAMERA_API ASI_ERROR_CODE ASISetChunkedTransfers(int iCameraID, unsigned int chunkedTransfers);

#ifdef __cplusplus
}
#endif
