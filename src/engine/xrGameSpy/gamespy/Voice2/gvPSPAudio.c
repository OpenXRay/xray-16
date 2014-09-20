#include "gvDevice.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvUtil.h"
#include <audioinput.h>

#if !defined(_PSP)
#error This file should only be used with the PSP
#endif

/************
** DEFINES **
************/
// the number of samples to capture at a time
// is a multiple of 64 and 160
// at 11025hz, corresponds to about 30ms
#define GVI_INPUT_LEN 320

#define GVI_CAPTURE_THREAD_STACK_SIZE (1024 * 4)

// ADC settings
#define GVI_ADC_ALC    -6 //  -6 dB
#define GVI_ADC_GAIN  +30 // +30 dB
#define GVI_ADC_NOIZ  -60 // -60 dB
#define GVI_ADC_HOLD    0 // 0 ms
#define GVI_ADC_DECAY   3 // 192 ms
#define GVI_ADC_ATTACK  2 // 24 ms

/**********
** TYPES **
**********/
typedef struct
{
	GVBool m_capturing;
	GVScalar m_captureVolume;
	GVFrameStamp m_captureClock;
	GVScalar m_captureThreshold;
	GVFrameStamp m_captureLastCrossedThresholdTime;
	GVSample * m_captureBuffer;
	size_t m_captureBufferLen;  // len in samples (not bytes)
	size_t m_captureBufferWritePos;
	size_t m_captureBufferReadPos;
	SceUID m_captureBufferSemaphore;
	SceUID m_captureThreadID;
	GVBool m_captureThreadStop;
} GVIHardwareData;

/************
** GLOBALS **
************/
static GVIDevice * GVIPSPDevice = NULL;

/**************
** FUNCTIONS **
**************/
static void gviHardwareFreeDevice(GVIDevice * device);

static GVBool gviWaitSemaphore(GVIHardwareData * data)
{
	int rcode = sceKernelWaitSema(data->m_captureBufferSemaphore, 1, NULL);
	if(rcode == 0)
		return GVTrue;
	return GVFalse;
}

static GVBool gviSignalSemaphore(GVIHardwareData * data)
{
	int rcode = sceKernelSignalSema(data->m_captureBufferSemaphore, 1);
	if(rcode == 0)
		return GVTrue;
	return GVFalse;
}

GVBool gviHardwareStartup(void)
{
	SceAudioInputParam param;
	int rcode;

	// init mic capture
	param.alc = GVI_ADC_ALC;
	param.gain = GVI_ADC_GAIN;
	param.noiz = GVI_ADC_NOIZ;
	param.hold = GVI_ADC_HOLD;
	param.decay = GVI_ADC_DECAY;
	param.attack = GVI_ADC_ATTACK;
	rcode = sceAudioInputInitEx(&param);
	if(rcode < 0)
		return GVFalse;

	return GVTrue;
}

void gviHardwareCleanup(void)
{
	if(GVIPSPDevice)
	{
		gviHardwareFreeDevice(GVIPSPDevice);
		GVIPSPDevice = NULL;
	}
}

void gviHardwareThink(void)
{
	// no thinking needed
}

int gviHardwareListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
	if(maxDevices < 1)
		return 0;

	if(!(types & GV_CAPTURE))
		return 0;

	memset(&devices[0], 0, sizeof(GVDeviceInfo));
	devices[0].m_id = 0;
	strcpy(devices[0].m_name, _T("Headset Mic"));
	devices[0].m_deviceType = GV_CAPTURE;
	devices[0].m_defaultDevice = GV_CAPTURE;
	devices[0].m_hardwareType = GVHardwarePSPHeadset;

	return 1;
}

static void gviHardwareFreeDevice(GVIDevice * device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	// don't track this device anymore
	GVIPSPDevice = NULL;

 	// tell the thread to stop
	// the thread will free the device after it stops
	data->m_captureThreadStop = GVTrue;
}

static GVBool gviHardwareStartDevice(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type != GV_CAPTURE)
		return GVFalse;

	// already capturing?
	if(data->m_capturing == GVTrue)
		return GVTrue;

	// set vars
	data->m_captureBufferWritePos = 0;
	data->m_captureBufferReadPos = 0;

	// start capturing
	data->m_capturing = GVTrue;

	return GVTrue;
}

