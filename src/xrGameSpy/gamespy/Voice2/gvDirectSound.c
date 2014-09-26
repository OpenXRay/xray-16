#include <stdio.h>
#include <math.h>
#include "gvDirectSound.h"
#include "gvDevice.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvUtil.h"
#pragma warning(disable:4201)
#include <mmsystem.h>
#include <dsound.h>
#if (DIRECTSOUND_VERSION == 0x0800)
#include <dvoice.h>
#endif

#if !defined(_WIN32)
#error This file should only be used with Windows
#endif

/************
** DEFINES **
************/
// GUID utility macros
#define GVI_GUID_COPY(dest, src)          memcpy((dest), (src), sizeof(GUID))

// these were copied from dsound.h
const GUID GVDefaultPlaybackDeviceID = {0xdef00002, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03};
const GUID GVDefaultCaptureDeviceID  = {0xdef00003, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03};

/**********
** TYPES **
**********/
typedef struct
{
	GVBool m_playing;
	LPDIRECTSOUND m_playback;
	LPDIRECTSOUNDBUFFER m_playbackBuffer;
	DWORD m_playbackBufferSize;
	DWORD m_playbackBufferHalfSize;
	DWORD m_playbackBufferPosition;
	GVFrameStamp m_playbackClock;
	GVISourceList m_playbackSources;
	gsi_time m_playbackLastThink;

	GVBool m_capturing;
	LPDIRECTSOUNDCAPTURE m_capture;
	LPDIRECTSOUNDCAPTUREBUFFER m_captureBuffer;
	DWORD m_captureBufferSize;
	DWORD m_captureBufferPosition;
	GVScalar m_captureVolume;
	GVFrameStamp m_captureClock;
	GVScalar m_captureThreshold;
	GVFrameStamp m_captureLastCrossedThresholdTime;
} GVIHardwareData;

typedef struct
{
	GVDeviceInfo * m_devices;
	int m_len;
	int m_num;
	GVBool m_enumeratingCapture;
} GVIEnumDevicesInfo;

/************
** GLOBALS **
************/
static HWND GVIHwnd;
static GVIDeviceList GVIDevices;
static WAVEFORMATEX GVIWaveFormat;
static GVBool GVICleanupCOM;

/**************
** FUNCTIONS **
**************/
static void gviFreeArrayDevice(void * elem)
{
	GVIDevice * device = *(GVIDevice **)elem;
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(device->m_types == GV_PLAYBACK)
	{
		if(data->m_playbackSources)
			gviFreeSourceList(data->m_playbackSources);
		if(data->m_playbackBuffer)
			IDirectSoundBuffer_Release(data->m_playbackBuffer);
		if(data->m_playback)
			IDirectSound_Release(data->m_playback);
	}
	else
	{
		if(data->m_captureBuffer)
		{
			IDirectSoundCaptureBuffer_Stop(data->m_captureBuffer);
			IDirectSoundCaptureBuffer_Release(data->m_captureBuffer);
		}
		if(data->m_capture)
			IDirectSoundCapture_Release(data->m_capture);
	}

	gviFreeDevice(device);
}

GVBool gviHardwareStartup(HWND hWnd)
{
	HRESULT result;

	// check the hwnd
	if(!hWnd)
	{
		hWnd = GetForegroundWindow();
		if(!hWnd)
			hWnd = GetDesktopWindow();
	}

	// store it
	GVIHwnd = hWnd;

	// create the devices array
	GVIDevices = gviNewDeviceList(gviFreeArrayDevice);
	if(!GVIDevices)
		return GVFalse;

	// init COM
	assert(GVICleanupCOM == GVFalse);
	result = CoInitialize(NULL);
	if(SUCCEEDED(result))
		GVICleanupCOM = GVTrue;

	return GVTrue;
}

void gviHardwareCleanup(void)
{
	// cleanup the devices
	if(GVIDevices)
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
	}

	// cleanup COM
	if(GVICleanupCOM == GVTrue)
	{
		CoUninitialize();
		GVICleanupCOM = GVFalse;
	}
}

