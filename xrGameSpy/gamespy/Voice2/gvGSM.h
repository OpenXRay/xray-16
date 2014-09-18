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

#ifndef _GV_GSM_H_
#define _GV_GSM_H_

#include "gvMain.h"

/*
quality: samplesPerFrame encodedFrameSize bitsPerSecond (at 11025hz)
All:           160              33            18200
*/

GVBool gviGSMInitialize(void);
void gviGSMCleanup(void);

int gviGSMGetSamplesPerFrame(void);
int gviGSMGetEncodedFrameSize(void);

GVBool gviGSMNewDecoder(GVDecoderData * data);
void gviGSMFreeDecoder(GVDecoderData data);

void gviGSMEncode(GVByte * out, const GVSample * in);
void gviGSMDecodeAdd(GVSample * out, const GVByte * in, GVDecoderData data);
void gviGSMDecodeSet(GVSample * out, const GVByte * in, GVDecoderData data);

#endif
