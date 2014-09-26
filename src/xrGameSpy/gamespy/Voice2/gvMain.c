/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

devsupport@gamespy.com
http://gamespy.net
*/

#include "gvMain.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvFrame.h"
#include "gvCustomDevice.h"
#if !defined(GV_NO_DEFAULT_HARDWARE)
	#if defined(_WIN32)
		#include "gvDirectSound.h"
	#elif defined(_PS2)
		#include "gvPS2Audio.h"
	#elif defined(_MACOSX)
		#include "gvOSXAudio.h"
	#elif defined(_PSP)
		#include "gvPSPAudio.h"
	#elif defined(_PS3)
		#include "gvPS3Audio.h"
	#else
		#error There is no default hardware support on this platform.  Define GV_NO_DEFAULT_HARDWARE to compile without default hardware support.
	#endif
#endif


/************
** GENERAL **
************/
#if defined(_WIN32)
GVBool gvStartup(HWND hWnd)
#else
GVBool gvStartup(void)
#endif
{
	// init codecs
	gviCodecsInitialize();

	GVIGlobalMute = GVFalse;

	// startup the devices
#if !defined(GV_NO_DEFAULT_HARDWARE)
	#if defined(_WIN32)
		if(!gviHardwareStartup(hWnd))
	#else
		if(!gviHardwareStartup())
	#endif
		{
			return GVFalse;
		}
#endif
	return GVTrue;
}

void gvCleanup(void)
{
#if !defined(GV_NO_DEFAULT_HARDWARE)
	gviHardwareCleanup();
#endif
	gviCodecsCleanup();
	gviFramesCleanup();
}

void gvThink(void)
{
#if !defined(GV_NO_DEFAULT_HARDWARE)
	gviHardwareThink();
#endif
}

/**********
** CODEC **
**********/
GVBool gvSetCodec(GVCodec codec)
{
	return gviSetCodec(codec);
}

void gvSetCustomCodec(GVCustomCodecInfo * info)
{
	gviSetCustomCodec(info);
}

void gvGetCodecInfo(int * samplesPerFrame, int * encodedFrameSize, int * bitsPerSecond)
{
	if(samplesPerFrame)
		*samplesPerFrame = GVISamplesPerFrame;
	if(encodedFrameSize)
		*encodedFrameSize = GVIEncodedFrameSize;
	if(bitsPerSecond)
		*bitsPerSecond = (int)(8 * GVIBytesPerSecond * GVIEncodedFrameSize / GVIBytesPerFrame);
}

/****************
** Sample Rate **
****************/
void gvSetSampleRate(GVRate sampleRate)
{
	gviSetSampleRate(sampleRate);
}

GVRate gvGetSampleRate(void)
{
	return gviGetSampleRate();
}

/************
** DEVICES **
************/
#if !defined(GV_NO_DEFAULT_HARDWARE)
int gvListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
	return gviHardwareListDevices(devices, maxDevices, types);
}

GVDevice gvNewDevice(GVDeviceID deviceID, GVDeviceType type)
{
	return gviHardwareNewDevice(deviceID, type);
}
#endif

void gvFreeDevice(GVDevice device)
{
	device->m_methods.m_freeDevice(device);
}

GVBool gvStartDevice(GVDevice device, GVDeviceType type)
{
	return device->m_methods.m_startDevice(device, type);
}

void gvStopDevice(GVDevice device, GVDeviceType type)
{
	device->m_methods.m_stopDevice(device, type);
}

GVBool gvIsDeviceStarted(GVDevice device, GVDeviceType type)
{
	return device->m_methods.m_isDeviceStarted(device, type);
}

void gvSetDeviceVolume(GVDevice device, GVDeviceType type, GVScalar volume)
{
	volume = max(volume, 0.0);
	volume = min(volume, 1.0);

	device->m_methods.m_setDeviceVolume(device, type, volume);
}

GVScalar gvGetDeviceVolume(GVDevice device, GVDeviceType type)
{
	return device->m_methods.m_getDeviceVolume(device, type);
}

void gvSetUnpluggedCallback(GVDevice device, gvUnpluggedCallback unpluggedCallback)
{
	device->m_unpluggedCallback = unpluggedCallback;
}

void gvSetFilter(GVDevice device, GVDeviceType type, gvFilterCallback callback)
{
	if(type & GV_CAPTURE)
		device->m_captureFilterCallback = callback;
	if(type & GV_PLAYBACK)
		device->m_playbackFilterCallback = callback;
}