static GVBool gviPlaybackDeviceThink(GVIDevice * device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	HRESULT result;
	DWORD lockPosition;
	LPVOID audioPtr1, audioPtr2;
	DWORD audioLen1, audioLen2;
	GVBool wroteToBuffer;
	DWORD newBytes;
	DWORD rawPlayCursor;
	DWORD playCursor;
	DWORD writeCursor;
	gsi_time now;
	int diff;
	int numFrames;
	int i;

	// don't do anythying if we're not playing
	if(!data->m_playing)
		return GVTrue;

	// get the current position
	result = IDirectSoundBuffer_GetCurrentPosition(data->m_playbackBuffer, &rawPlayCursor, &writeCursor);
	if(FAILED(result))
		return GVFalse;

	// we only want to deal with whole frames
	playCursor = (DWORD)gviRoundDownToNearestMultiple((int)rawPlayCursor, GVIBytesPerFrame);

	// get the number of new bytes
	newBytes = (((playCursor + data->m_playbackBufferSize) - data->m_playbackBufferPosition) % data->m_playbackBufferSize);

	// before we store the new position, save the old one
	lockPosition = data->m_playbackBufferPosition;

	// store the new position
	data->m_playbackBufferPosition = playCursor;

	// figure out how long it has been since out last think
	now = current_time();
	diff = (int)(now - data->m_playbackLastThink);
	data->m_playbackLastThink = now;

	// adjust the time based on the number of new samples we know about
	diff -= gviDivideByBytesPerMillisecond(newBytes);

	// adjust again based on the raw play cursor
	// because newBytes is based on an adjusted play cursor
	diff -= (rawPlayCursor - playCursor);

	// diff should now be approximately 0
	// if it is closer to a multiple of the buffer length (in ms),
	// that is because we have missed at least one loop through the buffer
	// we use half of the buffer size because it is half way between
	// what we expect and what we are checking for
	if(diff >= gviDivideByBytesPerMillisecond((int)data->m_playbackBufferHalfSize))
	{
		int numMissedLoops;
		int numMissedFrames;
		int msPerBuffer = gviDivideByBytesPerMillisecond((int)data->m_playbackBufferSize);

		// estimate how many loops we missed
		numMissedLoops = (gviRoundToNearestMultiple(diff, msPerBuffer) / msPerBuffer);

		// convert to frames
		numMissedFrames = (numMissedLoops * (data->m_playbackBufferSize / GVIBytesPerFrame));

		// adjust the clock
		// Expanded to remove warnings in VS2K5
		data->m_playbackClock = data->m_playbackClock + (GVFrameStamp)(numMissedFrames);
	}

	// if we don't have any new bytes, there's nothing to do
	if(newBytes == 0)
		return GVTrue;

	// lock the appropriate half of the playback buffer
	result = IDirectSoundBuffer_Lock(data->m_playbackBuffer, lockPosition, newBytes, &audioPtr1, &audioLen1, &audioPtr2, &audioLen2, 0);
	if(FAILED(result))
		return GVFalse;

	// fill it
	numFrames = (audioLen1 / GVIBytesPerFrame);
	wroteToBuffer = gviWriteSourcesToBuffer(data->m_playbackSources, data->m_playbackClock, (GVSample *)audioPtr1, numFrames);
	if(!wroteToBuffer)
		memset(audioPtr1, 0, audioLen1);

	// filter
	if(device->m_playbackFilterCallback)
	{
		for(i = 0 ; i < numFrames ; i++)
			device->m_playbackFilterCallback(device, (GVSample *)audioPtr1 + (GVISamplesPerFrame * i), (GVFrameStamp)(data->m_playbackClock + i));
	}

	// update the clock
	// Expanded to remove warnings in VS2K5
	data->m_playbackClock = data->m_playbackClock + (GVFrameStamp)numFrames;

	// do the same for the second pointer, if it is set
	if(audioPtr2)
	{
		// fill it
		numFrames = (audioLen2 / GVIBytesPerFrame);
		wroteToBuffer = gviWriteSourcesToBuffer(data->m_playbackSources, data->m_playbackClock, (GVSample *)audioPtr2, numFrames);
		if(!wroteToBuffer)
			memset(audioPtr2, 0, audioLen2);

		// filter
		if(device->m_playbackFilterCallback)
		{
			for(i = 0 ; i < numFrames ; i++)
				device->m_playbackFilterCallback(device, (GVSample *)audioPtr2 + (GVISamplesPerFrame * i), (GVFrameStamp)(data->m_playbackClock + i));
		}

		// update the clock
		// Expanded to remove warnings in VS2K5
		data->m_playbackClock = data->m_playbackClock + (GVFrameStamp)numFrames;
	}

	// unlock the buffer
	result = IDirectSoundBuffer_Unlock(data->m_playbackBuffer, audioPtr1, audioLen1, audioPtr2, audioLen2);
	if(FAILED(result))
		return GVFalse;

	return GVTrue;
}

