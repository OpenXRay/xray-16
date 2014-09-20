/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

devsupport@gamespy.com
http://gamespy.net
*/

#ifndef _GV_CODEC_H_
#define _GV_CODEC_H_

#include "gvMain.h"

/************
** GLOBALS **
************/
extern int GVISamplesPerFrame;
extern int GVIBytesPerFrame;
extern int GVIEncodedFrameSize;
extern GVRate GVISampleRate;			//In samples per second.
extern int GVIBytesPerSecond;

/**************
** FUNCTIONS **
**************/
void gviCodecsInitialize(void);
void gviCodecsCleanup(void);

GVBool gviSetCodec(GVCodec codec);
void gviSetCustomCodec(GVCustomCodecInfo * info);

void gviSetSampleRate(GVRate sampleRate);
GVRate gviGetSampleRate(void);

GVBool gviNewDecoder(GVDecoderData * data);
void gviFreeDecoder(GVDecoderData data);

void gviEncode(GVByte * out, const GVSample * in);
void gviDecodeAdd(GVSample * out, const GVByte * in, GVDecoderData data);
void gviDecodeSet(GVSample * out, const GVByte * in, GVDecoderData data);

void gviResetEncoder(void);

#endif
