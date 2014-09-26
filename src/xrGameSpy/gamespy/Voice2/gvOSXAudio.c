#include "gvOSXAudio.h"
#include "gvDevice.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvUtil.h"
#include <AudioToolbox/AudioConverter.h>
#include <CoreAudio/CoreAudio.h>
#include <pthread.h>

#if !defined(_MACOSX)
#error This file should only be used with MacOS X
#endif

#if !defined(GVI_VOLUME_IN_SOFTWARE)
	#define GVI_VOLUME_IN_SOFTWARE 1
#endif

/**********
** TYPES **
**********/
// when allocated, enough memory is allocated to fit an entire
// captured frame into the m_frame array
typedef struct GVICapturedFrame
{
	GVFrameStamp m_frameStamp;
	struct GVICapturedFrame * m_next;
	// m_frame must be the last member of this struct
	GVSample m_frame[1];
} GVICapturedFrame;

typedef struct
{
	GVBool m_playing;
	AudioConverterRef m_playbackConverter;
	AudioStreamBasicDescription m_playbackStreamDescriptor;
	GVFrameStamp m_playbackClock;
	GVSample * m_playbackBuffer;
	GVISourceList m_playbackSources;
	int m_playbackChannel;
#if GVI_VOLUME_IN_SOFTWARE
	GVScalar m_playbackVolume;
#endif

	GVBool m_capturing;
	AudioConverterRef m_captureConverter;
	AudioStreamBasicDescription m_captureStreamDescriptor;
	GVFrameStamp m_captureClock;
	GVSample * m_captureBuffer;
	GVICapturedFrame m_capturedFrames;
	GVICapturedFrame m_captureAvailableFrames;
	GVScalar m_captureThreshold;
	GVFrameStamp m_captureLastCrossedThresholdTime;
	int m_captureChannel;
#if GVI_VOLUME_IN_SOFTWARE
	GVScalar m_captureVolume;
#endif

	pthread_mutex_t m_mutex;
	GVBool m_unplugged;
} GVIHardwareData;

typedef struct
{
	GVIDevice * m_device;
	AudioBufferList * m_capturedAudio;
	GVBool m_used;
} GVICaptureConverterData;

/************
** GLOBALS **
************/
static GVIDeviceList GVIDevices;
static AudioStreamBasicDescription GVIVoiceFormat;

/**************
** FUNCTIONS **
**************/
static void gviFreeCapturedFrames(GVICapturedFrame * frameHead)
{
	GVICapturedFrame * frame;
	while(frameHead->m_next)
	{
		frame = frameHead->m_next;
		frameHead->m_next = frame->m_next;
		gsifree(frame);
	}
}

static GVICapturedFrame * gviPopFirstFrame(GVICapturedFrame * frameHead)
{
	GVICapturedFrame * frame = frameHead->m_next;
	if(frame)
	{
		frameHead->m_next = frame->m_next;
		frame->m_next = NULL;
	}
	return frame;
}

static void gviPushFirstFrame(GVICapturedFrame * frameHead, GVICapturedFrame * frame)
{
	frame->m_next = frameHead->m_next;
	frameHead->m_next = frame;
}

static void gviPushLastFrame(GVICapturedFrame * frameHead, GVICapturedFrame * frame)
{
	GVICapturedFrame * lastFrame;

	// find the last frame in the list
	lastFrame = frameHead;
	while(lastFrame->m_next)
		lastFrame = lastFrame->m_next;

	// add this frame to the end of the capture list
	lastFrame->m_next = frame;
}

static GVBool gviLockDevice(GVIHardwareData * data)
{
	int rcode;
	rcode = pthread_mutex_lock(&data->m_mutex);
	return (rcode == 0)?GVTrue:GVFalse;
}

static void gviUnlockDevice(GVIHardwareData * data)
{
	pthread_mutex_unlock(&data->m_mutex);
}

static void gviCleanupPlayback(GVIHardwareData * data)
{
	if(data->m_playbackSources)
		gviFreeSourceList(data->m_playbackSources);
	if(data->m_playbackConverter)
		AudioConverterDispose(data->m_playbackConverter);
	gsifree(data->m_playbackBuffer);
}

static void gviCleanupCapture(GVIHardwareData * data)
{
	if(data->m_captureConverter)
		AudioConverterDispose(data->m_captureConverter);
	gviFreeCapturedFrames(&data->m_captureAvailableFrames);
	gviFreeCapturedFrames(&data->m_capturedFrames);
	gsifree(data->m_captureBuffer);
}

static void gviFreeArrayDevice(void * elem)
{
	GVIDevice * device = *(GVIDevice **)elem;
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// stop the device
	device->m_methods.m_stopDevice(device, device->m_types);

	// free the mutex
	pthread_mutex_destroy(&data->m_mutex);

	// playback specific cleanup
	if(device->m_types & GV_PLAYBACK)
	{
		gviCleanupPlayback(data);
	}

	// capture specific cleanup
	if(device->m_types & GV_CAPTURE)
	{
		gviCleanupCapture(data);
	}

	gviFreeDevice(device);
}

