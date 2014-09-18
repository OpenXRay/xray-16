/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

18002 Skypark Circle
Irvine, California 92614
949.798.4200 (Tel)
949.798.4299 (Fax)
devsupport@gamespy.com
http://gamespy.net
*/

#ifndef _GV_LOGITECH_PS2_CODECS_H_
#define _GV_LOGITECH_PS2_CODECS_H_

#include "gvMain.h"

/*
name:    samplesPerFrame encodedFrameSize bitsPerSecond
LPC10:         180               7             2489
SPEEX:         160              20             8000
GSM:           160              33            13200
G723.24:       160              60            24000
uLaw:          160             160            64000
PCM:           160             320           128000
*/

GVBool gviLGCodecInitialize(const char * name);
void gviLGCodecCleanup(void);

int gviLGCodecGetSamplesPerFrame(void);
int gviLGCodecGetEncodedFrameSize(void);

void gviLGCodecEncode(GVByte * out, const GVSample * in);
void gviLGCodecDecodeAdd(GVSample * out, const GVByte * in, GVDecoderData data);
void gviLGCodecDecodeSet(GVSample * out, const GVByte * in, GVDecoderData data);

#endif
