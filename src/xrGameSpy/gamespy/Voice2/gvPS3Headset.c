///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "gvPS3Headset.h"
#if !defined(GV_NO_PS3_HEADSET)
#include "gvDevice.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvUtil.h"
#include <types.h>
#include <sys/event.h>
#include <cell/audio.h>
#include <cell/mic.h>
#include <cell/sysmodule.h>
#include <cell/mixer.h>


#if !defined(_PS3)
#error This file should only be used with the PlayStation3
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// definitions
#define GVI_PLAYBACK_STAYAHEAD_MILLISECONDS  50

// Used as a starting value for the audio port queue key
#define GVI_AUDIO_QUEUE_KEY_BASE	0x0000998877660000ULL

// Used as a starting value for the capture event queue key
#define GVI_CAPTURE_QUEUE_KEY_BASE  0x0000000072110700UL 

#define GVI_AUDIO_QUEUE_DEPTH		4
#define GVI_CAPTURE_VOLUME_MAX      241
#define GVI_PLAYBACK_NUM_BLOCKS     CELL_AUDIO_BLOCK_32
#define GVI_PLAYBACK_BLOCK_SAMPLES  CELL_AUDIO_BLOCK_SAMPLES
#define GVI_PLAYBACK_SAMPLE_RATE    48000 //Hz
#define GVI_PS3_MIC_BUFFER_MS       1000
#define GVI_LOCAL_TALK_MAX          10
#define GVI_PLAYBACK_SAMPLE_FACTOR  (GVI_PLAYBACK_SAMPLE_RATE / gviGetSampleRate())
#define GVI_NUM_CHANNELS            CELL_AUDIO_PORT_2CH


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// structs
typedef struct
{
	sys_event_t         m_captureCallbackEvent;
	sys_event_queue_t   m_captureCallbackQueue;
	uint64_t            m_captureEventQueueKey;
	GVBool              m_capturing;
	GVScalar            m_captureVolume;
	GVFrameStamp        m_captureClock;
	GVScalar            m_captureThreshold;
	GVFrameStamp        m_captureLastCrossedThresholdTime;
	float              *m_capturePreConvertBuffer;
	size_t              m_capturePreConvertBufferLen;
	GVSample           *m_captureBuffer;

	int                 m_captureBufferBytes;
	int                 m_deviceNum; // used to keep the microphone device number
	GVBool              m_captureMicOpen;

	GVBool              m_playing;
	GVScalar            m_playbackVolume;
	GVFrameStamp        m_playbackClock;
	GVISourceList       m_playbackSources;
	GVSample           *m_playbackBuffer;
	gsi_u32             m_playbackCellAudioPortNum;
	CellAudioPortConfig m_playbackCellAudioConfig;
	sys_event_queue_t   m_playbackQueue;
	uint64_t            m_playbackQueueKey;
	int                 m_playbackPortPos;
} GVIPS3HeadsetData;



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// globals
static GVIDeviceList GVIDevices;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// gets the device by the deviceID
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// frees the device
static void gviFreeArrayDevice(void * elem)
{
	GS_ASSERT(elem);
	GVIDevice * device = *(GVIDevice **)elem;
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;
	int result;

	// turn off mic if its a capture device and if the mic port is open
	if (data->m_capturing && cellMicIsOpen(data->m_deviceNum))
		result = cellMicClose(data->m_deviceNum);

	// Destroy the callback and playback queues
	if (data->m_playbackQueue)
		cellAudioRemoveNotifyEventQueue(data->m_playbackQueueKey);
	if (data->m_captureCallbackQueue)
		cellMicRemoveNotifyEventQueue(data->m_captureEventQueueKey);
	if (data->m_playbackQueue)
		sys_event_queue_destroy(data->m_playbackQueue, 0);
	if (data->m_captureCallbackQueue)
		sys_event_queue_destroy(data->m_captureCallbackQueue, 0);

	// close audio port
	if (data->m_playing)
		cellAudioPortClose(data->m_playbackCellAudioPortNum);

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
		gsifree(data->m_capturePreConvertBuffer);
	}

	// free the device
	gviFreeDevice(device);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// starts up the headset