GVBool gviHardwareStartup(void)
{
	// create the devices array
	GVIDevices = gviNewDeviceList(gviFreeArrayDevice);
	if(!GVIDevices)
		return GVFalse;

	// setup the format descriptor for the GV format
	memset(&GVIVoiceFormat, 0, sizeof(AudioStreamBasicDescription));
	GVIVoiceFormat.mSampleRate = (Float64)GV_SAMPLES_PER_SECOND;
	GVIVoiceFormat.mFormatID = kAudioFormatLinearPCM;
	GVIVoiceFormat.mFormatFlags = kAudioFormatFlagsNativeEndian|kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;
	GVIVoiceFormat.mBytesPerPacket = GV_BYTES_PER_SAMPLE;
	GVIVoiceFormat.mFramesPerPacket = 1;
	GVIVoiceFormat.mBytesPerFrame = GV_BYTES_PER_SAMPLE;
	GVIVoiceFormat.mChannelsPerFrame = 1;
	GVIVoiceFormat.mBitsPerChannel = GV_BITS_PER_SAMPLE;

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
}

void gviHardwareThink(void)
{
	GVIDevice * device;
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

		// has it been unplugged?
		if(((GVIHardwareData*)device->m_data)->m_unplugged)
		{
			gviDeviceUnplugged(device);
		}
	}
}

static GVBool gviCFStringToString(CFStringRef ref, gsi_char str[], int strLen)
{
#if !defined(GSI_UNICODE)

	Boolean result = CFStringGetCString(ref, str, strLen, kCFStringEncodingASCII);
	return (result == true)?GVTrue:GVFalse;

#else
	
	CFRange range;

	range.location = 0;
	range.length = CFStringGetLength(ref);
	range.length = min(range.length, strLen - 1);

	CFStringGetCharacters(ref, range, str);
	str[range.length] = '\0';

	return GVTrue;

#endif
}

static int gviCountChannels(AudioDeviceID deviceID, bool input)
{
	OSStatus result;
	UInt32 size;
	AudioBufferList * list;
	int numChannels;
	int i;

	// get the size of the buffer list
	result = AudioDeviceGetPropertyInfo(deviceID, 0, input, kAudioDevicePropertyStreamConfiguration, &size, NULL);
	if(result != noErr)
		return 0;

	// allocate the buffer list
	list = (AudioBufferList *)gsimalloc(size);
	if(list == NULL)
		return 0;

	// fill the buffer list
	result = AudioDeviceGetProperty(deviceID, 0, input, kAudioDevicePropertyStreamConfiguration, &size, list);
	if(result != noErr)
	{
		gsifree(list);
		return 0;
	}

	// count the number of channels
	numChannels = 0;
	for(i = 0 ; i < list->mNumberBuffers ; i++)
		numChannels += list->mBuffers[i].mNumberChannels;

	// free the list
	gsifree(list);

	return numChannels;
}

static GVBool gviFillDeviceInfo(AudioDeviceID deviceID, GVDeviceInfo * device, GVDeviceType types)
{
	OSStatus result;
	UInt32 size;
	UInt32 isAlive;
	GVDeviceType supportedTypes;
	CFStringRef nameRef;

	// make sure the device is alive
	size = sizeof(UInt32);
	result = AudioDeviceGetProperty(deviceID, 0, true, kAudioDevicePropertyDeviceIsAlive, &size, &isAlive);
	if((result != noErr) || (isAlive != 1))
		return GVFalse;

	// set the hardware type
	device->m_hardwareType = GVHardwareMacOSX;

	// store the id
	device->m_id = deviceID;

	// figure out what types it supports
	supportedTypes = (GVDeviceType)0;
	if(gviCountChannels(deviceID, true) > 0)
		supportedTypes |= GV_CAPTURE;
	if(gviCountChannels(deviceID, false) > 0)
		supportedTypes |= GV_PLAYBACK;

	// if there's nothing in common, return
	if(!(supportedTypes & types))
		return GVFalse;

	// store the supported types
	device->m_deviceType = supportedTypes;

	// get the name
	size = sizeof(nameRef);
	result = AudioDeviceGetProperty(deviceID, 0, (supportedTypes & GV_CAPTURE)?true:false, kAudioDevicePropertyDeviceNameCFString, &size, &nameRef);
	if(result != noErr)
		return GVFalse;

	// if there's a blank name, we'll supply our own
	if(CFStringGetLength(nameRef) <= 0)
	{
		CFRelease(nameRef);
		nameRef = NULL;
	}

	// if there's no name, give a default
	if(nameRef == NULL)
	{
		if(supportedTypes == GV_CAPTURE)
			nameRef = CFSTR("Capture Device");
		else if(supportedTypes == GV_PLAYBACK)
			nameRef = CFSTR("Playback Device");
		else
			nameRef = CFSTR("Capture & Playback Device");
	}

	// convert it to an array of gsi_char
	gviCFStringToString(nameRef, device->m_name, GV_DEVICE_NAME_LEN);

	// release the CFString
	CFRelease(nameRef);

	return GVTrue;
}

int gviHardwareListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
	OSStatus result;
	UInt32 size;
	int numDevices;
	int deviceCount;
	AudioDeviceID * deviceIDs;
	AudioDeviceID defaultCaptureDeviceID;
	AudioDeviceID defaultPlaybackDeviceID;
	GVDeviceType defaultTypes;
	int i;

	// get the default device ids
	size = sizeof(AudioDeviceID);
	result = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice, &size, &defaultCaptureDeviceID);
	if(result != noErr)
		defaultCaptureDeviceID = kAudioDeviceUnknown;
	result = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &defaultPlaybackDeviceID);
	if(result != noErr)
		defaultPlaybackDeviceID = kAudioDeviceUnknown;

	// get the size of the device ids array
	result = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
	if(result != noErr)
		return 0;

	// calc the number of devices
	numDevices = (size / sizeof(AudioDeviceID));

	// allocate the array of device ids
	deviceIDs = (AudioDeviceID *)gsimalloc(size);
	if(deviceIDs == NULL)
		return 0;

	// get the device ids
	result = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, deviceIDs);
	if(result != noErr)
	{
		gsifree(deviceIDs);
		return 0;
	}

	// fill the array of devices with info
	deviceCount = 0;
	for(i = 0 ; (i < maxDevices) && (deviceCount < numDevices) ; i++)
	{
		// try to add the device to the list
		GVBool addedDevice = gviFillDeviceInfo(deviceIDs[i], &devices[deviceCount], types);

		// check if it was added successfully
		if(addedDevice)
		{
			// check it against the defaults
			defaultTypes = (GVDeviceType)0;
			if(deviceIDs[i] == defaultCaptureDeviceID)
				defaultTypes |= GV_CAPTURE;
			if(deviceIDs[i] == defaultPlaybackDeviceID)
				defaultTypes |= GV_PLAYBACK;
			devices[deviceCount].m_defaultDevice = defaultTypes;

			// increment the count
			deviceCount++;
		}
	}

	// free the array
	gsifree(deviceIDs);

	return deviceCount;
}

static void gviHardwareFreeDevice(GVDevice device)
{
	// delete it from the array
	// it will clear out internal data in the array's free function
	gviDeleteDeviceFromList(GVIDevices, device);
}

static OSStatus gviAudioConverterCaptureProc(AudioConverterRef inAudioConverter,
                                             UInt32 * ioNumberDataPackets,
                                             AudioBufferList * ioData,
                                             AudioStreamPacketDescription ** outDataPacketDescription,
                                             void * inUserData)
{
	GVICaptureConverterData * converterData = (GVICaptureConverterData *)inUserData;
	GVIDevice * device = converterData->m_device;
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	AudioBufferList * bufferList = converterData->m_capturedAudio;

	// check if this data has already been used
	if(converterData->m_used)
		return (OSStatus)1;

	// it only wants one buffer
	ioData->mBuffers[0].mNumberChannels = bufferList->mBuffers[data->m_captureChannel].mNumberChannels;
	ioData->mBuffers[0].mData = bufferList->mBuffers[data->m_captureChannel].mData;
	ioData->mBuffers[0].mDataByteSize = bufferList->mBuffers[data->m_captureChannel].mDataByteSize;

	// fill in the number of samples we provided
	*ioNumberDataPackets = (ioData->mBuffers[0].mDataByteSize / data->m_captureStreamDescriptor.mBytesPerPacket);

	// we've used the buffer
	converterData->m_used = GVTrue;

	return noErr;
}

static OSStatus gviHardwareCaptureIOProc(AudioDeviceID inDevice,
                                         const AudioTimeStamp * inNow,
                                         const AudioBufferList * inInputData, 
                                         const AudioTimeStamp * inInputTime,
                                         AudioBufferList * outOutputData, 
                                         const AudioTimeStamp * inOutputTime,
                                         void * inClientData)
{
	GVIDevice * device = (GVIDevice *)inClientData;
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	OSStatus result;
	UInt32 size;
	GVICaptureConverterData converterData;
	AudioBufferList bufferList;
	GVICapturedFrame * frame;

	// get a lock on the device
	if(!gviLockDevice(data))
		return (OSStatus)1;

	// make sure we are capturing
	if(data->m_capturing)
	{
		// setup the buffer list
		bufferList.mNumberBuffers = 1;
		bufferList.mBuffers[0].mNumberChannels = 1;
		bufferList.mBuffers[0].mDataByteSize = GVIBytesPerFrame;
		bufferList.mBuffers[0].mData = data->m_captureBuffer;

		// setup the converter data struct
		converterData.m_device = device;
		converterData.m_capturedAudio = (AudioBufferList *)inInputData;
		converterData.m_used = GVFalse;

		// loop while it is converting data
		do
		{
			// request one frame
			size = GVISamplesPerFrame;

			// convert the captured data into our format
			result = AudioConverterFillComplexBuffer(data->m_captureConverter, gviAudioConverterCaptureProc, &converterData, &size, &bufferList, NULL);

			// was there enough to fill a buffer
			if(result == noErr)
			{
				// get a frame
				frame = gviPopFirstFrame(&data->m_captureAvailableFrames);

				// if there aren't any available frames, repurpose the oldest captured frame
				if(!frame)
				{
					frame = gviPopFirstFrame(&data->m_capturedFrames);
					assert(frame);
					if(!frame)
						break;
				}

				// setup the frame
				memcpy(frame->m_frame, data->m_captureBuffer, GVIBytesPerFrame);
				frame->m_frameStamp = data->m_captureClock;

				// increment the capture clock
				data->m_captureClock++;

				// add this frame to the end of the capture list
				gviPushLastFrame(&data->m_capturedFrames, frame);
			}
		}
		while(result == noErr);
	}

	// release the device lock
	gviUnlockDevice(data);

	return noErr;
}

