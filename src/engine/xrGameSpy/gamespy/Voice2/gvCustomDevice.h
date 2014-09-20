/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

devsupport@gamespy.com
http://gamespy.net
*/

#ifndef _GV_CUSTOM_DEVICE_H_
#define _GV_CUSTOM_DEVICE_H_

#include "gvMain.h"
#include "gvDevice.h"

GVDevice gviCustomNewDevice(GVDeviceType type);

GVBool gviGetCustomPlaybackAudio(GVIDevice * device, GVSample * audio, int numSamples);

GVBool gviSetCustomCaptureAudio(GVDevice device, const GVSample * audio, int numSamples,
                                GVByte * packet, int * packetLen, GVFrameStamp * frameStamp, GVScalar * volume);

#endif