GVBool gviPS3HeadsetStartup(void)
{
	int result;

	// create the array of devices
	GVIDevices = gviNewDeviceList(gviFreeArrayDevice);
	if(!GVIDevices)
		return GVFalse;

	// initialize the mic library
	result = cellSysmoduleLoadModule(CELL_SYSMODULE_MIC);
	if(result != CELL_OK)
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
		return GVFalse;
	}

	result = cellMicInit();
	if(result != CELL_OK)
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
		return GVFalse;
	}

	// initialize the audio library
	result = cellSysmoduleLoadModule(CELL_SYSMODULE_AUDIO);
	if(result != CELL_OK)
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
		return GVFalse;
	}

	result = cellAudioInit();
	if(result != CELL_OK && result != CELL_AUDIO_ERROR_ALREADY_INIT)
	{
		gviFreeDeviceList(GVIDevices);
		cellMicEnd();
		GVIDevices = NULL;
		return GVFalse;
	}

	return GVTrue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// unloads the headset
void gviPS3HeadsetCleanup(void)
{
	// free the device array
	if(GVIDevices)
	{
		gviFreeDeviceList(GVIDevices);
		GVIDevices = NULL;
	}

	cellMicEnd();
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// process playback if there is any in the queue
static GVBool gviPlaybackDeviceThink(GVIDevice * device)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;
	int remainingSamples;
	int numFrames;
	GVBool wroteToBuffer;
	int i, j, k;
	sys_event_t playbackQueueEvent;
	int result;
	int playbackReadPos;
	unsigned int currentBlock;
	int totalSamples;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVTrue;

	// check the queue
	result = sys_event_queue_receive(data->m_playbackQueue, &playbackQueueEvent, 1);
	if(result == ETIMEDOUT)
	{
		return GVTrue;
	}
	if(result != CELL_OK)
	{
		return GVFalse;
	}

	totalSamples = (GVI_PLAYBACK_NUM_BLOCKS * GVI_PLAYBACK_BLOCK_SAMPLES);

	currentBlock = (unsigned int)*(uint64_t *)(data->m_playbackCellAudioConfig.readIndexAddr);
	playbackReadPos = (currentBlock * GVI_PLAYBACK_BLOCK_SAMPLES);

	if(data->m_playbackPortPos == -1)
	{
		unsigned int nextBlock = (currentBlock + 1) % GVI_PLAYBACK_NUM_BLOCKS; // write target is next block
		data->m_playbackPortPos = (nextBlock * GVI_PLAYBACK_BLOCK_SAMPLES);
	}

	remainingSamples = (((playbackReadPos + totalSamples) - data->m_playbackPortPos) % totalSamples);

	// figure out the number of frames that we can write
	numFrames = ((remainingSamples / GVI_PLAYBACK_SAMPLE_FACTOR) / GVISamplesPerFrame);

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
		
		// write to port buffer from m_playbackBuffer
		// converting from sample to float
		// converting from 8khz or 16KHz to 48khz and mono to stereo (write each sample 6-12 
		// times depending on sample rate)
		for(j = 0 ; j < GVISamplesPerFrame ; j++)
		{
			float sample = ((float)data->m_playbackBuffer[j] / (float)SHRT_MAX);
			for(k = 0; k < GVI_PLAYBACK_SAMPLE_FACTOR; k++)
			{
				float *dest = (float *)(data->m_playbackCellAudioConfig.portAddr +
					(data->m_playbackPortPos * GVI_NUM_CHANNELS * sizeof(float)));
				*dest++ = sample;
				*dest = sample;
				data->m_playbackPortPos++;
				data->m_playbackPortPos %= totalSamples;
			}
		}

		// update the clock
		data->m_playbackClock++;
	}

	return GVTrue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// called every once in a while to process playback