static void gviHardwareStopDevice(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;

	if(type != GV_CAPTURE)
		return;

	// stop capturing
	data->m_capturing = GVFalse;

	// increment the clock so new audio isn't contiguous
	data->m_captureClock++;
}

static GVBool gviHardwareIsDeviceStarted(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	if(type != GV_CAPTURE)
		return GVFalse;
	return data->m_capturing;
}

static void gviHardwareSetDeviceVolume(GVIDevice * device, GVDeviceType type, GVScalar volume)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	if(type != GV_CAPTURE)
		return;
	data->m_captureVolume = volume;
}

static GVScalar gviHardwareGetDeviceVolume(GVIDevice * device, GVDeviceType type)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	if(type != GV_CAPTURE)
		return (GVScalar)0;
	return data->m_captureVolume;
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
	int numSamples;

	if(!data->m_capturing)
		return GVFalse;

	// figure out how many samples are available
	gviWaitSemaphore(data);
		if(data->m_captureBufferWritePos < data->m_captureBufferReadPos)
			numSamples = (data->m_captureBufferLen - data->m_captureBufferReadPos);
		else
			numSamples = (data->m_captureBufferWritePos - data->m_captureBufferReadPos);
	gviSignalSemaphore(data);

	return numSamples;
}

static GVBool gviHardwareCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	int numBytes;
	int numSamples;
	int numFrames;
	GVBool overThreshold = GVFalse;
	int lenAvailable;
	int framesAvailable;
	int i, j;
	GVSample * readPtr;

	if(!data->m_capturing)
		return GVFalse;

	// figure out how many encoded bytes they can handle
	lenAvailable = *len;

	// clear the len and volume
	*len = 0;
	if(volume)
		*volume = 0;

	// figure out how many bytes can be captured
	numSamples = gviHardwareGetAvailableCaptureBytes(device);
	numBytes = (numSamples * GV_BYTES_PER_SAMPLE);

	// figure out how many frames that is
	numFrames = (numBytes / GVIBytesPerFrame);

	if(numFrames == 0)
		return GVFalse;

	// figure out how many frames they can handle
	framesAvailable = (lenAvailable / GVIEncodedFrameSize);

	// don't give them more frames than they can handle
	numFrames = min(numFrames, framesAvailable);
	if(!numFrames)
		return GVFalse;

	// figure out how many bytes to capture
	numBytes = (numFrames * GVIBytesPerFrame);
	numSamples = (numBytes / GV_BYTES_PER_SAMPLE);

	// get the read pointer
	readPtr = (data->m_captureBuffer + data->m_captureBufferReadPos);

	// get the volume if they're interested
	if(volume)
		*volume = gviGetSamplesVolume(readPtr, numSamples);

	// check against the threshold
	if(volume)
	{
		// we already got the volume, so use that to check
		overThreshold = (*volume >= data->m_captureThreshold);
	}
	else
	{
		// we didn't get a volume, so check the samples directly
		overThreshold = gviIsOverThreshold(readPtr, numSamples, data->m_captureThreshold);
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
		// store the framestamp
		*frameStamp = data->m_captureClock;

		// handle the data one frame at a time
		for(i = 0 ; i < numFrames ; i++)
		{
			// scale the data
			if(data->m_captureVolume < 1.0)
			{
				for(j = 0 ; j < GVISamplesPerFrame ; j++)
					readPtr[j] = (GVSample)(readPtr[j] * data->m_captureVolume);
			}

			// filter
			if(device->m_captureFilterCallback)
				device->m_captureFilterCallback(device, readPtr, (GVFrameStamp)(data->m_captureClock + i));

			// encode the buffer into the packet
			gviEncode(packet + (GVIEncodedFrameSize * i), readPtr);

			// update the loop info as needed
			readPtr += GVISamplesPerFrame;
		}
	}

	// advance the read position and clock
	data->m_captureBufferReadPos += numSamples;
	data->m_captureBufferReadPos %= data->m_captureBufferLen;
	data->m_captureClock += numFrames;

	// set the len
	*len = (numFrames * GVIEncodedFrameSize);

	// return false if we didn't get a packet
	if(!overThreshold)
		return GVFalse;

	return GVTrue;
}

