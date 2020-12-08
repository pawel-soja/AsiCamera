#pragma once

#include <ASICamera2.h>

#ifdef __cplusplus
extern "C" {
#endif

ASICAMERA_API  ASI_ERROR_CODE ASIGetVideoDataPointer(int iCameraID, unsigned char** pBuffer, int iWaitms);

#ifdef __cplusplus
}
#endif
