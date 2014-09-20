/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

devsupport@gamespy.com
http://gamespy.net
*/

#ifndef _GV_DIRECT_SOUND_H_
#define _GV_DIRECT_SOUND_H_

#include "gvMain.h"

GVBool gviHardwareStartup(HWND hWnd);
void gviHardwareCleanup(void);
void gviHardwareThink(void);

int gviHardwareListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types);

GVDevice gviHardwareNewDevice(GVDeviceID deviceID, GVDeviceType type);

#endif