static GVBool gviHardwareStartCapture(GVDevice device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	OSStatus result;

	// now capturing
	data->m_capturing = GVTrue;

	// add the IO proc
	result = AudioDeviceAddIOProc((AudioDeviceID)device->m_deviceID, gviHardwareCaptureIOProc, device);
	if(result != noErr)
	{
		data->m_capturing = GVFalse;
		return GVFalse;
	}

	// start it
	result = AudioDeviceStart((AudioDeviceID)device->m_deviceID, gviHardwareCaptureIOProc);
	if(result != noErr)
	{
		data->m_capturing = GVFalse;
		return GVFalse;
	}

	return GVTrue;
}

static OSStatus gviAudioConverterPlaybackProc(AudioConverterRef inAudioConverter,
                                              UInt32 * ioNumberDataPackets,
                                              AudioBufferList * ioData,
                                              AudioStreamPacketDescription ** outDataPacketDescription,
                                              void * inUserData)
{
	GVIDevice * device = (GVIDevice *)inUserData;
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVBool wroteToBuffer = GVFalse;

	// make sure we are playing
	if(data->m_playing)
	{
		// write sources to the playback buffer
		wroteToBuffer = gviWriteSourcesToBuffer(data->m_playbackSources, data->m_playbackClock, data->m_playbackBuffer, 1);
	}

	// clear it if nothing was written
	if(!data->m_playing || !wroteToBuffer)
	{
		memset(data->m_playbackBuffer, 0, (size_t)GVIBytesPerFrame);
	}
	// check if we need to adjust the volume
	else if(data->m_playbackVolume < 1.0)
	{
		int i;
		for(i = 0 ; i < GVISamplesPerFrame ; i++)
			data->m_playbackBuffer[i] = (GVSample)(data->m_playbackBuffer[i] * data->m_playbackVolume);
	}

	// filter
	if(device->m_playbackFilterCallback)
		device->m_playbackFilterCallback(device, data->m_playbackBuffer, data->m_playbackClock);

	// setup the output data
	ioData->mBuffers[0].mNumberChannels = 1;
	ioData->mBuffers[0].mDataByteSize = GVIBytesPerFrame;
	ioData->mBuffers[0].mData = (void *)data->m_playbackBuffer;

	// we wrote one frame
	*ioNumberDataPackets = GVISamplesPerFrame;

	// update the clock
	data->m_playbackClock++;

	return noErr;
}

static OSStatus gviHardwarePlaybackIOProc(AudioDeviceID inDevice,
                                          const AudioTimeStamp * inNow,
                                          const AudioBufferList * inInputData, 
                                          const AudioTimeStamp * inInputTime,
                                          AudioBufferList * outOutputData, 
                                          const AudioTimeStamp * inOutputTime,
                                          void * inClientData)
{
	GVIDevice * device = (GVIDevice *)inClientData;
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	AudioBufferList bufferList;
	OSStatus result;
	UInt32 size;

	// get a lock on the device
	if(!gviLockDevice(data))
		return (OSStatus)1;

	// calculate the number of samples the proc wants
	size = (outOutputData->mBuffers[0].mDataByteSize / data->m_playbackStreamDescriptor.mBytesPerFrame);

	// setup our own buffer list, pointing at the channel (buffer) we want
	bufferList.mNumberBuffers = 1;
	bufferList.mBuffers[0] = outOutputData->mBuffers[data->m_playbackChannel];

	// fill the buffer using the callback
	result = AudioConverterFillComplexBuffer(data->m_playbackConverter, gviAudioConverterPlaybackProc, device, &size, &bufferList, NULL);
	if(result != noErr)
		return (OSStatus)1;

	// release the device lock
	gviUnlockDevice(data);

	return noErr;
}

static GVBool gviHardwareStartPlayback(GVDevice device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	OSStatus result;

	// now playing
	data->m_playing = GVTrue;

	// reset the playback clock
	data->m_playbackClock = 0;

	// add the IO proc
	result = AudioDeviceAddIOProc((AudioDeviceID)device->m_deviceID, gviHardwarePlaybackIOProc, device);
	if(result != noErr)
	{
		data->m_playing = GVFalse;
		return GVFalse;
	}

	// start it
	result = AudioDeviceStart((AudioDeviceID)device->m_deviceID, gviHardwarePlaybackIOProc);
	if(result != noErr)
	{
		data->m_playing = GVFalse;
		return GVFalse;
	}

	return GVTrue;
}

static GVBool gviHardwareStartDevice(GVDevice device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVBool result;

	// get a lock on the device
	if(!gviLockDevice(data))
		return GVFalse;

	// capture
	if((type & GV_CAPTURE) && !data->m_capturing)
	{
		result = gviHardwareStartCapture(device);
		if(!result)
		{
			gviUnlockDevice(data);
			return GVFalse;
		}
	}

	// playback
	if((type & GV_PLAYBACK) && !data->m_playing)
	{
		result = gviHardwareStartPlayback(device);
		if(!result)
		{
			if(type & GV_CAPTURE)
				device->m_methods.m_stopDevice(device, GV_CAPTURE);
			gviUnlockDevice(data);
			return GVFalse;
		}
	}

	// release the device lock
	gviUnlockDevice(data);

	return GVTrue;
}

