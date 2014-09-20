#include "gvPS2Headset.h"
#if !defined(GV_NO_PS2_HEADSET)
#include "gvDevice.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvUtil.h"
#include <liblgaud.h>

#if !defined(_PS2)
#error This file should only be used with the PlayStation2
#endif

/************
** DEFINES **
************/
#define GVI_PLAYBACK_STAYAHEAD_MILLISECONDS  50

/**********
** TYPES **
**********/
typedef struct
{
	int m_handle;

	GVBool m_playing;
	GVScalar m_playbackVolume;
	GVFrameStamp m_playbackClock;
	GVISourceList m_playbackSources;
	GVSample * m_playbackBuffer;

	GVBool m_capturing;
	GVScalar m_captureVolume;
	GVFrameStamp m_captureClock;
	GVScalar m_captureThreshold;
	GVFrameStamp m_captureLastCrossedThresholdTime;
	GVSample * m_captureBuffer;
	int m_captureBufferBytes;
} GVIPS2HeadsetData;

/************
** GLOBALS **
************/
static GVIDeviceList GVIDevices;

/**************
** FUNCTIONS **
**************/
static GVIDevice * gviFindDeviceByID(GVDeviceID deviceID)
{
	GVIDevice * device;
	int num;
	int i;

	num = gviGetNumDevices(GVIDevices);
	for(i = 0 ; i < num ; i++)
	{
		device = gviGetDevice(GVIDevices, i);
		if(device->m_deviceID == deviceID)
			return device;
	}

	return NULL;
}

static void gviFreeArrayDevice(void * elem)
{
	GVIDevice * device = *(GVIDevice **)elem;
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	// close the device
	lgAudClose(data->m_handle);

	// playback specific cleanup
	if(device->m_types & GV_PLAYBACK)
	{
		if(data->m_playbackSources)
			gviFreeSourceList(data->m_playbackSources);
		gsifree(data->m_playbackBuffer);
	}

	// capture specific cleanup
	if(device->m_types & GV_CAPTURE)
	{
		gsifree(data->m_captureBuffer);
	}

	// free the device
	gviFreeDevice(device);
}

GVBool gviPS2HeadsetStartup(void)
{
	int result;

	// create the array of devices
	GVIDevices = gviNewDeviceList(gviFreeArrayDevice);
	if(!GVIDevices)
		return GVFalse;

	// initialize the headset library
	result = lgAudInit(gsimalloc, gsifree);
	if(LGAUD_FAILED(result))
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
		return GVFalse;
	}

	return GVTrue;
}

void gviPS2HeadsetCleanup(void)
{
	// free the device array
	if(GVIDevices)
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
	}

	// cleanup the headset library
	lgAudDeInit();
}

static GVBool gviPlaybackDeviceThink(GVIDevice * device)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;
	int result;
	int remainingBytes;
	int numFrames;
	int newBytes;
	GVBool wroteToBuffer;
	int size;
	int i;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVTrue;

	// get the remaining bytes
	result = lgAudGetRemainingPlaybackBytes(data->m_handle, &remainingBytes);
	if(LGAUD_FAILED(result))
		return GVFalse;

	// figure out how many bytes we need to write to stay ahead
	newBytes = (gviMultiplyByBytesPerMillisecond(GVI_PLAYBACK_STAYAHEAD_MILLISECONDS) - remainingBytes);
	newBytes = gviRoundUpToNearestMultiple(newBytes, GVIBytesPerFrame);

	// figure out the number of frames that is
	numFrames = (newBytes / GVIBytesPerFrame);

	// write the frames
	for(i = 0 ; i < numFrames ; i++)
	{
		// write a frame of sources to our buffer
		wroteToBuffer = gviWriteSourcesToBuffer(data->m_playbackSources, data->m_playbackClock, data->m_playbackBuffer, 1);

		// clear it if nothing was written
		if(!wroteToBuffer)
			memset(data->m_playbackBuffer, 0, (unsigned int)GVIBytesPerFrame);

		// filter
		if(device->m_playbackFilterCallback)
			device->m_playbackFilterCallback(device, data->m_playbackBuffer, data->m_playbackClock);

		// send it to the device
		size = GVIBytesPerFrame;
		result = lgAudWrite(data->m_handle, LGAUD_BLOCKMODE_BLOCKING, (u_char *)data->m_playbackBuffer, &size);
		if(LGAUD_FAILED(result))
			return GVFalse;

		// update the clock
		data->m_playbackClock++;
	}

	return GVTrue;
}