void gviHardwareThink(void)
{
	GVIDevice * device;
	GVBool rcode;
	int num;
	int i;

	if(!GVIDevices)
		return;

	// loop through backwards so that we can remove devices as we go
	num = gviGetNumDevices(GVIDevices);
	for(i = (num - 1) ; i >= 0 ; i--)
	{
		// get the device
		device = gviGetDevice(GVIDevices, i);

		// check if this is a playback device
		if(device->m_types == GV_PLAYBACK)
		{
			// let it think
			rcode = gviPlaybackDeviceThink(device);

			// check if the device was unplugged
			if(!rcode)
				gviDeviceUnplugged(device);
		}
	}
}

#if !defined(GSI_UNICODE)
	static BOOL CALLBACK gviEnumDevicesCallback(LPGUID lpGuid, LPCSTR desc, LPCSTR module, LPVOID lpContext)
#else
	static BOOL CALLBACK gviEnumDevicesCallback(LPGUID lpGuid, LPCWSTR desc, LPCWSTR module, LPVOID lpContext)
#endif
{
	if(lpGuid != NULL)
	{
		GVIEnumDevicesInfo * info = (GVIEnumDevicesInfo *)lpContext;
		GVDeviceInfo * device = NULL;
		int i;

		// first check if it is already in the list
		for(i = 0 ; i < info->m_num ; i++)
		{
			if(IsEqualGUID(&info->m_devices[i].m_id, lpGuid))
				device = &info->m_devices[i];
		}

		if(!device)
		{
			// if not, add it
			device = &info->m_devices[info->m_num++];

			// clear it
			memset(device, 0, sizeof(GVDeviceInfo));

			// store the ID and name
			GVI_GUID_COPY(&device->m_id, lpGuid);
			_tcsncpy(device->m_name, desc, GV_DEVICE_NAME_LEN);
			device->m_name[GV_DEVICE_NAME_LEN - 1] = '\0';
		}

		// store if it is a cap or playback device
		if(info->m_enumeratingCapture)
			device->m_deviceType = GV_CAPTURE;
		else
			device->m_deviceType = GV_PLAYBACK;

		// directsound device
		device->m_hardwareType = GVHardwareDirectSound;

		// check for full list
		if(info->m_num == info->m_len)
			return FALSE;
	}
	GSI_UNUSED(module);
	return TRUE;
}

int gviHardwareListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
#if DIRECTSOUND_VERSION >= 0x0800
	GUID defaultCaptureDevice;
	GUID defaultPlaybackDevice;
#endif
	GVIEnumDevicesInfo info;
	int i;

	// make sure there is space for at least one device
	if(maxDevices < 1)
		return 0;

	// enumerate capture and playback devices
	info.m_devices = devices;
	info.m_len = maxDevices;
	info.m_num = 0;
	if(types & GV_CAPTURE)
	{
		info.m_enumeratingCapture = GVTrue;
#if !defined(GSI_UNICODE)
		DirectSoundCaptureEnumerateA(gviEnumDevicesCallback, &info);
#else
		DirectSoundCaptureEnumerateW(gviEnumDevicesCallback, &info);
#endif
	}
	if(types & GV_PLAYBACK)
	{
		info.m_enumeratingCapture = GVFalse;
#if !defined(GSI_UNICODE)
		DirectSoundEnumerateA(gviEnumDevicesCallback, &info);
#else
		DirectSoundEnumerateW(gviEnumDevicesCallback, &info);
#endif
	}

#if DIRECTSOUND_VERSION >= 0x0800
	// check for the default capture and playback devices
	GetDeviceID(&DSDEVID_DefaultVoiceCapture, &defaultCaptureDevice);
	GetDeviceID(&DSDEVID_DefaultVoicePlayback, &defaultPlaybackDevice);
	for(i = 0 ; i < info.m_num ; i++)
	{
		if(IsEqualGUID(&devices[i].m_id, &defaultCaptureDevice))
			devices[i].m_defaultDevice = GV_CAPTURE;
		else if(IsEqualGUID(&devices[i].m_id, &defaultPlaybackDevice))
			devices[i].m_defaultDevice = GV_PLAYBACK;
	}
