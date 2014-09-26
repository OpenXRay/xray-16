/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

devsupport@gamespy.com
http://gamespy.net
*/

#ifndef _GV_DEVICE_H_
#define _GV_DEVICE_H_

#include "gvMain.h"

// amount of time to keep capturing even after voice drops below the capture threshold
// makes sure that speech that trails off, or slightly dips in volume, is still captured
#define GVI_HOLD_THRESHOLD_FRAMES       20

// buffer sizes for capture and playback
#define GVI_PLAYBACK_BUFFER_MILLISECONDS     200
#define GVI_CAPTURE_BUFFER_MILLISECONDS      1000

/***********
** DEVICE **
***********/
typedef struct
{
	void (* m_freeDevice)(GVDevice device);

	GVBool (* m_startDevice)(GVDevice device, GVDeviceType type);
	void (* m_stopDevice)(GVDevice device, GVDeviceType type);
	GVBool (* m_isDeviceStarted)(GVDevice device, GVDeviceType type);

	void (* m_setDeviceVolume)(GVDevice device, GVDeviceType type, GVScalar volume);
	GVScalar (* m_getDeviceVolume)(GVDevice device, GVDeviceType type);

	void (* m_setCaptureThreshold)(GVDevice device, GVScalar threshold);
	GVScalar (* m_getCaptureThreshold)(GVDevice device);

	GVBool (*m_capturePacket)(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume);
	int (* m_getAvailableCaptureBytes)(GVDevice device);

	void (* m_playPacket)(GVDevice device, const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute);
	
	GVBool (* m_isSourceTalking)(GVDevice device, GVSource source);
	int (* m_listTalkingSources)(GVDevice device, GVSource sources[], int maxSources);

	int (* m_getNumChannels)(GVDevice device, GVDeviceType type);
	void (* m_getChannelName)(GVDevice device, GVDeviceType type, int channel, gsi_char name[GV_CHANNEL_NAME_LEN]);

	void (* m_setChannel)(GVDevice device, GVDeviceType type, int channel);
	int (* m_getChannel)(GVDevice device, GVDeviceType type);
} GVIDeviceMethods;

typedef struct GVIDevice
{
	GVDeviceID m_deviceID;
	GVHardwareType m_hardwareType;
	GVDeviceType m_types;
	GVIDeviceMethods m_methods;
	gvUnpluggedCallback m_unpluggedCallback;
	gvFilterCallback m_captureFilterCallback;
	gvFilterCallback m_playbackFilterCallback;
	void * m_data;
	GVCaptureMode m_captureMode;
	GVScalar m_savedCaptureThreshold;
} GVIDevice;

GVIDevice * gviNewDevice(GVDeviceID deviceID, GVHardwareType hardwareType, GVDeviceType types, int dataSize);
void gviFreeDevice(GVIDevice * device);

void gviDeviceUnplugged(GVIDevice * device);

/****************
** DEVICE LIST **
****************/
typedef DArray GVIDeviceList;

GVIDeviceList gviNewDeviceList(ArrayElementFreeFn elemFreeFn);
void gviFreeDeviceList(GVIDeviceList devices);

void gviAppendDeviceToList(GVIDeviceList devices, GVIDevice * device);
void gviDeleteDeviceFromList(GVIDeviceList devices, GVIDevice * device);

int gviGetNumDevices(GVIDeviceList devices);
GVIDevice * gviGetDevice(GVIDeviceList devices, int index);

#endif