void gviPS2HeadsetThink(void)
{
	GVIDevice * device;
	GVBool rcode;
	int num;
	int i;

	if(!GVIDevices)
		return;

	// loop through the devices backwards to that we can remove devices as we go
	num = gviGetNumDevices(GVIDevices);
	for(i = (num - 1) ; i >= 0 ; i--)
	{
		// get the device
		device = gviGetDevice(GVIDevices, i);

		// // check if playback is setup on the device
		if(device->m_types & GV_PLAYBACK)
		{
			// let it think
			rcode = gviPlaybackDeviceThink(device);

			// check if the device was unplugged
			if(!rcode)
				gviDeviceUnplugged(device);
		}
	}
}

static GVBool gviDoesSupportOurFormat(lgAudSamplingFormat * formats, int count)
{
	int i;

	for(i = 0 ; i < count ; i++)
	{
		if(formats[i].Channels == 1)
			if(formats[i].BitResolution == 16)
				if(formats[i].LowerSamplingRate <= GV_SAMPLES_PER_SECOND)
					if(formats[i].HigherSamplingRate >= GV_SAMPLES_PER_SECOND)
						return GVTrue;
	}

	return GVFalse;
}

int gviPS2HeadsetListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
	lgAudDeviceDesc headsetDesc;
	GVDeviceType supportedTypes;
	int result;
	int index;
	int numDevices = 0;

enumerate:
	for(index = 0 ; numDevices < maxDevices ; index++)
	{
		// get the indexed device
		result = lgAudEnumerate(index, &headsetDesc);

		// check for the unplugged device error
		if(result == LGAUD_ERR_DEVICE_LOST)
		{
			// as recommended by the docs, restart the enumeration
			numDevices = 0;
			goto enumerate;
		}

		// check for any other error
		if(LGAUD_FAILED(result))
			break;

		// check what this device supports
		supportedTypes = (GVDeviceType)0;
		if(gviDoesSupportOurFormat(headsetDesc.RecordingFormats, headsetDesc.RecordingFormatsCount))
			supportedTypes |= GV_CAPTURE;
		if(gviDoesSupportOurFormat(headsetDesc.PlaybackFormats, headsetDesc.PlaybackFormatsCount))
			supportedTypes |= GV_PLAYBACK;

		// only add it if it supports a format for a type that the app requested
		// this library doesn't support device names or default devices
		if(supportedTypes & types)
		{
			// store this device's info in the array
			devices[numDevices].m_id = index;
			devices[numDevices].m_deviceType = supportedTypes;
			devices[numDevices].m_defaultDevice = (GVDeviceType)0; // ps2 doesn't support default devices

			// set the name and the hardware type based on the capabilities
			if(supportedTypes == GV_CAPTURE)
			{
				_tcscpy(devices[numDevices].m_name, _T("USB Microphone"));
				devices[numDevices].m_hardwareType = GVHardwarePS2Microphone;
			}
			else if(supportedTypes == GV_PLAYBACK)
			{
				_tcscpy(devices[numDevices].m_name, _T("USB Speakers"));
				devices[numDevices].m_hardwareType = GVHardwarePS2Speakers;
			}
			else
			{
				_tcscpy(devices[numDevices].m_name, _T("USB Headset"));
				devices[numDevices].m_hardwareType = GVHardwarePS2Headset;
			}

			// one more device
			numDevices++;
		}
	}

	// return the number of devices we found
	return numDevices;
}

static void gviPS2HeadsetFreeDevice(GVIDevice * device)
{
	// delete it from the array
	// it will clear out internal data in the array's free function
	gviDeleteDeviceFromList(GVIDevices, device);
}

static GVBool gviStartPlaybackDevice(GVIPS2HeadsetData * data)
{
	int result;

	// start the buffer
	result = lgAudStartPlayback(data->m_handle);
	if(LGAUD_FAILED(result))
		return GVFalse;

	// clear the clock
	data->m_playbackClock = 0;

	// started playing
	data->m_playing = GVTrue;

	return GVTrue;
}

