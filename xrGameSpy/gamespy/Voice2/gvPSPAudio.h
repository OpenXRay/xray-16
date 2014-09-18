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

#ifndef _GV_PSP_H_
#define _GV_PSP_H_

#include "gvMain.h"

GVBool gviHardwareStartup(void);
void gviHardwareCleanup(void);
void gviHardwareThink(void);

int gviHardwareListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types);

GVDevice gviHardwareNewDevice(GVDeviceID deviceID, GVDeviceType type);

#endif