#else
	for(i = 0 ; i < info.m_num ; i++)
	{
		devices[i].m_defaultDevice = (GVDeviceType)0;
	}
#endif

	return info.m_num;
}

static void gviHardwareFreeDevice(GVIDevice * device)
{
	// delete it from the array
	// it will clear out internal data in the array's free function
	gviDeleteDeviceFromList(GVIDevices, device);
}

static GVBool gviStartPlaybackDevice(GVIHardwareData * data)
{
	HRESULT result;
	LPVOID audioPtr1, audioPtr2;
	DWORD audioLen1, audioLen2;

	// make sure the device is stopped
	IDirectSoundBuffer_Stop(data->m_playbackBuffer);

	// lock the buffer
	result = IDirectSoundBuffer_Lock(data->m_playbackBuffer, 0, 0, &audioPtr1, &audioLen1, &audioPtr2, &audioLen2, DSBLOCK_ENTIREBUFFER);
	if(FAILED(result))
		return GVFalse;

	// clear it
	memset(audioPtr1, 0, audioLen1);

	// unlock the buffer
	result = IDirectSoundBuffer_Unlock(data->m_playbackBuffer, audioPtr1, audioLen1, audioPtr2, audioLen2);
	if(FAILED(result))
		return GVFalse;

	// reset the position
	result = IDirectSoundBuffer_SetCurrentPosition(data->m_playbackBuffer, 0);
	if(FAILED(result))
		return GVFalse;

	// start the buffer
	result = IDirectSoundBuffer_Play(data->m_playbackBuffer, 0, 0, DSBPLAY_LOOPING);
	if(FAILED(result))
		return GVFalse;

	// clear the playback clocks
	data->m_playbackClock = 0;

	// init the think time
	data->m_playbackLastThink = current_time();

	// started playing
	data->m_playing = GVTrue;

	return GVTrue;
}

static GVBool gviStartCaptureDevice(GVIHardwareData * data)
{
	// started capturing
	data->m_capturing = GVTrue;

	return GVTrue;
}

static GVBool gviHardwareStartDevice(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type == GV_PLAYBACK)
		return gviStartPlaybackDevice(data);
	return gviStartCaptureDevice(data);
}

static void gviStopPlaybackDevice(GVIHardwareData * data)
{
	// stop the playback buffer
	IDirectSoundBuffer_Stop(data->m_playbackBuffer);

	// stopped playing
	data->m_playing = GVFalse;

	// clear any pending sources & buffers
	gviClearSourceList(data->m_playbackSources);
}

static void gviStopCaptureDevice(GVIHardwareData * data)
{
	// stopped capturing
	data->m_capturing = GVFalse;

	// reset the encoder's state
	gviResetEncoder();
}

static void gviHardwareStopDevice(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type == GV_PLAYBACK)
		gviStopPlaybackDevice(data);
	else
		gviStopCaptureDevice(data);
}

static GVBool gviIsPlaybackDeviceStarted(GVIHardwareData * data)
{
	HRESULT result;
	DWORD status;

	result = IDirectSoundBuffer_GetStatus(data->m_playbackBuffer, &status);
	if(FAILED(result))
		return GVFalse;

	if(status & DSBSTATUS_PLAYING)
		return GVTrue;
	return GVFalse;
}

static GVBool gviIsCaptureDeviceStarted(GVIHardwareData * data)
{
	return data->m_capturing;
}

static GVBool gviHardwareIsDeviceStarted(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type == GV_PLAYBACK)
		return gviIsPlaybackDeviceStarted(data);
	return gviIsCaptureDeviceStarted(data);
}

static void gviSetPlaybackDeviceVolume(GVIHardwareData * data, GVScalar volume)
{
	LONG vol;

	// convert from our scale to DS's scale
	if(volume == 0.0)
		vol = -10000;
	else
		vol = (LONG)(log(volume) * 2000);

	// set the volume
	IDirectSoundBuffer_SetVolume(data->m_playbackBuffer, vol);
}

static void gviSetCaptureDeviceVolume(GVIHardwareData * data, GVScalar volume)
{
	data->m_captureVolume = volume;
}