static GVBool gviStartCaptureDevice(GVIPS2HeadsetData * data)
{
	int result;

	// start the buffer
	result = lgAudStartRecording(data->m_handle);
	if(LGAUD_FAILED(result))
		return GVFalse;

	// no data in the capture buffer
	data->m_captureBufferBytes = 0;

	// started capturing
	data->m_capturing = GVTrue;

	return GVTrue;
}

static GVBool gviPS2HeadsetStartDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	if(type == GV_PLAYBACK)
		return gviStartPlaybackDevice(data);
	if(type == GV_CAPTURE)
		return gviStartCaptureDevice(data);
	if(type == GV_CAPTURE_AND_PLAYBACK)
	{
		if(!gviStartPlaybackDevice(data))
			return GVFalse;
		if(!gviStartCaptureDevice(data))
		{
			device->m_methods.m_stopDevice(device, GV_PLAYBACK);
			return GVFalse;
		}
		return GVTrue;
	}
	return GVFalse;
}

static void gviPS2HeadsetStopDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	if(type & GV_PLAYBACK)
	{
		// stop the playback buffer
		lgAudStopPlayback(data->m_handle);

		// stopped playing
		data->m_playing = GVFalse;

		// clear any pending sources & buffers
		gviClearSourceList(data->m_playbackSources);
	}
	if(type & GV_CAPTURE)
	{
		// stop the capture buffer
		lgAudStopRecording(data->m_handle);

		// stopped capturing
		data->m_capturing = GVFalse;

		// so a stop then start isn't continuous
		data->m_captureClock++;
	}
}

static GVBool gviPS2HeadsetIsDeviceStarted(GVIDevice * device, GVDeviceType type)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	if(type == GV_PLAYBACK)
		return data->m_playing;
	if(type == GV_CAPTURE)
		return data->m_capturing;
	if(type == GV_CAPTURE_AND_PLAYBACK)
		return (data->m_playing && data->m_capturing);
	return GVFalse;
}

static void gviPS2HeadsetSetDeviceVolume(GVIDevice * device, GVDeviceType type, GVScalar volume)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	if(type & GV_PLAYBACK)
	{
		lgAudSetPlaybackVolume(data->m_handle, LGAUD_CH_BOTH, (u_char)(volume * 100));
		data->m_playbackVolume = volume;
	}
	if(type & GV_CAPTURE)
	{
		lgAudSetRecordingVolume(data->m_handle, LGAUD_CH_BOTH, (u_char)(volume * 100));
		data->m_captureVolume = volume;
	}
}

static GVScalar gviPS2HeadsetGetDeviceVolume(GVIDevice * device, GVDeviceType type)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	if(type & GV_PLAYBACK)
		return data->m_playbackVolume;
	return data->m_captureVolume;
}

static void gviPS2HeadsetSetCaptureThreshold(GVIDevice * device, GVScalar threshold)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;
	data->m_captureThreshold = threshold;
}

static GVScalar gviPS2HeadsetGetCaptureThreshold(GVIDevice * device)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;
	return data->m_captureThreshold;
}

static int gviPS2HeadsetGetAvailableCaptureBytes(GVDevice device)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;
	int result;
	int newBytes;
	int numFrames;

	// don't do anything if we're not capturing
	if(!data->m_capturing)
		return 0;

	// get the number of new bytes
	result = lgAudGetAvailableRecordingBytes(data->m_handle, &newBytes);
	if(LGAUD_FAILED(result))
	{
		gviDeviceUnplugged(device);
		return 0;
	}

	// figure out how many frames this is
	numFrames = (newBytes / GVIBytesPerFrame);

	// calculate how many bytes this is once encoded
	newBytes = (numFrames * GVIEncodedFrameSize);

	return newBytes;
}

static void gviProcessCapturedFrame(GVDevice device, GVByte * frameOut, GVSample * frameIn, GVScalar * volume, GVBool * threshold)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;
	GVScalar frameVolume;

	// get the volume if requested
	if(volume)
	{
		frameVolume = gviGetSamplesVolume(frameIn, GVISamplesPerFrame);
		if(frameVolume > *volume)
			*volume = frameVolume;
	}

	// check against the threshold
	if(threshold && !*threshold)
	{
		if(volume)
		{
			// we already got the volume, so use that to check
			*threshold = (*volume >= data->m_captureThreshold);
		}
		else
		{
			// we didn't get a volume, so check the samples directly
			*threshold = gviIsOverThreshold(frameIn, GVISamplesPerFrame, data->m_captureThreshold);
		}
	}

	// filter
	if(device->m_captureFilterCallback)
		device->m_captureFilterCallback(device, frameIn, data->m_captureClock);

	// increment the capture clock
	data->m_captureClock++;

	// encode the buffer into the packet
	gviEncode(frameOut, frameIn);
}