void gviPS3HeadsetThink(void)
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// gets the devices detected
int gviPS3HeadsetListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
	int index;
	int numDevices = 0;

	maxDevices = min(maxDevices, CELL_MAX_MICS);

	for(index = 0 ; index < maxDevices ; index++)
	{
		if(cellMicIsAttached(index))
		{
			int deviceType;
			if (cellMicGetType(index, &deviceType) == CELL_OK)
			{
				if (deviceType == CELLMIC_TYPE_USBAUDIO)
				{
					devices[numDevices].m_id = index;
					devices[numDevices].m_deviceType = GV_CAPTURE | GV_PLAYBACK;
					devices[numDevices].m_defaultDevice = (GVDeviceType)0; 
					_tcscpy(devices[numDevices].m_name, _T("USB Headset"));
					devices[numDevices].m_hardwareType = GVHardwarePS3Headset;
				}
				else if (deviceType == CELLMIC_TYPE_BLUETOOTH)
				{
					devices[numDevices].m_id = index;
					devices[numDevices].m_deviceType = GV_CAPTURE | GV_PLAYBACK;
					devices[numDevices].m_defaultDevice = (GVDeviceType)0; 
					_tcscpy(devices[numDevices].m_name, _T("Bluetooth Headset"));
					devices[numDevices].m_hardwareType = GVHardwarePS3Headset;
				}
			}

			numDevices++;
		}
	}
	return numDevices;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// unloades the device from the device array
static void gviPS3HeadsetFreeDevice(GVIDevice * device)
{
	// delete it from the array
	// it will clear out internal data in the array's free function
	gviDeleteDeviceFromList(GVIDevices, device);
}

static GVBool gviPS3HeadsetInitHeadphone(GVIPS3HeadsetData *data)
{
	int result;
	CellAudioPortParam  audioParam;
	sys_event_queue_attribute_t	aQueueAttr;
	int aCount;	

	audioParam.attr = CELL_AUDIO_PORTATTR_OUT_SECONDARY;
	audioParam.nBlock = GVI_PLAYBACK_NUM_BLOCKS;
	audioParam.nChannel = GVI_NUM_CHANNELS;

	// set the audio port value to something really abnormal
	// so that if the open function fails, the gviPS3HeadsetClose
	// will close the port only if a valid port number is assigned
	data->m_playbackCellAudioPortNum = 0xFFFFFFFF;

	// Playback
	///////////

	result = cellAudioPortOpen(&audioParam, &data->m_playbackCellAudioPortNum);
	if (result != CELL_OK)
	{	
		return GVFalse;
	}

	// get the config for the audio port so we can use it to write data to the audio port ring buffer
	result = cellAudioGetPortConfig(data->m_playbackCellAudioPortNum, &data->m_playbackCellAudioConfig);
	if (result != CELL_OK)
	{
		return GVFalse;
	}

	// Event queue notify tells us when the system is ready to play new data
	aCount = 0;
	sys_event_queue_attribute_initialize(aQueueAttr);
	aQueueAttr.attr_protocol  = SYS_SYNC_FIFO;
	data->m_playbackQueueKey = GVI_AUDIO_QUEUE_KEY_BASE;

	while (aCount < 10)
	{
		result = sys_event_queue_create(&data->m_playbackQueue, &aQueueAttr, 
			data->m_playbackQueueKey, GVI_AUDIO_QUEUE_DEPTH);
		if (result == CELL_OK)
		{
			break;
		}
		// search unused key
		data->m_playbackQueueKey = GVI_AUDIO_QUEUE_KEY_BASE | (rand() & 0x0ffff);
		aCount++;
	}

	if (result != CELL_OK)
	{
		return GVFalse;
	}

	// register event queue to libaudio
	result = cellAudioSetNotifyEventQueue(data->m_playbackQueueKey);
	if (result < 0)
	{
		return GVFalse;
	}

	return GVTrue;
}