/************
** CAPTURE **
************/
GVBool gvCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume)
{
	return device->m_methods.m_capturePacket(device, packet, len, frameStamp, volume);
}

int gvGetAvailableCaptureBytes(GVDevice device)
{
	return device->m_methods.m_getAvailableCaptureBytes(device);
}

void gvSetCaptureThreshold(GVDevice device, GVScalar threshold)
{
	device->m_methods.m_setCaptureThreshold(device, threshold);
}

GVScalar gvGetCaptureThreshold(GVDevice device)
{
	return device->m_methods.m_getCaptureThreshold(device);
}

void gvSetCaptureMode(GVDevice device, GVCaptureMode captureMode)
{
	//This only works with Capture devices.
	assert(device->m_types & GV_CAPTURE);

	//See if we are switching from Threshold to PTT.
	if ((device->m_captureMode == GVCaptureModeThreshold) && (captureMode == GVCaptureModePushToTalk))
	{
		device->m_savedCaptureThreshold = gvGetCaptureThreshold(device);
		gvSetCaptureThreshold(device, 0.0f);

		//Stop the capture device.
		if (gvIsDeviceStarted(device, GV_CAPTURE))
			gvStopDevice(device, GV_CAPTURE);
	}
	//See if we are switching from PTT to Threshold.
	if ((device->m_captureMode == GVCaptureModePushToTalk) && (captureMode == GVCaptureModeThreshold))
	{
		gvSetCaptureThreshold(device, device->m_savedCaptureThreshold);

		//Turn on the capture device.
		gvStartDevice(device, GV_CAPTURE);
	}

	device->m_captureMode = captureMode;
}

GVCaptureMode gvGetCaptureMode(GVDevice device)
{
	return device->m_captureMode;
}

void gvSetPushToTalk(GVDevice device, GVBool talkOn)
{
	if (talkOn)
		gvStartDevice(device, GV_CAPTURE);
	else
		gvStopDevice(device, GV_CAPTURE);
}

GVBool gvGetPushToTalk(GVDevice device)
{
	return gvIsDeviceStarted(device, GV_CAPTURE);
}

/*************
** PLAYBACK **
*************/
void gvPlayPacket(GVDevice device, const GVByte * data, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute)
{
	device->m_methods.m_playPacket(device, data, len, source, frameStamp, mute);
}

GVBool gvIsSourceTalking(GVDevice device, GVSource source)
{
	return device->m_methods.m_isSourceTalking(device, source);
}

int gvListTalkingSources(GVDevice device, GVSource sources[], int maxSources)
{
	return device->m_methods.m_listTalkingSources(device, sources, maxSources);
}

void gvSetGlobalMute(GVBool mute)
{
	gviSetGlobalMute(mute);
}

GVBool gvGetGlobalMute(void)
{
	return gviGetGlobalMute();
}


/******************
** CUSTOM DEVICE **
******************/
GVDevice gvNewCustomDevice(GVDeviceType type)
{
	return gviCustomNewDevice(type);
}

GVBool gvGetCustomPlaybackAudio(GVDevice device, GVSample * audio, int numSamples)
{
	return gviGetCustomPlaybackAudio(device, audio, numSamples);
}

GVBool gvSetCustomCaptureAudio(GVDevice device, const GVSample * audio, int numSamples,
                               GVByte * packet, int * packetLen, GVFrameStamp * frameStamp, GVScalar * volume)
{
	return gviSetCustomCaptureAudio(device, audio, numSamples, packet, packetLen, frameStamp, volume);
}

/*************
** CHANNELS **
*************/
int gvGetNumChannels(GVDevice device, GVDeviceType type)
{
	if(device->m_methods.m_getNumChannels)
		return device->m_methods.m_getNumChannels(device, type);
	return 0;
}

void gvGetChannelName(GVDevice device, GVDeviceType type, int channel, gsi_char name[GV_CHANNEL_NAME_LEN])
{
	if(device->m_methods.m_getNumChannels)
		device->m_methods.m_getChannelName(device, type, channel, name);
}

void gvSetChannel(GVDevice device, GVDeviceType type, int channel)
{
	if(device->m_methods.m_setChannel)
		device->m_methods.m_setChannel(device, type, channel);
}

int gvGetChannel(GVDevice device, GVDeviceType type)
{
	if(device->m_methods.m_getChannel)
		return device->m_methods.m_getChannel(device, type);
	return 0;
}