static GVBool gviPS2HeadsetCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;
	GVBool overThreshold;
	int result;
	int numFrames;
	int readSize;
	int lenAvailable;
	int framesAvailable;
	GVSample * frameIn;
	GVByte * frameOut;

	// figure out how many encoded bytes they can handle
	lenAvailable = *len;

	// clear the len and volume
	*len = 0;
	if(volume)
		*volume = 0;

	// don't do anything if we're not capturing
	if(!data->m_capturing)
		return GVFalse;

	// set the frameStamp
	*frameStamp = data->m_captureClock;

	// figure out how many frames they can handle
	framesAvailable = (lenAvailable / GVIEncodedFrameSize);

	// handle the data one frame at a time
	overThreshold = GVFalse;
	frameIn = data->m_captureBuffer;
	frameOut = packet;
	for(numFrames = 0 ; numFrames < framesAvailable ; numFrames++)
	{
		// read this frame
		readSize = (GVIBytesPerFrame - data->m_captureBufferBytes);
		result = lgAudRead(data->m_handle, LGAUD_BLOCKMODE_NOT_BLOCKING, ((u_char *)data->m_captureBuffer) + data->m_captureBufferBytes, &readSize);
		if(LGAUD_FAILED(result))
		{
			gviDeviceUnplugged(device);
			return GVFalse;
		}

		// check if we have a full frame
		if(readSize != (GVIBytesPerFrame - data->m_captureBufferBytes))
		{
			// add to our count of how many bytes we have
			data->m_captureBufferBytes += readSize;
			break;
		}

		// process the frame
		gviProcessCapturedFrame(device, frameOut, frameIn, volume, &overThreshold);

		// we got a full frame, so there's no leftover
		data->m_captureBufferBytes = 0;

		// update the frame pointer
		frameOut += GVIEncodedFrameSize;
	}

	// check if this packet crossed the threshold
	if(overThreshold)
	{
		// store the time we crossed it
		data->m_captureLastCrossedThresholdTime = data->m_captureClock;
	}
	else
	{
		// check if we are still on the overhang from a previous crossing
		overThreshold = ((GVFrameStamp)(*frameStamp - data->m_captureLastCrossedThresholdTime) < GVI_HOLD_THRESHOLD_FRAMES);
	}

	// set the len
	*len = (numFrames * GVIEncodedFrameSize);

	// return false if we didn't get a packet
	if(!overThreshold || (*len == 0))
		return GVFalse;

	return GVTrue;
}

static void gviPS2HeadsetPlayPacket(GVIDevice * device, const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return;

	//add it
	gviAddPacketToSourceList(data->m_playbackSources, packet, len, source, frameStamp, mute, data->m_playbackClock);
}

static GVBool gviPS2HeadsetIsSourceTalking(GVDevice device, GVSource source)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviIsSourceTalking(data->m_playbackSources, source);
}

