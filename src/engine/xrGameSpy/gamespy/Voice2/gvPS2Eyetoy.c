#include "gvPS2Eyetoy.h"
#if !defined(GV_NO_PS2_EYETOY)
#include "gvDevice.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvUtil.h"
#include <liblgvid.h>

#if !defined(_PS2)
#error This file should only be used with the PlayStation2
#endif

/**********
** TYPES **
**********/
typedef struct
{
	int m_handle;

	GVBool m_capturing;
	GVFrameStamp m_captureClock;
	GVScalar m_captureThreshold;
	GVFrameStamp m_captureLastCrossedThresholdTime;
	GVSample * m_captureBuffer;
	int m_captureBufferBytes;
} GVIPS2EyetoyData;

/************
** GLOBALS **
************/
static GVIDeviceList GVIDevices;
static GVBool GVILGVidInitialized;

/**************
** FUNCTIONS **
**************/
static void gviFreeArrayDevice(void * elem)
{
	GVIDevice * device = *(GVIDevice **)elem;
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;

	// close the device
	lgVidClose(data->m_handle);

	// cleanup the buffer
	gsifree(data->m_captureBuffer);

	// free the device
	gviFreeDevice(device);
}

GVBool gviPS2EyetoyStartup(void)
{
	int result;

	// create the array of devices
	GVIDevices = gviNewDeviceList(gviFreeArrayDevice);
	if(!GVIDevices)
		return GVFalse;

	// initialize the eyetoy library
	if(GVILGVidInitialized)
	{
		result = lgVidReInit();
	}
	else
	{
		lgVidCustomAllocator anAllocator;
		anAllocator.pfnMalloc = gsimalloc;
		anAllocator.pfnMemAlign = gsimemalign;
		anAllocator.pfnFree = gsifree;
#ifdef GVI_LGVID_OLD_DRIVER
		result = lgVidInit(NULL);
#else
		result = lgVidInit(&anAllocator, NULL, 0);
#endif

	}
	if(LGVID_FAILED(result))
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
		return GVFalse;
	}

	GVILGVidInitialized = GVTrue;

	return GVTrue;
}

void gviPS2EyetoyCleanup(void)
{
	// free the device array
	if(GVIDevices)
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
	}

	// cleanup the eyetoy library
	lgVidUnloadIOPModule();
}

int gviPS2EyetoyListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
	lgVidDeviceDesc eyetoyDesc;
	GVDeviceType supportedTypes;
	int result;
	int index;
	int numDevices = 0;
	int modeIndex;

	if(!(types & GV_CAPTURE))
		return 0;

	// find all the eyetoy devices
enumerate:
	for(index = 0 ; numDevices < maxDevices ; index++)
	{
		// get the indexed device
		result = lgVidEnumerate(index, &eyetoyDesc);

		// check for the unplugged device error
		if(result == LGVID_ERR_DEVICE_LOST)
		{
			// as recommended by the docs, restart the enumeration
			numDevices = 0;
			goto enumerate;
		}

		// check for any other error
		if(LGVID_FAILED(result))
			break;

		// check what this device supports
		supportedTypes = (GVDeviceType)0;
		for(modeIndex = 0 ; modeIndex < eyetoyDesc.SupportedModesCount ; modeIndex++)
		{
			if(eyetoyDesc.SupportedModes[modeIndex].OperatingMode.AudioRate == LGVID_AUD_8000)
			{
				supportedTypes = GV_CAPTURE;
				break;
			}
		}

		if(supportedTypes == GV_CAPTURE)
		{
#if defined(GSI_UNICODE)
			char name[GV_DEVICE_NAME_LEN];
#endif

			// store this device's info in the array
			devices[numDevices].m_id = (int)(index | GVI_EYETOY_DEVICEID_BIT);
#if defined(GSI_UNICODE)
			strncpy(name, eyetoyDesc.FriendlyName, GV_DEVICE_NAME_LEN);
			name[GV_DEVICE_NAME_LEN - 1] = '\0';
			AsciiToUCS2String(name, devices[numDevices].m_name);
#else
			strncpy(devices[numDevices].m_name, eyetoyDesc.FriendlyName, GV_DEVICE_NAME_LEN);
			devices[numDevices].m_name[GV_DEVICE_NAME_LEN - 1] = '\0';
#endif
			devices[numDevices].m_deviceType = supportedTypes;
			devices[numDevices].m_defaultDevice = (GVDeviceType)0; // ps2 doesn't support default devices
			devices[numDevices].m_hardwareType = GVHardwarePS2Eyetoy;

			// one more device
			numDevices++;
		}
	}

	// return the number of devices we found
	return numDevices;
}

static void gviPS2EyetoyFreeDevice(GVIDevice * device)
{
	// delete it from the array
	// it will clear out internal data in the array's free function
	gviDeleteDeviceFromList(GVIDevices, device);
}