static void gviHardwareSetDeviceVolume(GVIDevice * device, GVDeviceType type, GVScalar volume)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type == GV_PLAYBACK)
		gviSetPlaybackDeviceVolume(data, volume);
	else
		gviSetCaptureDeviceVolume(data, volume);
}

static GVScalar gviGetPlaybackDeviceVolume(GVIHardwareData * data)
{
	HRESULT result;
	LONG vol;

	// get the volume
	result = IDirectSoundBuffer_GetVolume(data->m_playbackBuffer, &vol);
	if(FAILED(result))
		return (GVScalar)0;

	// convert it to our format
	return (GVScalar)(exp(vol / 2000.0));
}

static GVScalar gviGetCaptureDeviceVolume(GVIHardwareData * data)
{
	return data->m_captureVolume;
}

static GVScalar gviHardwareGetDeviceVolume(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type == GV_PLAYBACK)
		return gviGetPlaybackDeviceVolume(data);
	return gviGetCaptureDeviceVolume(data);
}

static void gviHardwareSetCaptureThreshold(GVIDevice * device, GVScalar threshold)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	data->m_captureThreshold = threshold;
}

static GVScalar gviHardwareGetCaptureThreshold(GVIDevice * device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	return data->m_captureThreshold;
}

static int gviHardwareGetAvailableCaptureBytes(GVDevice device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	HRESULT result;
	DWORD readPosition;
	DWORD newBytes;
	int numFrames;

	// get the current position
	result = IDirectSoundCaptureBuffer_GetCurrentPosition(data->m_captureBuffer, NULL, &readPosition);
	if(FAILED(result))
	{
		gviDeviceUnplugged(device);
		return 0;
	}

	// get the number of new bytes
	newBytes = (((readPosition + data->m_captureBufferSize) - data->m_captureBufferPosition) % data->m_captureBufferSize);

	// figure out how many frames this is
	numFrames = (newBytes / GVIBytesPerFrame);

	// calculate how many bytes this is once encoded
	newBytes = (numFrames * GVIEncodedFrameSize);

	return newBytes;
}

