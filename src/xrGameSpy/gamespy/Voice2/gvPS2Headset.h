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

#ifndef _GV_PS2_HEADSET_H_
#define _GV_PS2_HEADSET_H_

#include "gvMain.h"

GVBool gviPS2HeadsetStartup(void);
void gviPS2HeadsetCleanup(void);
void gviPS2HeadsetThink(void);

int gviPS2HeadsetListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types);

GVDevice gviPS2HeadsetNewDevice(GVDeviceID deviceID, GVDeviceType type);

#endif