static void gviHardwareStopDevice(GVDevice device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVICapturedFrame * frame;

	// lock the device
	gviLockDevice(data);

	// capture
	if((type & GV_CAPTURE) && data->m_capturing)
	{
		// unlock so the ioproc can stop
		gviUnlockDevice(data);

		// stop io
		AudioDeviceStop((AudioDeviceID)device->m_deviceID, gviHardwareCaptureIOProc);

		// relock
		gviLockDevice(data);

		// remove the IO proc
		AudioDeviceRemoveIOProc((AudioDeviceID)device->m_deviceID, gviHardwareCaptureIOProc);

		// move captured frames back to the available frames list
		while((frame = gviPopFirstFrame(&data->m_capturedFrames)) != NULL)
			gviPushFirstFrame(&data->m_captureAvailableFrames, frame);

		// no longer capturing
		data->m_capturing = GVFalse;
	}

	// playback
	if((type & GV_PLAYBACK) && data->m_playing)
	{
		// unlock so the ioproc can stop
		gviUnlockDevice(data);

		// stop io
		AudioDeviceStop((AudioDeviceID)device->m_deviceID, gviHardwarePlaybackIOProc);

		// relock
		gviLockDevice(data);

		// remove the IO proc
		AudioDeviceRemoveIOProc((AudioDeviceID)device->m_deviceID, gviHardwarePlaybackIOProc);

		// clear any pending sources & buffers
		gviClearSourceList(data->m_playbackSources);

		// no longer playing
		data->m_playing = GVFalse;
	}

	// unlock the device
	gviUnlockDevice(data);
}

static GVBool gviHardwareIsDeviceStarted(GVDevice device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVBool result = GVFalse;

	// get a lock on the device
	if(!gviLockDevice(data))
		return GVFalse;

	// figure out the result
	if(type == GV_PLAYBACK)
		result = data->m_playing;
	else if(type == GV_CAPTURE)
		result = data->m_capturing;
	else if (type == GV_CAPTURE_AND_PLAYBACK)
		result = (data->m_playing && data->m_capturing)?GVTrue:GVFalse;

	// release the device lock
	gviUnlockDevice(data);

	return result;
}

#if GVI_VOLUME_IN_SOFTWARE

	static void gviSetDeviceVolume(GVDevice device, GVBool isInput, GVScalar volume)
	{
		GVIHardwareData * data = (GVIHardwareData *)device->m_data;

		if(isInput)
			data->m_captureVolume = volume;
		else
			data->m_playbackVolume = volume;
	}

	static GVScalar gviGetDeviceVolume(GVDevice device, GVBool isInput)
	{
		GVIHardwareData * data = (GVIHardwareData *)device->m_data;

		if(isInput)
			return data->m_captureVolume;
		else
			return data->m_playbackVolume;
	}

#else

	static void gviSetDeviceVolume(GVDevice device, GVBool isInput, GVScalar volume)
	{
		Float32 vol = volume;
		int channel;

		// set the volume on all three channels
		// this covers mono and stereo devices
		for(channel = 0 ; channel < 3 ; channel++)
			AudioDeviceSetProperty(device->m_deviceID, NULL, channel, isInput, kAudioDevicePropertyVolumeScalar, sizeof(Float32), &vol);
	}

	static GVBool gviGetChannelVolume(GVDevice device, GVBool isInput, int channel, Float32 * volume)
	{
		OSStatus result;
		UInt32 size = sizeof(Float32);

		// get the volume for a specific channel
		result = AudioDeviceGetProperty(device->m_deviceID, channel, isInput, kAudioDevicePropertyVolumeScalar, &size, volume);

		return (result == noErr)?GVTrue:GVFalse;
	}

	static GVScalar gviGetDeviceVolume(GVDevice device, GVBool isInput)
	{
		GVBool result;
		Float32 channels[2];

		// check for mono
		result = gviGetChannelVolume(device, isInput, 0, &channels[0]);
		if(result)
			return (GVScalar)channels[0];

		// get left
		result = gviGetChannelVolume(device, isInput, 1, &channels[0]);
		if(!result)
			return 0;

		// get right
		result = gviGetChannelVolume(device, isInput, 2, &channels[1]);
		if(!result)
			return (GVScalar)channels[0];

		// return a mix of left and right
		return (GVScalar)((channels[0] + channels[1]) / 2);
	}

#endif

static void gviHardwareSetDeviceVolume(GVDevice device, GVDeviceType type, GVScalar volume)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// get a lock on the device
	if(!gviLockDevice(data))
		return;

	// set the volume
	if(type & GV_PLAYBACK)
		gviSetDeviceVolume(device, GVFalse, volume);
	if(type & GV_CAPTURE)
		gviSetDeviceVolume(device, GVTrue, volume);

	// release the device lock
	gviUnlockDevice(data);
}