static GVBool gviHardwareCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	HRESULT result;
	DWORD readPosition;
	DWORD newBytes;
	LPVOID audioPtr1, audioPtr2;
	GVSample * audioPtr;
	DWORD audioLen, audioLen1, audioLen2;
	GVBool overThreshold = GVFalse;
	int numFrames;
	int i, j;
	int lenAvailable;
	int framesAvailable;

	// figure out how many encoded bytes they can handle
	lenAvailable = *len;

	// clear the len and volume
	*len = 0;
	if(volume)
		*volume = 0;

	// get the current position
	result = IDirectSoundCaptureBuffer_GetCurrentPosition(data->m_captureBuffer, NULL, &readPosition);
	if(FAILED(result))
	{
		gviDeviceUnplugged(device);
		return GVFalse;
	}

	// get the number of new bytes
	newBytes = (((readPosition + data->m_captureBufferSize) - data->m_captureBufferPosition) % data->m_captureBufferSize);

	// figure out how many frames this is
	numFrames = (newBytes / GVIBytesPerFrame);

	// figure out how many frames they can handle
	framesAvailable = (lenAvailable / GVIEncodedFrameSize);

	// don't give them more frames than they can handle
	numFrames = min(numFrames, framesAvailable);
	if(!numFrames)
		return GVFalse;

	// round off the bytes to a frame boundary
	newBytes = (numFrames * GVIBytesPerFrame);

	// only do this part if we are capturing
	if(data->m_capturing)
	{
		// lock the buffer
		result = IDirectSoundCaptureBuffer_Lock(data->m_captureBuffer, data->m_captureBufferPosition, newBytes, &audioPtr1, &audioLen1, &audioPtr2, &audioLen2, 0);
		if(FAILED(result))
		{
			gviDeviceUnplugged(device);
			return GVFalse;
		}

		// get the volume if requested
		if(volume)
		{
			GVScalar vol1 = gviGetSamplesVolume(audioPtr1, audioLen1 / GV_BYTES_PER_SAMPLE);
			GVScalar vol2 = gviGetSamplesVolume(audioPtr2, audioLen2 / GV_BYTES_PER_SAMPLE);
			*volume = max(vol1, vol2);
		}

		// check against the threshold
		if(volume)
		{
			// we already got the volume, so use that to check
			overThreshold = (*volume >= data->m_captureThreshold);
		}
		else
		{
			// we didn't get a volume, so check the samples directly
			overThreshold = gviIsOverThreshold(audioPtr1, audioLen1 / GV_BYTES_PER_SAMPLE, data->m_captureThreshold);

			// if not over threshold, and there is a second portion of audio, check it
			if(!overThreshold && audioPtr2)
				overThreshold = gviIsOverThreshold(audioPtr2, audioLen2 / GV_BYTES_PER_SAMPLE, data->m_captureThreshold);
		}

		// did the audio cross the threshold?
		if(overThreshold)
		{
			// update the time at which we crossed
			data->m_captureLastCrossedThresholdTime = data->m_captureClock;
		}
		else
		{
			// check if we are still within the hold time
			overThreshold = ((GVFrameStamp)(data->m_captureClock - data->m_captureLastCrossedThresholdTime) < GVI_HOLD_THRESHOLD_FRAMES);
		}

		if(overThreshold)
		{
			// store the frameStamp
			*frameStamp = data->m_captureClock;

			// setup the starting audio pointer and len
			audioPtr = audioPtr1;
			audioLen = audioLen1;

			// handle the data one frame at a time
			for(i = 0 ; i < numFrames ; i++)
			{
				// scale the data
				if(data->m_captureVolume < 1.0)
				{
					for(j = 0 ; j < GVISamplesPerFrame ; j++)
						audioPtr[j] = (GVSample)(audioPtr[j] * data->m_captureVolume);
				}

				// filter
				if(device->m_captureFilterCallback)
					device->m_captureFilterCallback(device, audioPtr, (GVFrameStamp)(data->m_captureClock + i));

				// encode the buffer into the packet
				gviEncode(packet + (GVIEncodedFrameSize * i), audioPtr);

				// update the loop info as needed
				if(audioLen > (DWORD)GVIBytesPerFrame)
				{
					audioPtr += GVISamplesPerFrame;
					audioLen -= GVIBytesPerFrame;
				}
				else
				{
					audioPtr = (GVSample*)audioPtr2;
					audioLen = audioLen2;
				}
			}
		}

		// unlock the buffer
		result = IDirectSoundCaptureBuffer_Unlock(data->m_captureBuffer, audioPtr1, audioLen1, audioPtr2, audioLen2);
		if(FAILED(result))
		{
			gviDeviceUnplugged(device);
			return GVFalse;
		}
	}

	// set the new position
	data->m_captureBufferPosition += newBytes;
	data->m_captureBufferPosition %= data->m_captureBufferSize;

	// increment the clock
	// Expanded to remove warnings in VS2K5
	data->m_captureClock = data->m_captureClock + (GVFrameStamp)numFrames;

	// set the len
	*len = (numFrames * GVIEncodedFrameSize);

	// return false if we didn't get a packet
	if(!overThreshold)
		return GVFalse;

	return GVTrue;
}

static void gviHardwarePlayPacket(GVIDevice * device, const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// don't do anythying if we're not playing
	if(!data->m_playing)
		return;

	//add it
	gviAddPacketToSourceList(data->m_playbackSources, packet, len, source, frameStamp, mute, data->m_playbackClock);
}

static GVBool gviHardwareIsSourceTalking(GVDevice device, GVSource source)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// don't do anythying if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviIsSourceTalking(data->m_playbackSources, source);
}

static int gviHardwareListTalkingSources(GVDevice device, GVSource sources[], int maxSources)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// don't do anythying if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviListTalkingSources(data->m_playbackSources, sources, maxSources);
}

