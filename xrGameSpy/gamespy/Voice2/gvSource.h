/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

devsupport@gamespy.com
http://gamespy.net
*/

#ifndef _GV_SOURCE_H_
#define _GV_SOURCE_H_

#include "gvMain.h"
/************
** GLOBALS **
************/
extern GVBool GVIGlobalMute;

typedef struct GVISource * GVISourceList;

GVISourceList gviNewSourceList(void);
void gviFreeSourceList(GVISourceList sourceList);
void gviClearSourceList(GVISourceList sourceList);

GVBool gviIsSourceTalking(GVISourceList sourceList, GVSource source);
int gviListTalkingSources(GVISourceList sourceList, GVSource sources[], int maxSources);

void gviSetGlobalMute(GVBool mute);
GVBool gviGetGlobalMute(void);

void gviAddPacketToSourceList(GVISourceList sourceList,
							  const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute,
							  GVFrameStamp currentPlayClock);

GVBool gviWriteSourcesToBuffer(GVISourceList sourceList, GVFrameStamp startTime,
                               GVSample * sampleBuffer, int numFrames);

#endif
