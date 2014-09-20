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

#ifndef _GV_PS2_SPU2_H_
#define _GV_PS2_SPU2_H_

#include "gvMain.h"

GVBool gviPS2Spu2Startup(void);
void gviPS2Spu2Cleanup(void);
void gviPS2Spu2Think(void);

int gviPS2Spu2ListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types);

GVDevice gviPS2Spu2NewDevice(GVDeviceID deviceID, GVDeviceType type);

#endif