static GVScalar gviHardwareGetDeviceVolume(GVDevice device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVScalar volume = 0;

	// get a lock on the device
	if(!gviLockDevice(data))
		return 0;

	// get the volume
	if(type & GV_PLAYBACK)
		volume = gviGetDeviceVolume(device, GVFalse);
	else if(type & GV_CAPTURE)
		volume = gviGetDeviceVolume(device, GVTrue);

	// release the device lock
	gviUnlockDevice(data);

	return volume;
}

static void gviHardwareSetCaptureThreshold(GVDevice device, GVScalar threshold)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// get a lock on the device
	if(!gviLockDevice(data))
		return;

	// store the threshold
	data->m_captureThreshold = threshold;

	// release the device lock
	gviUnlockDevice(data);
}

static GVScalar gviHardwareGetCaptureThreshold(GVDevice device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVScalar threshold;

	// get a lock on the device
	if(!gviLockDevice(data))
		return GVFalse;

	// store the threshold
	threshold = data->m_captureThreshold;
	
	// release the device lock
	gviUnlockDevice(data);
	
	return threshold;
}

static GVBool gviHardwareCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	int numFrames;
	GVICapturedFrame * frame;
	GVScalar frameVolume;
	GVBool overThreshold;

	// get a lock on the device
	if(!gviLockDevice(data))
		return GVFalse;

	// calculate the number of frames we can put in the packet
	numFrames = (*len / GVIEncodedFrameSize);

	// clear the len and volume
	*len = 0;
	if(volume)
		*volume = 0;

	// don't do anything if we're not capturing
	if(!data->m_capturing)
	{
		gviUnlockDevice(data);
		return GVFalse;
	}

	// don't do anything if there isn't room for a frame
	if(numFrames < 1)
	{
		gviUnlockDevice(data);
		return GVFalse;
	}

	do
	{
		// get a frame
		frame = gviPopFirstFrame(&data->m_capturedFrames);
		if(!frame)
		{
			gviUnlockDevice(data);
			return GVFalse;
		}

		// calculate the volume
		frameVolume = gviGetSamplesVolume(frame->m_frame, GVISamplesPerFrame);

		// check against the threshold
		overThreshold = (frameVolume >= data->m_captureThreshold);
		if(overThreshold)
		{
			// update the time at which we crossed the threshold
			data->m_captureLastCrossedThresholdTime = frame->m_frameStamp;
		}
		else
		{
			// check if we are still within the hold time
			overThreshold = ((GVFrameStamp)(frame->m_frameStamp - data->m_captureLastCrossedThresholdTime) < GVI_HOLD_THRESHOLD_FRAMES);
		}

		// check if this should be captured
		if(overThreshold)
		{
			// scale the data
			if(data->m_captureVolume < 1.0)
			{
				int j;
				for(j = 0 ; j < GVISamplesPerFrame ; j++)
					frame->m_frame[j] = (GVSample)(frame->m_frame[j] * data->m_captureVolume);
			}

			// filter
			if(device->m_captureFilterCallback)
				device->m_captureFilterCallback(device, frame->m_frame, frame->m_frameStamp);

			// encode the frame into the packet
			gviEncode(packet, frame->m_frame);
			*len = GVIEncodedFrameSize;
			*frameStamp = frame->m_frameStamp;
			if(volume)
				*volume = frameVolume;
		}

		// put the frame back in the available list
		gviPushFirstFrame(&data->m_captureAvailableFrames, frame);
	}
	while(!overThreshold);
	
	// release the device lock
	gviUnlockDevice(data);

	return GVTrue;
}

static int gviHardwareGetAvailableCaptureBytes(GVDevice device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVICapturedFrame * frame;
	int count = 0;

	// don't do anything if we're not capturing
	if(!data->m_capturing)
		return 0;

	// count the number of captured frames
	for(frame = data->m_capturedFrames.m_next ; frame ; frame = frame->m_next)
		count++;

	// return the number of bytes
	return (count * GVIEncodedFrameSize);
}

static void gviHardwarePlayPacket(GVDevice device, const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// get a lock on the device
	if(!gviLockDevice(data))
		return;

	// check if we're not playing
	if(data->m_playing)
	{
		//add it
		gviAddPacketToSourceList(data->m_playbackSources, packet, len, source, frameStamp, mute, data->m_playbackClock);
	}

	// release the device lock
	gviUnlockDevice(data);
}

static GVBool gviHardwareIsSourceTalking(GVDevice device, GVSource source)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	GVBool isTalking = GVFalse;
	
	// get a lock on the device
	if(!gviLockDevice(data))
		return GVFalse;

	// check if we're playing
	if(data->m_playing)
	{
		// check if the source is talking
		isTalking = gviIsSourceTalking(data->m_playbackSources, source);
	}
	
	// release the device lock
	gviUnlockDevice(data);
	
	return isTalking;
}

static int gviHardwareListTalkingSources(GVDevice device, GVSource sources[], int maxSources)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	int numTalking = 0;

	// get a lock on the device
	if(!gviLockDevice(data))
		return 0;

	// check if we're not playing
	if(data->m_playing)
	{
		// list them
		numTalking = gviListTalkingSources(data->m_playbackSources, sources, maxSources);
	}
	
	// release the device lock
	gviUnlockDevice(data);
	
	return numTalking;
}