static int gviPS2HeadsetListTalkingSources(GVDevice device, GVSource sources[], int maxSources)
{
	GVIPS2HeadsetData * data = (GVIPS2HeadsetData *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviListTalkingSources(data->m_playbackSources, sources, maxSources);
}

static GVBool gviPS2HeadsetInitDevice(GVIDevice * device, int deviceIndex, GVDeviceType type)
{
	GVIPS2HeadsetData * data;
	lgAudOpenParam openParam;
	int result;

	// setup the open param
	openParam.Mode = 0;
	if(type & GV_CAPTURE)
	{
		openParam.Mode |= LGAUD_MODE_RECORDING;
		openParam.RecordingFormat.Channels = 1;
		openParam.RecordingFormat.BitResolution = 16;
		openParam.RecordingFormat.SamplingRate = GV_SAMPLES_PER_SECOND;
		openParam.RecordingFormat.BufferMilliseconds = GVI_CAPTURE_BUFFER_MILLISECONDS;
	}
	if(type & GV_PLAYBACK)
	{
		openParam.Mode |= LGAUD_MODE_PLAYBACK;
		openParam.PlaybackFormat.Channels = 1;
		openParam.PlaybackFormat.BitResolution = 16;
		openParam.PlaybackFormat.SamplingRate = GV_SAMPLES_PER_SECOND;
		openParam.PlaybackFormat.BufferMilliseconds = GVI_PLAYBACK_BUFFER_MILLISECONDS;
	}

	// get a pointer to the data
	data = (GVIPS2HeadsetData *)device->m_data;

	// open the device
	result = lgAudOpen(deviceIndex, &openParam, &data->m_handle);
	if(LGAUD_FAILED(result))
		return GVFalse;

	// handle playback specific stuff
	if(type & GV_PLAYBACK)
	{
		// create the array of sources
		data->m_playbackSources = gviNewSourceList();
		if(!data->m_playbackSources)
		{
			lgAudClose(data->m_handle);
			return GVFalse;
		}

		// allocate the buffer
		data->m_playbackBuffer = (GVSample *)gsimalloc((unsigned int)GVIBytesPerFrame);
		if(!data->m_playbackBuffer)
		{
			gviFreeSourceList(data->m_playbackSources);
			lgAudClose(data->m_handle);
			return GVFalse;
		}

		// set data
		//data->m_playbackVolume = gviGetMixerVolume(data, GV_PLAYBACK);
		data->m_playbackVolume = 1.0;
	}

	// handle capture specific stuff
	if(type & GV_CAPTURE)
	{
		// set some data vars
		data->m_captureVolume = 1.0;
		data->m_captureLastCrossedThresholdTime = (GVFrameStamp)(data->m_captureClock - GVI_HOLD_THRESHOLD_FRAMES - 1);

		// allocate the buffer
		data->m_captureBuffer = (GVSample *)gsimalloc((unsigned int)GVIBytesPerFrame);
		if(!data->m_captureBuffer)
		{
			if(type & GV_PLAYBACK)
			{
				gviFreeSourceList(data->m_playbackSources);
				gsifree(data->m_playbackBuffer);
			}
			lgAudClose(data->m_handle);
			return GVFalse;
		}
	}

	return GVTrue;
}

GVDevice gviPS2HeadsetNewDevice(GVDeviceID deviceID, GVDeviceType type)
{
	GVIDevice * device;
	GVBool result;

	// check for no type
	if(!(type & GV_CAPTURE_AND_PLAYBACK))
		return NULL;

	// check if the device already exists
	if(gviFindDeviceByID(deviceID))
		return NULL;

	// create a new device
	device = gviNewDevice(deviceID, GVHardwarePS2Headset, type, sizeof(GVIPS2HeadsetData));
	if(!device)
		return NULL;

	// init the device
	result = gviPS2HeadsetInitDevice(device, deviceID, type);
	if(result == GVFalse)
	{
		gviFreeDevice(device);
		return NULL;
	}

	// store the pointers
	device->m_methods.m_freeDevice = gviPS2HeadsetFreeDevice;
	device->m_methods.m_startDevice = gviPS2HeadsetStartDevice;
	device->m_methods.m_stopDevice = gviPS2HeadsetStopDevice;
	device->m_methods.m_isDeviceStarted = gviPS2HeadsetIsDeviceStarted;
	device->m_methods.m_setDeviceVolume = gviPS2HeadsetSetDeviceVolume;
	device->m_methods.m_getDeviceVolume = gviPS2HeadsetGetDeviceVolume;
	device->m_methods.m_setCaptureThreshold = gviPS2HeadsetSetCaptureThreshold;
	device->m_methods.m_getCaptureThreshold = gviPS2HeadsetGetCaptureThreshold;
	device->m_methods.m_getAvailableCaptureBytes = gviPS2HeadsetGetAvailableCaptureBytes;
	device->m_methods.m_capturePacket = gviPS2HeadsetCapturePacket;
	device->m_methods.m_playPacket = gviPS2HeadsetPlayPacket;
	device->m_methods.m_isSourceTalking = gviPS2HeadsetIsSourceTalking;
	device->m_methods.m_listTalkingSources = gviPS2HeadsetListTalkingSources;

	// add it to the list
	gviAppendDeviceToList(GVIDevices, device);

	return device;
}

#endif //!defined(GV_NO_PS2_HEADSET)