static int gviPSPCaptureThread(SceSize args, void * argp)
{
	GVIDevice * device = *(GVIDevice **)argp;
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	int rcode;

	assert(device);
	assert(args == sizeof(GVIDevice*));

	// loop until we're told to stop
	while(!data->m_captureThreadStop)
	{
		// are we capturing?
		if(data->m_capturing)
		{
			// get some input
			rcode = sceAudioInputBlocking(GVI_INPUT_LEN, GV_SAMPLES_PER_SECOND,
				data->m_captureBuffer + data->m_captureBufferWritePos);
			if(rcode < 0)
			{
				gviDeviceUnplugged(device);
			}
			else
			{
				gviWaitSemaphore(data);
					data->m_captureBufferWritePos += GVI_INPUT_LEN;
					data->m_captureBufferWritePos %= data->m_captureBufferLen;
				gviSignalSemaphore(data);
			}
		}
		else
		{
			// sleep
			msleep(10);
		}
	}

	// end sampling
	// if another device has already been created, don't do this
	if(GVIPSPDevice != NULL)
		sceAudioInputBlocking(GVI_INPUT_LEN, GV_SAMPLES_PER_SECOND, NULL);

	// free the device and its memory
	gsifree(data->m_captureBuffer);
	sceKernelDeleteSema(data->m_captureBufferSemaphore);
	gviFreeDevice(device);

	sceKernelExitThread(0);

	return 0;
}

static GVBool gviPSPAudioInitDevice(GVIDevice *device)
{
	GVIHardwareData * data = (GVIHardwareData *)device->m_data;
	size_t size;
	size_t oldSize;
	int rcode;

	// create a semaphore
	data->m_captureBufferSemaphore = sceKernelCreateSema("capture buffer", SCE_KERNEL_SA_THFIFO, 1, 1, NULL);
	if(data->m_captureBufferSemaphore <= 0)
		return GVFalse;

	// figure out the buffer size
	size = gviMultiplyByBytesPerMillisecond(GVI_CAPTURE_BUFFER_MILLISECONDS);
	size /= GV_BYTES_PER_SAMPLE; // convert from bytes to samples
	do
	{
		// it needs to be a multiple of both the frame size and the input len
		oldSize = size;
		size = gviRoundUpToNearestMultiple(size, GVISamplesPerFrame);
		size = gviRoundUpToNearestMultiple(size, GVI_INPUT_LEN);
	}
	while(size != oldSize);
	data->m_captureBufferLen = size;

	// allocate the buffer
	data->m_captureBuffer = (GVSample *)gsimalloc(size * GV_BYTES_PER_SAMPLE);
	if(!data->m_captureBuffer)
	{
		sceKernelDeleteSema(data->m_captureBufferSemaphore);
		return GVFalse;
	}

	// create capture thread
	data->m_captureThreadID = sceKernelCreateThread("capture",gviPSPCaptureThread,
		SCE_KERNEL_USER_HIGHEST_PRIORITY, GVI_CAPTURE_THREAD_STACK_SIZE, 0, NULL);
	if(data->m_captureThreadID < 0)
	{
		gsifree(data->m_captureBuffer);
		sceKernelDeleteSema(data->m_captureBufferSemaphore);
		return GVFalse;
	}

	// start capture thread
	rcode = sceKernelStartThread(data->m_captureThreadID, sizeof(GVIDevice*), &device);
	if(rcode < 0)
	{
		sceKernelDeleteThread(data->m_captureThreadID);
		gsifree(data->m_captureBuffer);
		sceKernelDeleteSema(data->m_captureBufferSemaphore);
		return GVFalse;
	}

	return GVTrue;
}

GVDevice gviHardwareNewDevice(GVDeviceID deviceID, GVDeviceType type)
{
	GVIDevice * device;
	GVIHardwareData * data;
	GVBool result;

	// we only do capture
	if(type != GV_CAPTURE)
		return NULL;

	// there can only be one device at a time
	if(GVIPSPDevice != NULL)
		return NULL;

	// create the device
	device = gviNewDevice(deviceID, GVHardwarePSPHeadset, type, sizeof(GVIHardwareData));
	if(!device)
		return NULL;

	// init the device
	result = gviPSPAudioInitDevice(device);
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

	// get a pointer to the data
	data = (GVIHardwareData *)device->m_data;

	// init vars
	data->m_captureVolume = (GVScalar)1.0;
	data->m_captureClock = 0;
	data->m_captureLastCrossedThresholdTime = (data->m_captureClock - GVI_HOLD_THRESHOLD_FRAMES - 1);

	GVIPSPDevice = device;

	return device;
}