static GVBool gviInitPlaybackDevice(GVIHardwareData * data)
{
	HRESULT result;
	DSBUFFERDESC bufferDescriptor;

	// set the cooperative level
	result = IDirectSound_SetCooperativeLevel(data->m_playback, GVIHwnd, DSSCL_NORMAL);
	if(FAILED(result))
		return GVFalse;

	// setup the buffer size
	data->m_playbackBufferSize = gviMultiplyByBytesPerMillisecond(GVI_PLAYBACK_BUFFER_MILLISECONDS);

	// make sure it is a multiple of twice the raw frame size
	// it needs to be twice the size because it will be split in two
	data->m_playbackBufferSize = gviRoundUpToNearestMultiple(data->m_playbackBufferSize, GVIBytesPerFrame * 2);

	// we use this a lot, so calc it here
	data->m_playbackBufferHalfSize = (data->m_playbackBufferSize / 2);

	// set up the buffer descriptor
	memset(&bufferDescriptor, 0, sizeof(DSBUFFERDESC));
	bufferDescriptor.dwSize = sizeof(DSBUFFERDESC);
	bufferDescriptor.dwFlags = DSBCAPS_CTRLVOLUME|DSBCAPS_GETCURRENTPOSITION2;
#if !defined(GV_NO_GLOBAL_FOCUS)
	bufferDescriptor.dwFlags |= DSBCAPS_GLOBALFOCUS;
#endif
	bufferDescriptor.dwBufferBytes = (DWORD)data->m_playbackBufferSize;
	bufferDescriptor.lpwfxFormat = &GVIWaveFormat;

	// create the buffer
	result = IDirectSound_CreateSoundBuffer(data->m_playback, &bufferDescriptor, &data->m_playbackBuffer, NULL);
	if(FAILED(result))
		return GVFalse;

	return GVTrue;
}

static GVBool gviInitCaptureDevice(GVIHardwareData * data)
{
	HRESULT result;
	DSCBUFFERDESC bufferDescriptor;

	// setup the buffer size
	data->m_captureBufferSize = gviMultiplyByBytesPerMillisecond(GVI_CAPTURE_BUFFER_MILLISECONDS);

	// make sure it is a multiple of the raw frame size
	data->m_captureBufferSize = gviRoundUpToNearestMultiple(data->m_captureBufferSize, GVIBytesPerFrame);

	// setup the buffer descriptor
	memset(&bufferDescriptor, 0, sizeof(bufferDescriptor));
	bufferDescriptor.dwSize = sizeof(DSCBUFFERDESC);
	bufferDescriptor.dwBufferBytes = (DWORD)data->m_captureBufferSize;
	bufferDescriptor.lpwfxFormat = &GVIWaveFormat;

	// create the buffer
	result = IDirectSoundCapture_CreateCaptureBuffer(data->m_capture, &bufferDescriptor, &data->m_captureBuffer, NULL);
	if(FAILED(result))
		return GVFalse;

	// start the buffer
	result = IDirectSoundCaptureBuffer_Start(data->m_captureBuffer, DSCBSTART_LOOPING);
	if(FAILED(result))
	{
		IDirectSoundCaptureBuffer_Release(data->m_captureBuffer);
		data->m_captureBuffer = NULL;
		return GVFalse;
	}

	return GVTrue;
}

