#pragma once


#include <libusb-1.0/libusb.h>
#include <pthread.h>
#include "ASICamera2.h"

extern bool g_bDebugPrint;

extern libusb_context *g_usb_context;
extern pthread_mutex_t mtx_usb;

class CCameraFX3
{
    char dummy[1];
public:
    CCameraFX3();
    ~CCameraFX3();

public:
    /* ... */
};

class CCameraBase: public CCameraFX3
{
    char dummy[1];
public:
    CCameraBase();
    virtual ~CCameraBase(); // important virtual

public:
    void InitVariable();
    /* ... */
};

class CirBuf
{
    char dummy[1];
public:
    CirBuf(long size);
    ~CirBuf();

public:
    void StartInstBufThr();
    int InsertBuff(
        unsigned char *buffer,   int bufferSize,
        unsigned short tagBegin, int tagBeginIndex,
        unsigned short tagEnd,   int tagEndIndex,
        int tagCmpIndex1,        int tagCmpIndex2
    );
    int ReadBuff(unsigned char *buffer, int bufferSize, int timeout);
    /* ... */
};