static GVBool gviPS2EyetoyStartDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;
	int result;

	if(type != GV_CAPTURE)
		return GVFalse;

	// start streaming
	result = lgVidStartStreaming(data->m_handle);
	if(LGVID_FAILED(result))
		return GVFalse;

	// no data in the capture buffer
	data->m_captureBufferBytes = 0;

	// started capturing
	data->m_capturing = GVTrue;

	return GVTrue;
}

static void gviPS2EyetoyStopDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;

	if(!(type & GV_CAPTURE))
		return;

	// stop streaming
	lgVidStopStreaming(data->m_handle);

	// stopped capturing
	data->m_capturing = GVFalse;

	// so a stop then start isn't continuous
	data->m_captureClock++;
}

static GVBool gviPS2EyetoyIsDeviceStarted(GVIDevice * device, GVDeviceType type)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;

	if(!(type & GV_CAPTURE))
		return GVFalse;

	return data->m_capturing;
}

static void gviPS2EyetoySetDeviceVolume(GVIDevice * device, GVDeviceType type, GVScalar volume)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;
	lgVidCameraControl cameraControl;
	u_char gain;

	if(!(type & GV_CAPTURE))
		return;

	// the gain varies from 0 (silence) to 8 (full gain)
	gain = (u_char)(volume * 8);
	gain = (unsigned char)min(gain, 8);

	// setup the camera control struct
	cameraControl.Flags = LGVID_FLAG_AUDIO_GAIN;
	cameraControl.AudioGain = gain;

	// set it
	lgVidSetCameraControl(data->m_handle, &cameraControl);
}

static GVScalar gviPS2EyetoyGetDeviceVolume(GVIDevice * device, GVDeviceType type)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;
	lgVidCameraControl cameraControl;
	int result;

	if(!(type & GV_CAPTURE))
		return 0;

	cameraControl.Flags = LGVID_FLAG_AUDIO_GAIN;
	result = lgVidGetCameraControl(data->m_handle, &cameraControl);
	if(LGVID_FAILED(result) || !(cameraControl.Flags & LGVID_FLAG_AUDIO_GAIN))
		return 1.0;

	return (GVScalar)(cameraControl.AudioGain / 8.0);
}

static void gviPS2EyetoySetCaptureThreshold(GVIDevice * device, GVScalar threshold)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;
	data->m_captureThreshold = threshold;
}

static GVScalar gviPS2EyetoyGetCaptureThreshold(GVIDevice * device)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;
	return data->m_captureThreshold;
}

static int gviPS2EyetoyGetAvailableCaptureBytes(GVDevice device)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;

	// don't do anything if we're not capturing
	if(!data->m_capturing)
		return 0;

	// eyetoy's don't provide this info
	// so, let the app think something is available
	return 1;
}

static void gviProcessCapturedFrame(GVDevice device, GVByte * frameOut, GVSample * frameIn, GVScalar * volume, GVBool * threshold)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;
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