GVDevice gviHardwareNewDevice(GVDeviceID deviceID, GVDeviceType type)
{
	GVIDevice * device;
	GVIHardwareData * data;
	HRESULT result;

	// DS can only handle one type or the other
	if((type != GV_CAPTURE) && (type != GV_PLAYBACK))
		return NULL;

	//setup the wave format
	memset(&GVIWaveFormat, 0, sizeof(WAVEFORMATEX));
	GVIWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	GVIWaveFormat.nChannels = 1;
	GVIWaveFormat.nSamplesPerSec = GVISampleRate;
	GVIWaveFormat.wBitsPerSample = GV_BITS_PER_SAMPLE;
	GVIWaveFormat.nBlockAlign = ((GVIWaveFormat.nChannels * GVIWaveFormat.wBitsPerSample) / 8); 
	GVIWaveFormat.nAvgBytesPerSec = (GVIWaveFormat.nSamplesPerSec * GVIWaveFormat.nBlockAlign);


	// create a new device
	device = gviNewDevice(deviceID, GVHardwareDirectSound, type, sizeof(GVIHardwareData));
	if(!device)
		return NULL;

	// get a pointer to the data
	data = (GVIHardwareData *)device->m_data;

	// store the pointers
	device->m_methods.m_freeDevice = gviHardwareFreeDevice;
	device->m_methods.m_startDevice = gviHardwareStartDevice;
	device->m_methods.m_stopDevice = gviHardwareStopDevice;
	device->m_methods.m_isDeviceStarted = gviHardwareIsDeviceStarted;
	device->m_methods.m_setDeviceVolume = gviHardwareSetDeviceVolume;
	device->m_methods.m_getDeviceVolume = gviHardwareGetDeviceVolume;
	device->m_methods.m_setCaptureThreshold = gviHardwareSetCaptureThreshold;
	device->m_methods.m_getCaptureThreshold = gviHardwareGetCaptureThreshold;
	device->m_methods.m_getAvailableCaptureBytes = gviHardwareGetAvailableCaptureBytes;
	device->m_methods.m_capturePacket = gviHardwareCapturePacket;
	device->m_methods.m_playPacket = gviHardwarePlayPacket;
	device->m_methods.m_isSourceTalking = gviHardwareIsSourceTalking;
	device->m_methods.m_listTalkingSources = gviHardwareListTalkingSources;

	// check if they want to init playback
	if(type == GV_PLAYBACK)
	{
		// create the array of sources
		data->m_playbackSources = gviNewSourceList();
		if(!data->m_playbackSources)
		{
			gviFreeDevice(device);
			return NULL;
		}

		// create the interface
		result = DirectSoundCreate(&deviceID, &data->m_playback, NULL);
		if(FAILED(result))
		{
			gviFreeSourceList(data->m_playbackSources);
			gviFreeDevice(device);
			return NULL;
		}

		// setup the playback device
		if(!gviInitPlaybackDevice(data))
		{
			gviFreeSourceList(data->m_playbackSources);
			IDirectSound_Release(data->m_playback);
			gviFreeDevice(device);
			return NULL;
		}
	}
	// check if they want to init capture
	else if(type == GV_CAPTURE)
	{
		// create the interface
		result = DirectSoundCaptureCreate(&deviceID, &data->m_capture, NULL);
		if(FAILED(result))
		{
			gviFreeDevice(device);
			return NULL;
		}

		// setup the capture device
		if(!gviInitCaptureDevice(data))
		{
			IDirectSoundCapture_Release(data->m_capture);
			gviFreeDevice(device);
			return NULL;
		}

		// set some data vars
		data->m_captureVolume = 1.0;
		data->m_captureClock = 0;
		data->m_captureLastCrossedThresholdTime = (data->m_captureClock - GVI_HOLD_THRESHOLD_FRAMES - 1);
	}

	// add it to the list
	gviAppendDeviceToList(GVIDevices, device);

	return device;
}

#if (DIRECTSOUND_VERSION == 0x0800)
static LPDIRECTPLAYVOICETEST GetVoiceTestInstance(void)
{
	LPDIRECTPLAYVOICETEST voiceTest;
	HRESULT result;

	// create the IDirectPlayVoiceTest object
	result = CoCreateInstance(&CLSID_DirectPlayVoiceTest, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayVoiceTest, (LPVOID*)&voiceTest);

	if(FAILED(result))
		return NULL;

	return voiceTest;
}

GVBool gvRunSetupWizard(GVDeviceID captureDeviceID, GVDeviceID playbackDeviceID)
{
	LPDIRECTPLAYVOICETEST voiceTest;
	HRESULT result;

	// get the voice test instance
	voiceTest = GetVoiceTestInstance();
	if(!voiceTest)
		return GVFalse;

	// run the dialog
	result = IDirectPlayVoiceTest_CheckAudioSetup(voiceTest, &playbackDeviceID, &captureDeviceID, GVIHwnd, 0);

	// releate the instance
	IDirectPlayVoiceTest_Release(voiceTest);

	// check for success
	if((result == DV_OK) || (result == DV_FULLDUPLEX) || (result == DV_HALFDUPLEX))
		return GVTrue;

	return GVFalse;
}

GVBool gvAreDevicesSetup(GVDeviceID captureDevice, GVDeviceID playbackDevice)
{
	LPDIRECTPLAYVOICETEST voiceTest;
	HRESULT result;

	// get the voice test instance
	voiceTest = GetVoiceTestInstance();
	if(!voiceTest)
		return GVFalse;

	// run the dialog
	result = IDirectPlayVoiceTest_CheckAudioSetup(voiceTest, &playbackDevice, &captureDevice, GVIHwnd, DVFLAGS_QUERYONLY);

	// releate the instance
	IDirectPlayVoiceTest_Release(voiceTest);

	// check for success
	if((result == DV_FULLDUPLEX) || (result == DV_HALFDUPLEX))
		return GVTrue;

	return GVFalse;
}
#endif