GVBool gviPS3HeadsetInitMic(GVIPS3HeadsetData *data)
{
	int result = 0;
	int aMsg = 0;
	int aDevNum=0;

	int aCount = 0; 
	sys_event_queue_attribute_t  equeue_attr = {SYS_SYNC_FIFO, SYS_PPU_QUEUE, ""};

	// Event queue key for libmic
	// checks for an event occurrence 
	//create event queue to recv "MicIn" callback from MIOS
	data->m_captureEventQueueKey = GVI_CAPTURE_QUEUE_KEY_BASE;

	while ( (aCount++) < 100 )
	{
		result = sys_event_queue_create(&data->m_captureCallbackQueue, 
			&equeue_attr, data->m_captureEventQueueKey, 32);
		if (result == CELL_OK) break;
		data->m_captureEventQueueKey =  GVI_CAPTURE_QUEUE_KEY_BASE | ( rand() & 0xffff);
	}
	if (result != CELL_OK)
	{		
		return GVFalse;
	}

	//install "MicIn" system-callback(with devnum == -1) to recv attach/detach event
	result = cellMicSetNotifyEventQueue(data->m_captureEventQueueKey);
	if (result != CELL_OK)
	{
		return GVFalse;
	}

	// Wait up to 10 ms before checking if the mike is attached
	result = sys_event_queue_receive(data->m_captureCallbackQueue, 
		&data->m_captureCallbackEvent, 10000);
	if(result == ETIMEDOUT)
		return GVFalse;

	aMsg = (int)data->m_captureCallbackEvent.data1;
	aDevNum = (int)data->m_captureCallbackEvent.data2;

	if (aMsg == CELLMIC_ATTACH && aDevNum == data->m_deviceNum)
	{
		// start with the default audio device
		result = cellMicOpenEx(data->m_deviceNum, GVI_PLAYBACK_SAMPLE_RATE, 1, 
			GV_SAMPLES_PER_SECOND, GVI_PS3_MIC_BUFFER_MS, CELLMIC_SIGTYPE_DSP);

		if (result != CELL_OK)
		{
			return GVFalse;
		}
		data->m_captureMicOpen = GVTrue;
	}
	return GVTrue;
}

void gviPS3HeadsetCloseHeadphone(GVIPS3HeadsetData *data)
{
	// Remove and Destroy the callback and playback queues
	if (data->m_playbackQueue)
		cellAudioRemoveNotifyEventQueue(data->m_playbackQueue);
	if (data->m_playbackQueue)
		sys_event_queue_destroy(data->m_playbackQueue, 0);

	// the audio port needs to be closed if a device wasn't initialized properly
	if (data->m_playbackCellAudioPortNum != 0xFFFFFFFF)
		cellAudioPortClose(data->m_playbackCellAudioPortNum);
}

