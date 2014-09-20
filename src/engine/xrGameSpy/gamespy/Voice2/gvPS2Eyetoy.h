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

#ifndef _GV_PS2_EYETOY_H_
#define _GV_PS2_EYETOY_H_

#include "gvMain.h"

// this is set on all eyetoy GVDeviceID's
// it prevents a conflict with headset id's, which also use a 0-based index
#define GVI_EYETOY_DEVICEID_BIT              0x80000000

GVBool gviPS2EyetoyStartup(void);
void gviPS2EyetoyCleanup(void);

int gviPS2EyetoyListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types);

GVDevice gviPS2EyetoyNewDevice(GVDeviceID deviceID, GVDeviceType type);

#endif