static int gviHardwareGetNumChannels(GVDevice device, GVDeviceType type)
{
	OSStatus result;
	UInt32 size;
	int numStreams;

	result = AudioDeviceGetPropertyInfo(device->m_deviceID, 0, (type & GV_CAPTURE)?true:false, kAudioDevicePropertyStreams, &size, NULL);
	if(result != noErr)
		return 1;

	numStreams = (size / sizeof(AudioStreamID));

	return numStreams;
}

static void gviHardwareGetChannelName(GVDevice device, GVDeviceType type, int index, gsi_char name[GV_CHANNEL_NAME_LEN])
{
	OSStatus result;
	UInt32 size;
	CFStringRef nameRef;
	AudioStreamID * streamIDs;
	int num;

	// clear the name in case we abort
	name[0] = '\0';

	// figure out how many channels there are
	num = gviHardwareGetNumChannels(device, type);

	// make sure the index we have is valid
	if((index < 0) || (index >= num))
		return;

	// allocate memory for the stream IDs
	size = (num * sizeof(AudioStreamID));
	streamIDs = (AudioStreamID*)gsimalloc(size);

	// get the stream IDs
	result = AudioDeviceGetProperty(device->m_deviceID, 0, (type & GV_CAPTURE)?true:false, kAudioDevicePropertyStreams, &size, streamIDs);
	if(result == noErr)
	{
		// get the CFString for this stream
		size = sizeof(CFStringRef);
		result = AudioStreamGetProperty(streamIDs[index], 0, kAudioDevicePropertyDeviceNameCFString, &size, &nameRef);
		if(result != noErr)
			nameRef = NULL;
	}

	// if there's a blank name, we'll supply our own
	if(nameRef && (CFStringGetLength(nameRef) <= 0))
	{
		CFRelease(nameRef);
		nameRef = NULL;
	}

	// if there's no name, give a default
	if(nameRef == NULL)
	{
		CFStringRef format;

		if(type & GV_CAPTURE)
			format = CFSTR("Input Channel %d");
		else
			format = CFSTR("Output Channel %d");

		nameRef = CFStringCreateWithFormat(NULL, NULL, format, index);

		CFRelease(format);
	}

	// convert it to an array of gsi_char
	gviCFStringToString(nameRef, name, GV_CHANNEL_NAME_LEN);

	// release the CFString
	CFRelease(nameRef);

	// free the streamIDs
	gsifree(streamIDs);
}

static void gviHardwareSetChannel(GVDevice device, GVDeviceType type, int index)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	int num;
	
	num = gviHardwareGetNumChannels(device, type);

	if((index < 0) || (index >= num))
		return;

	if(type & GV_CAPTURE)
		data->m_captureChannel = index;
	else
		data->m_playbackChannel = index;
}

static int gviHardwareGetChannel(GVDevice device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type & GV_CAPTURE)
		return data->m_captureChannel;
	else
		return data->m_playbackChannel;
}

OSStatus gviPropertyListener(AudioDeviceID           inDevice,
                             UInt32                  inChannel,
                             Boolean                 isInput,
                             AudioDevicePropertyID   inPropertyID,
                             void*                   inClientData)
{
	GVIDevice * device = (GVIDevice*)inClientData;
	GVIHardwareData * data = (GVIHardwareData*)device->m_data;
	UInt32 value;
	UInt32 len = sizeof(UInt32);
	OSStatus result;

	result = AudioDeviceGetProperty(device->m_deviceID, 0, isInput, kAudioDevicePropertyDeviceIsAlive, &len, &value);
	if((result == noErr) && (value == 0))
	{
		// the device has been unplugged
		gviLockDevice(device->m_data);
		data->m_unplugged = GVTrue;
		gviUnlockDevice(device->m_data);
	}

	return (OSStatus)0;
}

static GVBool gviHardwareInitCapture(GVIDevice * device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	UInt32 size;
	OSStatus result;
	GVICapturedFrame * frame;
	int numCaptureBufferBytes;
	int numCaptureBufferFrames;
	int i;

	// get the capture format
	size = sizeof(AudioStreamBasicDescription);
	result = AudioDeviceGetProperty(device->m_deviceID, 0, true, kAudioDevicePropertyStreamFormat, &size, &data->m_captureStreamDescriptor);
	if(result != noErr)
		return GVFalse;

	// create a converter from the capture format to the GV format
	result = AudioConverterNew(&data->m_captureStreamDescriptor, &GVIVoiceFormat, &data->m_captureConverter);
	if(result != noErr)
		return GVFalse;

	// allocate a capture buffer
	data->m_captureBuffer = (GVSample *)gsimalloc(GVIBytesPerFrame);
	if(!data->m_captureBuffer)
	{
		AudioConverterDispose(data->m_captureConverter);
		return GVFalse;		
	}

	// allocate space for holding captured frames
	numCaptureBufferBytes = gviMultiplyByBytesPerMillisecond(GVI_CAPTURE_BUFFER_MILLISECONDS);
	numCaptureBufferBytes = gviRoundUpToNearestMultiple(numCaptureBufferBytes, GVIBytesPerFrame);
	numCaptureBufferFrames = (numCaptureBufferBytes / GVIBytesPerFrame);
	for(i = 0 ; i < numCaptureBufferFrames ; i++)
	{
		frame = (GVICapturedFrame *)gsimalloc(sizeof(GVICapturedFrame) + GVIBytesPerFrame - sizeof(GVSample));
		if(!frame)
		{
			gviFreeCapturedFrames(&data->m_captureAvailableFrames);
			gsifree(data->m_captureBuffer);
			AudioConverterDispose(data->m_captureConverter);
			return GVFalse;
		}
		gviPushFirstFrame(&data->m_captureAvailableFrames, frame);
	}

	// init the last crossed time
	data->m_captureLastCrossedThresholdTime = (data->m_captureClock - GVI_HOLD_THRESHOLD_FRAMES - 1);

	// add property listener
	AudioDeviceAddPropertyListener(device->m_deviceID, 0, true, kAudioDevicePropertyDeviceIsAlive, gviPropertyListener, device);

#if GVI_VOLUME_IN_SOFTWARE
	// init volume
	data->m_captureVolume = (GVScalar)1.0;
#endif

	return GVTrue;
}

