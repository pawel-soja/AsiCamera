/*
    Copyright (C) 2020 by Pawel Soja <kernel32.pl@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "asicamerainfo.h"
#include "asicamerainfo_p.h"

#include "ASICamera2.h"

AsiCameraInfoPrivate::AsiCameraInfoPrivate()
{ }

AsiCameraInfoPrivate::~AsiCameraInfoPrivate()
{ }

AsiCameraInfo::AsiCameraInfo()
{ }

AsiCameraInfo::~AsiCameraInfo()
{ }

AsiCameraInfo::AsiCameraInfo(const AsiCamera &camera)
{

}

AsiCameraInfo::AsiCameraInfo(AsiCameraInfoPrivate &dd)
    : d_ptr(&dd)
{ }


bool AsiCameraInfo::isValid() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr;
}

std::string AsiCameraInfo::name() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.Name : "";
}

int AsiCameraInfo::cameraId() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.CameraID : 0;
}

long AsiCameraInfo::maxHeight() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.MaxHeight : 0;
}

long AsiCameraInfo::maxWidth() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.MaxWidth : 0;
}

bool AsiCameraInfo::isColor() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.IsColorCam : false;
}

AsiCameraInfo::BayerPattern AsiCameraInfo::bayerPattern() const
{
    const AsiCameraInfoPrivate *d = d_func();

    if (d == nullptr)
        return BayerPatternUnknown;

    switch(d->info.BayerPattern)
    {
    case ASI_BAYER_RG: return BayerPatternRG;
    case ASI_BAYER_BG: return BayerPatternBG;
    case ASI_BAYER_GR: return BayerPatternGR;
    case ASI_BAYER_GB: return BayerPatternGB;
    default:           return BayerPatternUnknown;
    }
}

std::string AsiCameraInfo::bayerPatternAsString() const
{
    switch(bayerPattern())
    {
    case BayerPatternRG:      return "RG";
    case BayerPatternBG:      return "BG";
    case BayerPatternGR:      return "GR";
    case BayerPatternGB:      return "GB";
    case BayerPatternUnknown: return "Unknown";
    default:                  return "Unknown";
    } 
}

std::list<int> AsiCameraInfo::supportedBins() const
{
    const AsiCameraInfoPrivate *d = d_func();
    std::list<int> result;

    if (d == nullptr)
        return result;

    const int count =
        sizeof( d->info.SupportedBins) /
        sizeof(*d->info.SupportedBins);

    for(int j=0; j<count && d->info.SupportedBins[j] != 0; ++j)
        result.push_back(d->info.SupportedBins[j]);

    return result;
}

std::list<AsiCameraInfo::VideoFormat> AsiCameraInfo::supportedVideoFormat() const
{
    const AsiCameraInfoPrivate *d = d_func();
    std::list<VideoFormat> result;

    if (d == nullptr)
        return result;

    const int count =
        sizeof( d->info.SupportedVideoFormat) /
        sizeof(*d->info.SupportedVideoFormat);

    for(int j=0; j<8 && d->info.SupportedVideoFormat[j] >= 0; ++j)
        switch(d->info.SupportedVideoFormat[j])
        {
        case ASI_IMG_RAW8:  result.push_back(VideoFormatRaw8);    break;
        case ASI_IMG_RGB24: result.push_back(VideoFormatRgb24);   break;
        case ASI_IMG_RAW16: result.push_back(VideoFormatRaw16);   break;
        case ASI_IMG_Y8:    result.push_back(VideoFormatY8);      break;
        default:            result.push_back(VideoFormatUnknown); break;
        }

    return result;
}

std::list<std::string> AsiCameraInfo::supportedVideoFormatAsStringList() const
{
    std::list<std::string> result;
    for(auto &format: supportedVideoFormat())
        switch(format)
        {
        case VideoFormatRaw8:    result.push_back("Raw8");    break;
        case VideoFormatRgb24:   result.push_back("Rgb24");   break;
        case VideoFormatRaw16:   result.push_back("Raw16");   break;
        case VideoFormatY8:      result.push_back("Y8");      break;
        case VideoFormatUnknown: result.push_back("Unknown"); break;
        default:                 result.push_back("Unknown"); break;
        }
    return result;
}

double AsiCameraInfo::pixelSize() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.PixelSize : 0;
}

bool AsiCameraInfo::mechanicalShutter() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.MechanicalShutter : false;
}

bool AsiCameraInfo::st4Port() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.ST4Port : false;
}

bool AsiCameraInfo::isCooled() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.IsCoolerCam : false;
}

bool AsiCameraInfo::isUsb3Host() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.IsUSB3Host : false;
}

bool AsiCameraInfo::isUsb3Camera() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.IsUSB3Camera : false;
}

double AsiCameraInfo::elecPerADU() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.ElecPerADU : 0;
}

int AsiCameraInfo::bitDepth() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.BitDepth : 0;
}

bool AsiCameraInfo::isTriggerCamera() const
{
    const AsiCameraInfoPrivate *d = d_func();
    return d != nullptr ? d->info.IsTriggerCam : false;
}

std::list<AsiCameraInfo> AsiCameraInfo::availableCameras()
{
    std::list<AsiCameraInfo> cameras;

    int numofConnectCameras = ASIGetNumOfConnectedCameras();
    
    for(int i=0; i<numofConnectCameras; ++i)
    {
        AsiCameraInfoPrivate *priv = new AsiCameraInfoPrivate;
        ASIGetCameraProperty(&priv->info, i);

        cameras.push_back(*priv);
    }
    
    return cameras;
}

AsiCameraInfoPrivate * AsiCameraInfo::d_func() const
{
    return static_cast<AsiCameraInfoPrivate*>(d_ptr.get());
}