void gviPS3HeadsetCloseMic(GVIPS3HeadsetData *data)
{
	// Remove and Destroy the callback and playback queues
	if (data->m_captureCallbackQueue)
		cellAudioRemoveNotifyEventQueue(data->m_captureCallbackQueue);
	if (data->m_captureCallbackQueue)
		sys_event_queue_destroy(data->m_captureCallbackQueue, 0);

	// The mic should be closed if it is open 
	if (cellMicIsOpen(data->m_deviceNum))
		cellMicClose(data->m_deviceNum);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// initializes the device for playback
static GVBool gviStartPlaybackDevice(GVIPS3HeadsetData * data)
{
	int result;

	sys_event_queue_drain(data->m_playbackQueue);

	data->m_playbackPortPos = -1;

	result = cellAudioPortStart(data->m_playbackCellAudioPortNum);
	if (result != CELL_OK)
	{
		return GVFalse;
	}

	// clear the clock
	data->m_playbackClock = 0;

	// started playing
	data->m_playing = GVTrue;

	return GVTrue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// initializes the device for capture
static GVBool gviStartCaptureDevice(GVIPS3HeadsetData * data)
{
	int result;
	// start the mic capture
	if (data->m_captureMicOpen)
	{		
		result = cellMicStart(data->m_deviceNum);
		if (result != CELL_OK)
			return GVFalse;
	}
	else 
		return GVFalse;

	cellMicReset(data->m_deviceNum);
	// no data in the capture buffer
	data->m_captureBufferBytes = 0;
	data->m_capturePreConvertBufferLen = 0;

	// started capturing
	data->m_capturing = GVTrue;

	return GVTrue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// stops the device
static void gviPS3HeadsetStopDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	if(type & GV_PLAYBACK)
	{
		cellAudioPortStop(data->m_playbackCellAudioPortNum);

		// clear the playback buffer
		memset(data->m_playbackBuffer, 0, GVIBytesPerFrame);

		// stopped playing
		data->m_playing = GVFalse;

		// clear any pending sources & buffers
		gviClearSourceList(data->m_playbackSources);
	}
	if(type & GV_CAPTURE)
	{
		// stop the capture buffer
		cellMicStop(data->m_deviceNum);

		// clear capture buffer
		memset(data->m_captureBuffer, 0, GVIBytesPerFrame);

		// stopped capturing
		data->m_capturing = GVFalse;

		// so a stop then start isn't continuous
		data->m_captureClock++;
	}

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// starts the device
static GVBool gviPS3HeadsetStartDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	if(type == GV_PLAYBACK)
	{
		return gviStartPlaybackDevice(data);
	}
	if(type == GV_CAPTURE)
	{
		return gviStartCaptureDevice(data);
	}
	if(type == GV_CAPTURE_AND_PLAYBACK)
	{
		if(!gviStartPlaybackDevice(data))
			return GVFalse;
		if(!gviStartCaptureDevice(data))
		{
			gviPS3HeadsetStopDevice(device, GV_PLAYBACK);
			return GVFalse;
		}
		return GVTrue;
	}
	return GVFalse;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// checks to see if the device is ready
static GVBool gviPS3HeadsetIsDeviceStarted(GVIDevice * device, GVDeviceType type)
{
	// NULL device means not even created or started
	GS_ASSERT(device);
	if (!device)
		return GVFalse;

	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	if(type == GV_PLAYBACK)
		return data->m_playing;
	if(type == GV_CAPTURE)
		return data->m_capturing;
	if(type == GV_CAPTURE_AND_PLAYBACK)
		return (data->m_playing && data->m_capturing);
	return GVFalse;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// sets the volume for the device
static void gviPS3HeadsetSetDeviceVolume(GVIDevice * device, GVDeviceType type, GVScalar volume)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	if(type & GV_PLAYBACK)
	{
		cellAudioSetPortLevel(data->m_playbackCellAudioPortNum, volume);
		data->m_playbackVolume = volume;
	}
	if(type & GV_CAPTURE)
	{
		cellMicSetDeviceAttr(data->m_deviceNum, CELLMIC_DEVATTR_VOLUME, (int)(volume * GVI_CAPTURE_VOLUME_MAX), 0);
		data->m_captureVolume = volume;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// gets the device volume
static GVScalar gviPS3HeadsetGetDeviceVolume(GVIDevice * device, GVDeviceType type)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	if(type & GV_PLAYBACK)
		return data->m_playbackVolume;
	return data->m_captureVolume;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// sets the capture threshold
static void gviPS3HeadsetSetCaptureThreshold(GVIDevice * device, GVScalar threshold)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;
	data->m_captureThreshold = threshold;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// gets the capture threshold
static GVScalar gviPS3HeadsetGetCaptureThreshold(GVIDevice * device)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;
	return data->m_captureThreshold;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// gets the available capture bytes
static int gviPS3HeadsetGetAvailableCaptureBytes(GVDevice device)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	// don't do anything if we're not capturing
	if(!data->m_capturing)
		return 0;

	// no call listed in the Sony documentation, so just return 1
	return 1;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
GVBool gviPS3HeadsetHandleMicAttach(GVIPS3HeadsetData *data)
{
	int result;

	// start with the default audio device
	result = cellMicOpenEx(data->m_deviceNum, GVI_PLAYBACK_SAMPLE_RATE, 1, 
		GV_SAMPLES_PER_SECOND, GVI_PS3_MIC_BUFFER_MS, CELLMIC_SIGTYPE_DSP);

	if (result != CELL_OK)
	{
		return GVFalse;
	}
	data->m_captureMicOpen = GVTrue;
	result = cellMicStart(data->m_deviceNum);
	if (result != CELL_OK)
	{
		return GVFalse;
	}
	result = cellMicReset(data->m_deviceNum);
	if (result != CELL_OK)
	{
		return GVFalse;
	}
	return GVTrue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// processes the captured frame
static void gviProcessCapturedFrame(GVDevice device, GVSample *frameIn, GVByte* frameOut, GVScalar *volume, GVBool *threshold)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// captures a packet
static GVBool gviPS3HeadsetCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;
	GVBool overThreshold;
	int result;
	int numFrames;
	int readSize;
	int lenAvailable;
	int framesAvailable;
	GVByte * frameOut;
	float *frameIn;
	int aCaptureQueueMsg;
	int aDeviceIndex;
	int sample;
	int localtalk;
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
	overThreshold = GVFalse;

	frameOut = packet;
	//frameIn = data->m_capturePreConvertBuffer;
	// handle the data one frame at a time
	for(numFrames = 0 ; numFrames < framesAvailable ; numFrames++)
	{
		// Wait up to 1 us before checking if the mike is attached
		result = sys_event_queue_receive(data->m_captureCallbackQueue, 
			&data->m_captureCallbackEvent, 1);
		if(result == ETIMEDOUT)
		{
			break;
		}	

		aCaptureQueueMsg = (int)data->m_captureCallbackEvent.data1;
		aDeviceIndex = (int)data->m_captureCallbackEvent.data2;
		if (aDeviceIndex != data->m_deviceNum)
			continue;

		if (aCaptureQueueMsg == CELLMIC_ATTACH)
		{
			if (!gviPS3HeadsetHandleMicAttach(data))
			{
				break;
			}
		}	
		else if (aCaptureQueueMsg == CELLMIC_DETACH)
		{
			gviDeviceUnplugged(device);
			return GVFalse;
		}
		else if (aCaptureQueueMsg == CELLMIC_DATA)
		{
			// read this frame
			//readSize = (GVIBytesPerFrame - data->m_captureBufferBytes);
			readSize = ((GVISamplesPerFrame - data->m_capturePreConvertBufferLen) * sizeof(float));
			frameIn = data->m_capturePreConvertBuffer + data->m_capturePreConvertBufferLen;
			readSize = cellMicRead(data->m_deviceNum, frameIn, readSize);

			cellMicGetSignalState(0, CELLMIC_SIGSTATE_LOCTALK, &localtalk);
			if (localtalk < (int)(GVI_LOCAL_TALK_MAX * data->m_captureThreshold))
			{
				return GVFalse;
			}

			if (readSize == CELL_MICIN_ERROR_DEVICE_NOT_FOUND)
			{
				gviDeviceUnplugged(device);
				return GVFalse;
			}

			data->m_capturePreConvertBufferLen += (readSize / sizeof(float));
			if(data->m_capturePreConvertBufferLen < GVISamplesPerFrame)
			{
				break;
			}

			// convert the data from 32 bit Big Endian Floats [-1.0,1.0]
			// to 16 bit short and write the values into the buffer
			for (sample = 0; sample < GVISamplesPerFrame; sample++)
			{
				data->m_captureBuffer[sample] = (GVSample)((SHRT_MAX)*data->m_capturePreConvertBuffer[sample]); 
			}

			// process the frame
			gviProcessCapturedFrame(device, data->m_captureBuffer, frameOut, volume, &overThreshold);

			// we got a full frame, so there's no leftover
			data->m_captureBufferBytes = 0;

			// update the frame pointer
			frameOut += GVIEncodedFrameSize;

			data->m_capturePreConvertBufferLen = 0;
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// sends the packet to the mixer
static void gviPS3HeadsetPlayPacket(GVIDevice * device, const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return;

	//add it
	gviAddPacketToSourceList(data->m_playbackSources, packet, len, source, frameStamp, mute, data->m_playbackClock);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// checks to see if we're talking
static GVBool gviPS3HeadsetIsSourceTalking(GVDevice device, GVSource source)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviIsSourceTalking(data->m_playbackSources, source);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// lists the talking sources
static int gviPS3HeadsetListTalkingSources(GVDevice device, GVSource sources[], int maxSources)
{
	GVIPS3HeadsetData * data = (GVIPS3HeadsetData *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviListTalkingSources(data->m_playbackSources, sources, maxSources);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// initializes the device
static GVBool gviPS3HeadsetInitDevice(GVIDevice * device, int deviceIndex, GVDeviceType type)
{
	GVIPS3HeadsetData * data;

	// get a pointer to the data
	data = (GVIPS3HeadsetData *)device->m_data;
	data->m_deviceNum = deviceIndex;

	// handle playback specific stuff
	if(type & GV_PLAYBACK)
	{
		// create the array of sources
		data->m_playbackSources = gviNewSourceList();
		if(!data->m_playbackSources)
		{
			return NULL;
		}

		// allocate the buffer to hold one frame
		data->m_playbackBuffer = (GVSample *)gsimemalign(16, (unsigned int)(GVIBytesPerFrame));
		if(!data->m_playbackBuffer)
		{
			gviFreeSourceList(data->m_playbackSources);
			return NULL;
		}
		if (!gviPS3HeadsetInitHeadphone(data))
		{
			gviFreeSourceList(data->m_playbackSources);
			gsifree(data->m_playbackBuffer);
			gviPS3HeadsetCloseHeadphone(data);
			return NULL;
		}
		data->m_playbackVolume = 1.0;
	}

	// handle capture specific stuff
	if(type & GV_CAPTURE)
	{
		// set some data vars
		data->m_captureClock = 0;
		data->m_captureVolume = 1.0;
		data->m_capturePreConvertBufferLen = 0;

		data->m_captureLastCrossedThresholdTime = (GVFrameStamp)(data->m_captureClock - GVI_HOLD_THRESHOLD_FRAMES - 1);

		// allocate the buffer
		data->m_captureBuffer = (GVSample *)gsimemalign(16, (unsigned int)(GVIBytesPerFrame));
		if(!data->m_captureBuffer)
		{
			// Need to free any resources in data for playback
			// Also library needs to close audio port and mic
			// The library still needs to remain
			if(type & GV_PLAYBACK)
			{
				gviFreeSourceList(data->m_playbackSources);
				gsifree(data->m_playbackBuffer);
				gviPS3HeadsetCloseHeadphone(data);
			}
			return NULL;
		}

		data->m_capturePreConvertBuffer = (float *)gsimemalign(16,(unsigned int)(GVISamplesPerFrame * sizeof(float)));
		if(!data->m_capturePreConvertBuffer)
		{
			// Need to free any resources in data for playback
			// Also library needs to close audio port and mic
			// The library still needs to remain
			if(type & GV_PLAYBACK)
			{
				gviFreeSourceList(data->m_playbackSources);
				gsifree(data->m_playbackBuffer);
				gviPS3HeadsetCloseHeadphone(data);
			}

			// Still need to free capture buffer
			gsifree(data->m_captureBuffer);
			return NULL;
		}

		if (!gviPS3HeadsetInitMic(data))
		{
			// Need to free any resources in data for playback
			// Also library needs to close audio port and mic
			// The library still needs to remain
			if(type & GV_PLAYBACK)
			{
				gviFreeSourceList(data->m_playbackSources);
				gsifree(data->m_playbackBuffer);
				gviPS3HeadsetCloseHeadphone(data);
			}
			gviPS3HeadsetCloseMic(data);
			gsifree(data->m_captureBuffer);
			gsifree(data->m_capturePreConvertBuffer);
			return NULL;
		}
	}

	return GVTrue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// initializes a new device
GVDevice gviPS3HeadsetNewDevice(GVDeviceID deviceID, GVDeviceType type)
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
	device = gviNewDevice(deviceID, GVHardwarePS3Headset, type, sizeof(GVIPS3HeadsetData));
	if(!device)
		return NULL;

	// init the device
	result = gviPS3HeadsetInitDevice(device, deviceID, type);
	if(result == GVFalse)
	{
		gviFreeDevice(device);
		return NULL;
	}

	// store the pointers
	device->m_methods.m_freeDevice = gviPS3HeadsetFreeDevice;
	device->m_methods.m_startDevice = gviPS3HeadsetStartDevice;
	device->m_methods.m_stopDevice = gviPS3HeadsetStopDevice;
	device->m_methods.m_isDeviceStarted = gviPS3HeadsetIsDeviceStarted;
	device->m_methods.m_setDeviceVolume = gviPS3HeadsetSetDeviceVolume;
	device->m_methods.m_getDeviceVolume = gviPS3HeadsetGetDeviceVolume;
	device->m_methods.m_setCaptureThreshold = gviPS3HeadsetSetCaptureThreshold;
	device->m_methods.m_getCaptureThreshold = gviPS3HeadsetGetCaptureThreshold;
	device->m_methods.m_getAvailableCaptureBytes = gviPS3HeadsetGetAvailableCaptureBytes;
	device->m_methods.m_capturePacket = gviPS3HeadsetCapturePacket;
	device->m_methods.m_playPacket = gviPS3HeadsetPlayPacket;
	device->m_methods.m_isSourceTalking = gviPS3HeadsetIsSourceTalking;
	device->m_methods.m_listTalkingSources = gviPS3HeadsetListTalkingSources;

	// add it to the list
	gviAppendDeviceToList(GVIDevices, device);
	return device;
}

#endif //!defined(GV_NO_PS3_HEADSET)