static GVBool gviHardwareInitPlayback(GVIDevice * device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	UInt32 size;
	OSStatus result;
	UInt32 primeMethod;
	SInt32 channelMap[100];
	int i;

	// create the array of sources
	data->m_playbackSources = gviNewSourceList();
	if(!data->m_playbackSources)
		return GVFalse;

	// get the playback format
	size = sizeof(AudioStreamBasicDescription);
	result = AudioDeviceGetProperty(device->m_deviceID, 0, false, kAudioDevicePropertyStreamFormat, &size, &data->m_playbackStreamDescriptor);
	if(result != noErr)
	{
		gviFreeSourceList(data->m_playbackSources);
		return GVFalse;
	}

	// create a converter from the GV format to the playback format
	result = AudioConverterNew(&GVIVoiceFormat, &data->m_playbackStreamDescriptor, &data->m_playbackConverter);
	if(result != noErr)
	{
		gviFreeSourceList(data->m_playbackSources);
		return GVFalse;
	}

	// set it to do no priming
	primeMethod = kConverterPrimeMethod_None;
	result = AudioConverterSetProperty(data->m_playbackConverter, kAudioConverterPrimeMethod, sizeof(UInt32), &primeMethod);
	if(result != noErr)
	{
		AudioConverterDispose(data->m_playbackConverter);
		gviFreeSourceList(data->m_playbackSources);
		return GVFalse;
	}

	// setup the converter to map the input channel to all output channels
	result = AudioConverterGetPropertyInfo(data->m_playbackConverter, kAudioConverterChannelMap, &size, NULL);
	if(result == noErr)
	{
		result = AudioConverterGetProperty(data->m_playbackConverter, kAudioConverterChannelMap, &size, channelMap);
		if(result == noErr)
		{
			for(i = 0 ; i < (size / sizeof(SInt32)) ; i++)
				channelMap[i] = 0;

			AudioConverterSetProperty(data->m_playbackConverter, kAudioConverterChannelMap, size, channelMap);
		}
	}

	// allocate the playback buffer
	data->m_playbackBuffer = (GVSample *)gsimalloc(GVIBytesPerFrame);
	if(!data->m_playbackBuffer)
	{
		AudioConverterDispose(data->m_playbackConverter);
		gviFreeSourceList(data->m_playbackSources);
		return GVFalse;
	}

	// add property listener
	AudioDeviceAddPropertyListener(device->m_deviceID, 0, false, kAudioDevicePropertyDeviceIsAlive, gviPropertyListener, device);

#if GVI_VOLUME_IN_SOFTWARE
	// init volume
	data->m_playbackVolume = (GVScalar)1.0;
#endif

	return GVTrue;
}

static GVBool gviHardwareInitDevice(GVIDevice * device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	int result;

	// init the mutex
	result = pthread_mutex_init(&data->m_mutex, NULL);
	if(result != 0)
		return GVFalse;

	// handle playback specific stuff
	if(device->m_types & GV_PLAYBACK)
	{
		if(!gviHardwareInitPlayback(device))
		{
			pthread_mutex_destroy(&data->m_mutex);
			return GVFalse;
		}
	}

	// handle capture specific stuff
	if(device->m_types & GV_CAPTURE)
	{
		if(!gviHardwareInitCapture(device))
		{
			pthread_mutex_destroy(&data->m_mutex);
			gviCleanupPlayback(data);
			return GVFalse;
		}
	}

	return GVTrue;
}

GVDevice gviHardwareNewDevice(GVDeviceID deviceID, GVDeviceType type)
{
	GVIDevice * device;
	GVBool result;

	// check for no type
	if(!(type & GV_CAPTURE_AND_PLAYBACK))
		return NULL;

	// create a new device
	device = gviNewDevice(deviceID, GVHardwareMacOSX, type, sizeof(GVIHardwareData));
	if(!device)
		return NULL;

	// init the device
	result = gviHardwareInitDevice(device);
	if(result == GVFalse)
	{
		gviFreeDevice(device);
		return NULL;
	}

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
	device->m_methods.m_getNumChannels = gviHardwareGetNumChannels;
	device->m_methods.m_getChannelName = gviHardwareGetChannelName;
	device->m_methods.m_setChannel= gviHardwareSetChannel;
	device->m_methods.m_getChannel = gviHardwareGetChannel;

	// add it to the list
	gviAppendDeviceToList(GVIDevices, device);

	return device;
}