static GVBool gviPS2EyetoyCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume)
{
	GVIPS2EyetoyData * data = (GVIPS2EyetoyData *)device->m_data;
	lgVidAudioDesc audioDesc;
	GVBool overThreshold;
	int result;
	int numFrames;
	int lenAvailable;
	int framesAvailable;
	int framesCaptured;
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

	// how many frames have we already captured?
	framesCaptured = (data->m_captureBufferBytes / GVIBytesPerFrame);

	// handle the data one frame at a time
	overThreshold = GVFalse;
	frameIn = data->m_captureBuffer;
	frameOut = packet;
	for(numFrames = 0 ; (numFrames < framesAvailable) && (numFrames < framesCaptured) ; numFrames++)
	{
		// process the frame
		gviProcessCapturedFrame(device, frameOut, frameIn, volume, &overThreshold);

		// update the frame pointers
		frameIn += GVISamplesPerFrame;
		frameOut += GVIEncodedFrameSize;
	}

	// adjust buffer based on what we delivered
	if(numFrames)
	{
		data->m_captureBufferBytes -= (numFrames * GVIBytesPerFrame);
		if(data->m_captureBufferBytes)
			memmove(data->m_captureBuffer, data->m_captureBuffer + (numFrames * GVIBytesPerFrame), (unsigned int)data->m_captureBufferBytes);
	}

	// read from the device if they still have available frames
	if(numFrames < framesAvailable)
	{
		// if it got here, any pending frames should have already been handled
		assert(data->m_captureBufferBytes < GVIBytesPerFrame);

		// setup the audio descriptor
		audioDesc.Samples = (((u_char *)data->m_captureBuffer) + data->m_captureBufferBytes);

		// read the audio
		result = lgVidReadAudio(data->m_handle, &audioDesc);
		if(LGVID_FAILED(result))
		{
			gviDeviceUnplugged(device);
			return GVFalse;
		}

		// check that we got something
		if(audioDesc.TimeStamp != -1)
		{
			// add to our total
			data->m_captureBufferBytes += audioDesc.TotalBytes;

			// set the frame buffer pointer
			frameIn = data->m_captureBuffer;

			for( ; numFrames < framesAvailable ; numFrames++)
			{
				// check if we have a full frame
				if(data->m_captureBufferBytes < GVIBytesPerFrame)
					break;

				// process the frame
				gviProcessCapturedFrame(device, frameOut, frameIn, volume, &overThreshold);

				// update the capture buffer count
				data->m_captureBufferBytes -= GVIBytesPerFrame;

				// update the frame pointers
				frameIn += GVISamplesPerFrame;
				frameOut += GVIEncodedFrameSize;
			}
			
			// move what we have to the front of the buffer
			if(data->m_captureBufferBytes)
				memmove(data->m_captureBuffer, frameIn, (unsigned)data->m_captureBufferBytes);
		}
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

static GVBool gviPS2EyetoyInitDevice(GVIDevice * device, int deviceIndex, GVDeviceType type)
{
	lgVidDeviceDesc eyetoyDesc;
	lgVidOpenParam openParam;
	GVIPS2EyetoyData * data;
	int result;
	int size;
	int i;

	// we only support capture
	if(type != GV_CAPTURE)
		return GVFalse;

	// enum the device to get a list of supported modes
	result = lgVidEnumerate(deviceIndex, &eyetoyDesc);
	if(LGVID_FAILED(result))
		return GVFalse;

	// find a supported device mode
	//TODO: make this better at choosing the video mode that uses the least CPU and memory (ideally none at all)
	for(i = 0 ; i < eyetoyDesc.SupportedModesCount ; i++)
	{
		if(eyetoyDesc.SupportedModes[i].OperatingMode.AudioRate == LGVID_AUD_8000)
			break;
	}
	if(i == eyetoyDesc.SupportedModesCount)
		return GVFalse;

	// setup the open param
	memcpy(&openParam.OperatingMode, &eyetoyDesc.SupportedModes[i].OperatingMode, sizeof(lgVidOperatingMode));
	openParam.AudioBufferDuration = GVI_CAPTURE_BUFFER_MILLISECONDS;

	// get a pointer to the data
	data = (GVIPS2EyetoyData *)device->m_data;

	// open the device
	result = lgVidOpen(deviceIndex, &openParam, &data->m_handle);
	if(LGVID_FAILED(result))
		return GVFalse;

	// set some data vars
	data->m_captureLastCrossedThresholdTime = (GVFrameStamp)(data->m_captureClock - GVI_HOLD_THRESHOLD_FRAMES - 1);

	// allocate the buffer
	size = (GVI_CAPTURE_BUFFER_MILLISECONDS * GV_BYTES_PER_SECOND / 1000);
	size += GVIBytesPerFrame;
	data->m_captureBuffer = (GVSample *)gsimalloc((unsigned)size);
	if(!data->m_captureBuffer)
	{
		lgVidClose(data->m_handle);
		return GVFalse;
	}

	return GVTrue;
}

GVDevice gviPS2EyetoyNewDevice(GVDeviceID deviceID, GVDeviceType type)
{
	GVIDevice * device;
	GVBool result;

	// check for wrong type
	if(type != GV_CAPTURE)
		return NULL;

	// check for the eyetoy bit
	if(!(deviceID & GVI_EYETOY_DEVICEID_BIT))
		return NULL;

	// create a new device
	device = gviNewDevice(deviceID, GVHardwarePS2Eyetoy, type, sizeof(GVIPS2EyetoyData));
	if(!device)
		return NULL;

	// init the device
	result = gviPS2EyetoyInitDevice(device, (int)(deviceID & ~GVI_EYETOY_DEVICEID_BIT), type);
	if(result == GVFalse)
	{
		gviFreeDevice(device);
		return NULL;
	}

	// store the pointers
	device->m_methods.m_freeDevice = gviPS2EyetoyFreeDevice;
	device->m_methods.m_startDevice = gviPS2EyetoyStartDevice;
	device->m_methods.m_stopDevice = gviPS2EyetoyStopDevice;
	device->m_methods.m_isDeviceStarted = gviPS2EyetoyIsDeviceStarted;
	device->m_methods.m_setDeviceVolume = gviPS2EyetoySetDeviceVolume;
	device->m_methods.m_getDeviceVolume = gviPS2EyetoyGetDeviceVolume;
	device->m_methods.m_setCaptureThreshold = gviPS2EyetoySetCaptureThreshold;
	device->m_methods.m_getCaptureThreshold = gviPS2EyetoyGetCaptureThreshold;
	device->m_methods.m_getAvailableCaptureBytes = gviPS2EyetoyGetAvailableCaptureBytes;
	device->m_methods.m_capturePacket = gviPS2EyetoyCapturePacket;

	// add it to the list
	gviAppendDeviceToList(GVIDevices, device);

	return device;
}

#endif //!defined(GV_NO_PS2_EYETOY)
