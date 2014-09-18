#include "gvPS2Spu2.h"
#if !defined(GV_NO_PS2_SPU2)
#include "gvDevice.h"
#include "gvCodec.h"
#include "gvSource.h"
#include "gvUtil.h"
#include <libsdr.h>
#include <sif.h>

#if !defined(_PS2)
#error This file should only be used with the PlayStation2
#endif

// Thanks to Martin Jajam at Coresoft for original SPU2 code
////////////////////////////////////////////////////////////

/*
The input block we are writing to takes data in the form

---1st half of buffer-----
256 sample Left  channel  = 512 bytes ( 1 block )
256 sample Right channel  = 512 bytes

---2nd half of buffer-----
256 sample Left  channel  = 512 bytes
256 sample Right channel  = 512 bytes

*/

/************
** DEFINES **
************/
#define bss_align(val) \
 __attribute__ ((aligned(val))) __attribute__ ((section (".bss")))
#define LIMIT(x, minx, maxx)  ((x) < (minx) ? (minx) : ((x) > (maxx) ? (maxx) : (x)))

#define AUTODMA_CH			1

#define SPU_BLOCK_SIZE			 512	// in samples	
#define IOP_BUFF_SIZE		   12288	// 24 blocks
#define SPU_BUFF_SIZE			2048	// 4 blocks

#define CORE0_INPUT_L	(0x2000)// * 2)
#define CORE0_INPUT_R	(0x2200)// * 2)
#define CORE1_INPUT_L	(0x2400)// * 2)
#define CORE1_INPUT_R	(0x2600)// * 2)
#if (AUTODMA_CH == 0)
#define CORE_INPUT_L   CORE0_INPUT_L
#define CORE_INPUT_R   CORE0_INPUT_R
#else
#define CORE_INPUT_L   CORE1_INPUT_L
#define CORE_INPUT_R   CORE1_INPUT_R
#endif

// each DMA is 4 blocks = 256 samples * 2 bytes * 4 blocks = 2048 bytes
#define EEBUFFER_SAMPLE_COUNT (IOP_BUFF_SIZE/4)	// 2 bytes sample

/**********
** TYPES **
**********/
typedef struct
{
	GVBool m_playing;
	GVScalar m_playbackVolume;
	GVFrameStamp m_playbackClock;
	GVISourceList m_playbackSources;
	GVSample * m_playbackBuffer;
	int m_playbackBufferPos;
} GVIPS2Spu2Data;

/************
** GLOBALS **
************/
const GVDeviceID GVPS2Spu2DeviceID = -1;
static GVIDevice * GVIPS2Spu2Device;

static gsi_u16  EEBuffer[IOP_BUFF_SIZE]	bss_align(64);
static int	IOPBuffer			= 0;	
static int	gEEStreanIntr		= 0;	// flag for when an endpoint is reached by the audio play head.
static int  gEEDataSampleCount	= 0;	// how many samples do we have buffered up? ( 0 - 256 )

/**************
** FUNCTIONS **
**************/
static int DMA_EE_To_IOP(int dst, u_char *src, int size)
{
	sceSifDmaData transData;
	int did;

	assert((((gsi_u32)src) & 0x3f) == 0); // assert 64 byte aligned
		
	if (size <= 0) {
		return 0;
	}

	transData.data = (u_int)src;
	transData.addr = (u_int)dst;
	transData.size = (unsigned int)size;
	transData.mode = 0; // caution
	FlushCache(0);

	did = (int)sceSifSetDma( &transData, 1 );

	while (sceSifDmaStat((unsigned int)did) >= 0)
		;

	return size;
}

static int cbEEStreamTransfer( void* common )
{
	gEEStreanIntr = 1;
	GSI_UNUSED(common);
	return 0;
}

static void EEStreamClearSPUBuffer()
{
	memset (EEBuffer, 0, IOP_BUFF_SIZE);
	FlushCache(0);
	DMA_EE_To_IOP(	IOPBuffer,							// IOP side destination address
					(u_char *)EEBuffer,					// EE  side source		address
					IOP_BUFF_SIZE);	

	DMA_EE_To_IOP(	IOPBuffer + IOP_BUFF_SIZE,							// IOP side destination address
					(u_char *)EEBuffer,					// EE  side source		address
					IOP_BUFF_SIZE);	

	// Clear out SPU2 input area
    sceSdRemote(1, rSdVoiceTrans, AUTODMA_CH, SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		 IOPBuffer, CORE_INPUT_L, 0x800 );
    sceSdRemote(1, rSdVoiceTrans, AUTODMA_CH, SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		 IOPBuffer + IOP_BUFF_SIZE, CORE_INPUT_R, 0x800 );

    sceSdRemote (1, rSdVoiceTransStatus, AUTODMA_CH, SD_TRANS_STATUS_WAIT);
}

GVBool gviPS2Spu2Startup(void)
{
	// clear the device pointer
	GVIPS2Spu2Device = NULL;

	// do general initialization
	if(sceSdRemoteInit() != 0)
		return GVFalse;
	if(sceSdRemote(1, rSdInit, SD_INIT_COLD) != 0)
		return GVFalse;
	if(sceSifInitIopHeap() != 0)
		return GVFalse;

	return GVTrue;
}

void gviPS2Spu2Cleanup(void)
{
	// free the device array
	if(GVIPS2Spu2Device)
		gviFreeDevice(GVIPS2Spu2Device);
}

static void	AudioUpSample(gsi_i16* Dest48Khz, const gsi_i16* Src8Khz, int num8Khzsamples)
// 16 bit audio upsample from 8khz to 48khz
// numsamples = Src samples (8khz )
{
#if(0)
	// Point sample
	while (num8Khzsamples)
	{
		int a = Src8Khz[0];


		// bad alias upsampling code here....
		Dest48Khz[0]	= a;
		Dest48Khz[1]	= a;
		Dest48Khz[2]	= a;
		Dest48Khz[3]	= a;
		Dest48Khz[4]	= a;
		Dest48Khz[5]	= a;

		Dest48Khz		+=6;
		Src8Khz			++;
		num8Khzsamples--;
	}
#elif (1)
	// linear interpolate
	int a=0,b=0;
	while (num8Khzsamples - 1)
	{

		a = Src8Khz[0];
		b = Src8Khz[1];


		// bad alias upsampling code here....
		Dest48Khz[0]	= (gsi_i16)a;
		Dest48Khz[1]	= (gsi_i16)((5*a)/6 +(1*b)/6);
		Dest48Khz[2]	= (gsi_i16)((4*a)/6 +(2*b)/6);
		Dest48Khz[3]	= (gsi_i16)((3*a)/6 +(3*b)/6);
		Dest48Khz[4]	= (gsi_i16)((2*a)/6 +(4*b)/6);
		Dest48Khz[5]	= (gsi_i16)((1*a)/6 +(5*b)/6);

		Dest48Khz		+=6;
		Src8Khz			++;
		num8Khzsamples--;
	}
		Dest48Khz[0]	= (gsi_i16)b;
		Dest48Khz[1]	= (gsi_i16)b;
		Dest48Khz[2]	= (gsi_i16)b;
		Dest48Khz[3]	= (gsi_i16)b;
		Dest48Khz[4]	= (gsi_i16)b;
		Dest48Khz[5]	= (gsi_i16)b;

#else

	// use catmull-rom spline to interpolate


    // multiply tr vector by abcd matrix to get point

	float a = (float)Src8Khz[ 0];
	float b = (float)Src8Khz[ 0];
	float c = (float)Src8Khz[ 1];
	float d = (float)Src8Khz[ 2];
	float v;

	// bad alias upsampling code here....
	Dest48Khz[0]	=Src8Khz[ 0];	// t= 0;
	Dest48Khz[1]	=(gsi_u16)(a * -0.058f  + b * 0.938f  + c * 0.132f  + d * -0.012f );			// t= 1/6;
	Dest48Khz[2]	=(gsi_u16)(a * -0.074f  + b * 0.778f  + c * 0.333f  + d * -0.037f );			// t= 2/6;
	Dest48Khz[3]	=(gsi_u16)(a * -0.063f  + b * 0.563f  + c * 0.563f  + d * -0.063f );			// t= 3/6;
	Dest48Khz[4]	=(gsi_u16)(a * -0.037f  + b * 0.333f  + c * 0.778f  + d * -0.074f );			// t= 4/6;
	Dest48Khz[5]	=(gsi_u16)(a * -0.012f  + b * 0.132f  + c * 0.938f  + d * -0.058f );			// t= 5/6;

	Dest48Khz		+=6;
	Src8Khz			++;
	num8Khzsamples-=3;

	while (num8Khzsamples)
	{
		a = (float)Src8Khz[-1];
		b = (float)Src8Khz[ 0];
		c = (float)Src8Khz[ 1];
		d = (float)Src8Khz[ 2];

		// bad alias upsampling code here....
		Dest48Khz[0]	=Src8Khz[ 0];	// t= 0;

		v = (a * -0.058f  + b * 0.938f  + c * 0.132f  + d * -0.012f );
		Dest48Khz[1]	=(gsi_u16)	LIMIT(v,0.0f,65535.0f);		// t= 1/6;
		
		v = (a * -0.074f  + b * 0.778f  + c * 0.333f  + d * -0.037f );	
		Dest48Khz[2]	=(gsi_u16)	LIMIT(v,0.0f,65535.0f);	// t= 2/6;
		
		v = (a * -0.063f  + b * 0.563f  + c * 0.563f  + d * -0.063f );
		Dest48Khz[3]	=(gsi_u16)	LIMIT(v,0.0f,65535.0f);		// t= 3/6;
		
		v= (a * -0.037f  + b * 0.333f  + c * 0.778f  + d * -0.074f );
		Dest48Khz[4]	=(gsi_u16)	LIMIT(v,0.0f,65535.0f);		// t= 4/6;
		
		v=(a * -0.012f  + b * 0.132f  + c * 0.938f  + d * -0.058f );
		Dest48Khz[5]	=(gsi_u16)	LIMIT(v,0.0f,65535.0f);		// t= 5/6;

		Dest48Khz		+=6;
		Src8Khz			++;
		num8Khzsamples--;
	}

	a = (float)Src8Khz[ 0];
	b = (float)Src8Khz[ 0];
	c = (float)Src8Khz[ 1];
	d = (float)Src8Khz[ 1];

	// bad alias upsampling code here....
	Dest48Khz[0]	=Src8Khz[ 0];	// t= 0;
	Dest48Khz[1]	=(gsi_u16)(a * -0.058f  + b * 0.938f  + c * 0.132f  + d * -0.012f );			// t= 1/6;
	Dest48Khz[2]	=(gsi_u16)(a * -0.074f  + b * 0.778f  + c * 0.333f  + d * -0.037f );			// t= 2/6;
	Dest48Khz[3]	=(gsi_u16)(a * -0.063f  + b * 0.563f  + c * 0.563f  + d * -0.063f );			// t= 3/6;
	Dest48Khz[4]	=(gsi_u16)(a * -0.037f  + b * 0.333f  + c * 0.778f  + d * -0.074f );			// t= 4/6;
	Dest48Khz[5]	=(gsi_u16)(a * -0.012f  + b * 0.132f  + c * 0.938f  + d * -0.058f );			// t= 5/6;

	Dest48Khz		+=6;
	Src8Khz			++;

	a = (float)Src8Khz[ 0];
	b = (float)Src8Khz[ 0];
	c = (float)Src8Khz[ 0];
	d = (float)Src8Khz[ 0];

	// bad alias upsampling code here....
	Dest48Khz[0]	=Src8Khz[ 0];	// t= 0;
	Dest48Khz[1]	=(gsi_u16)(a * -0.058f  + b * 0.938f  + c * 0.132f  + d * -0.012f );			// t= 1/6;
	Dest48Khz[2]	=(gsi_u16)(a * -0.074f  + b * 0.778f  + c * 0.333f  + d * -0.037f );			// t= 2/6;
	Dest48Khz[3]	=(gsi_u16)(a * -0.063f  + b * 0.563f  + c * 0.563f  + d * -0.063f );			// t= 3/6;
	Dest48Khz[4]	=(gsi_u16)(a * -0.037f  + b * 0.333f  + c * 0.778f  + d * -0.074f );			// t= 4/6;
	Dest48Khz[5]	=(gsi_u16)(a * -0.012f  + b * 0.132f  + c * 0.938f  + d * -0.058f );			// t= 5/6;

	Dest48Khz		+=6;
	Src8Khz			++;


#endif

}

static int		EEStreamDataPush(const gsi_u16* data, int num8Khzsamples)
// numsamples is in 8khz format
// copies and converts.  returns numsamples read
{
	int remain48Khz = EEBUFFER_SAMPLE_COUNT - gEEDataSampleCount;

	if (num8Khzsamples *6  > remain48Khz)
	{
		num8Khzsamples = remain48Khz/6;
	}

	assert(EEBUFFER_SAMPLE_COUNT >= num8Khzsamples*6);

	AudioUpSample((gsi_i16*)&EEBuffer[gEEDataSampleCount],(const gsi_i16*)data,num8Khzsamples);

	gEEDataSampleCount+=num8Khzsamples*6;

	return num8Khzsamples;
}

static void EEStreamConvertAndTransfer(GVIDevice * device, int which)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;
	GVBool wroteToBuffer;
	int samplesPushed;

static int BlockSize= 256;	// in samples
static int EEMul	= 2;
static int IOPMul	= 4;

	int		iopmem;
	u_char *eemem;
	int i;

	// push any pending data
	if(data->m_playbackBufferPos)
	{
		EEStreamDataPush(data->m_playbackBuffer + data->m_playbackBufferPos, GVISamplesPerFrame - data->m_playbackBufferPos);
		data->m_playbackBufferPos = 0;
	}

	do
	{
		// write a frame of sources to our buffer
		wroteToBuffer = gviWriteSourcesToBuffer(data->m_playbackSources, data->m_playbackClock, data->m_playbackBuffer, 1);

		// clear it if nothing was written
		if(!wroteToBuffer)
			memset(data->m_playbackBuffer, 0, (unsigned int)GVIBytesPerFrame);

		// filter
		if(device->m_playbackFilterCallback)
			device->m_playbackFilterCallback(device, data->m_playbackBuffer, data->m_playbackClock);

		// push it
		samplesPushed = EEStreamDataPush(data->m_playbackBuffer, GVISamplesPerFrame);

		// update the clock
		data->m_playbackClock++;
	}
	while(gEEDataSampleCount < EEBUFFER_SAMPLE_COUNT);

	// store the position at which the push stopped
	if(samplesPushed < GVISamplesPerFrame)
		data->m_playbackBufferPos = samplesPushed;
	else
		data->m_playbackBufferPos = 0;

	gEEDataSampleCount = 0;	// reset

	// Do a memcpy from EE to IOP memory using DMA, respecting 256 sample interleave
	iopmem = IOPBuffer + which * IOP_BUFF_SIZE;
	eemem  = (u_char *)(&EEBuffer[0]);
	for (i=0; i< EEBUFFER_SAMPLE_COUNT;i+= BlockSize)
	{		
		//left
		DMA_EE_To_IOP(	iopmem,				// IOP side destination address
						eemem,				// EE  side source		address
						BlockSize*2);	

		//right
		DMA_EE_To_IOP(	iopmem+BlockSize*2,				// IOP side destination address
						eemem,				// EE  side source		address
						BlockSize*2);	

		iopmem  += BlockSize * IOPMul;		// 4 ,2,
		eemem	+= BlockSize * EEMul; // 2 ,2, 
	}
}

void gviPS2Spu2Think(void)
{
	GVIPS2Spu2Data * data;
	int v;
	int which;

	if(!GVIPS2Spu2Device)
		return;

	// get the data
	data = (GVIPS2Spu2Data *)GVIPS2Spu2Device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return;

	// Check if an interrupt has occured.  IF this were a separate thread, it could sleep until this happens.
	if(!gEEStreanIntr)
		return;

	// IOP_BUFF_SIZE has been played
	// clear flag
	gEEStreanIntr = 0;

	v	= sceSdRemote(1, rSdBlockTransStatus ,AUTODMA_CH,0);

	which = 1 - (v >>24);

	// done transfer, send some more data
	EEStreamConvertAndTransfer(GVIPS2Spu2Device, which);
}

int gviPS2Spu2ListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types)
{
	// check for no room
	if(maxDevices < 1)
		return 0;

	// check for no playback devices wanted
	if(!(types & GV_PLAYBACK))
		return 0;

	// store this device's info in the array
	devices[0].m_id = GVPS2Spu2DeviceID;
	strcpy(devices[0].m_name, "System Sound");
	devices[0].m_deviceType = GV_PLAYBACK;
	devices[0].m_defaultDevice = (GVDeviceType)0; // ps2 doesn't support default devices
	devices[0].m_hardwareType = GVHardwarePS2Spu2;

	return 1;
}

static GVBool gviPS2Spu2StartDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	if(!(type & GV_PLAYBACK))
		return GVFalse;

	// clear the clock
	data->m_playbackClock = 0;

	// started playing
	data->m_playing = GVTrue;

	return GVTrue;
}

static void gviPS2Spu2StopDevice(GVIDevice * device, GVDeviceType type)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	if(!(type & GV_PLAYBACK))
		return;

	// stopped playing
	data->m_playing = GVFalse;

	// clear any pending sources & buffers
	gviClearSourceList(data->m_playbackSources);

	// clear the SPU
	EEStreamClearSPUBuffer();
}

static GVBool gviPS2Spu2IsDeviceStarted(GVIDevice * device, GVDeviceType type)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	if(type == GV_PLAYBACK)
		return data->m_playing;
	return GVFalse;
}

static void gviPS2Spu2SetDeviceVolume(GVIDevice * device, GVDeviceType type, GVScalar volume)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;
	gsi_i16 sVol;

	if(!(type & GV_PLAYBACK))
		return;

	// store the volume
	data->m_playbackVolume = volume;

	// convert it into a signed short
	sVol = (gsi_i16)(volume * 0x3FFF);
    sceSdRemote(1, rSdSetParam, AUTODMA_CH | SD_P_BVOLL, sVol);
    sceSdRemote(1, rSdSetParam, AUTODMA_CH | SD_P_BVOLR, sVol);
}

static GVScalar gviPS2Spu2GetDeviceVolume(GVIDevice * device, GVDeviceType type)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	if(!(type & GV_PLAYBACK))
		return 0;

	return data->m_playbackVolume;
}

static void gviPS2Spu2PlayPacket(GVIDevice * device, const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return;

	//add it
	gviAddPacketToSourceList(data->m_playbackSources, packet, len, source, frameStamp, mute, data->m_playbackClock);
}

static GVBool gviPS2Spu2IsSourceTalking(GVDevice device, GVSource source)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviIsSourceTalking(data->m_playbackSources, source);
}

static int gviPS2Spu2ListTalkingSources(GVDevice device, GVSource sources[], int maxSources)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	// don't do anything if we're not playing
	if(!data->m_playing)
		return GVFalse;

	return gviListTalkingSources(data->m_playbackSources, sources, maxSources);
}

static void gviPS2Spu2FreeDevice(GVIDevice * device)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	EEStreamClearSPUBuffer();

	// Free resources
    sceSifFreeIopHeap((void *)IOPBuffer);

	IOPBuffer		= (int)NULL;

	// Turn off interrupt
	sceSdRemoteCallbackQuit();
	sceSdRemote(1, rSdSetTransIntrHandler,AUTODMA_CH,NULL, NULL);

	// kill volume
    //changeInputVolume(0x0000);

	if(data->m_playbackSources)
		gviFreeSourceList(data->m_playbackSources);
	gsifree(data->m_playbackBuffer);

	// free the device
	gviFreeDevice(device);

	GVIPS2Spu2Device = NULL;
}

static GVBool gviPS2Spu2InitDevice(GVIDevice * device, int deviceIndex, GVDeviceType type)
{
	GVIPS2Spu2Data * data = (GVIPS2Spu2Data *)device->m_data;

	// create the array of sources
	data->m_playbackSources = gviNewSourceList();
	if(!data->m_playbackSources)
		return GVFalse;

	// allocate the buffer
	data->m_playbackBuffer = (GVSample *)gsimalloc((unsigned int)GVIBytesPerFrame);
	if(!data->m_playbackBuffer)
	{
		gviFreeSourceList(data->m_playbackSources);
		return GVFalse;
	}

	// these are the buffers which the audio (SPU) will stream from on the IOP.
	// they are double buffered, with one half being read while the other is dma'ed into from the EE
	IOPBuffer		= (int)sceSifAllocIopHeap(IOP_BUFF_SIZE*2);
	assert(IOPBuffer	);

	// Set interrupt to receive mid and end point in buffer
	sceSdRemoteCallbackInit(5);
	sceSdRemote(1, rSdSetTransIntrHandler,AUTODMA_CH,cbEEStreamTransfer, NULL);


	// clear interrupt flag.  After this each interrupt, this flag will be set to 1.
	gEEStreanIntr = 0;	

	EEStreamClearSPUBuffer();


	// Turn on the auto dma transfer process.  From here in the dma continuously
	// load data into the sound processor memory, playing it.
	// at mid and end points, cbEEStreamTransfer will be called

	sceSdRemote(1, rSdBlockTrans,	AUTODMA_CH,
    								(SD_TRANS_MODE_WRITE| SD_BLOCK_LOOP),
		 							IOPBuffer,				// start of buffer
									IOP_BUFF_SIZE*2,		// size of buffer
									IOPBuffer				// where in buffer to start
				);

    sceSdRemote(1, rSdSetParam, AUTODMA_CH | SD_P_BVOLL, 0x3FFF);
    sceSdRemote(1, rSdSetParam, AUTODMA_CH | SD_P_BVOLR, 0x3FFF);
    sceSdRemote(1, rSdSetParam, AUTODMA_CH | SD_P_MVOLL, 0x3FFF);
    sceSdRemote(1, rSdSetParam, AUTODMA_CH | SD_P_MVOLR, 0x3FFF);

	// set data
	data->m_playbackVolume = 1.0;
	GSI_UNUSED(deviceIndex);
	GSI_UNUSED(type);
	return GVTrue;
}

GVDevice gviPS2Spu2NewDevice(GVDeviceID deviceID, GVDeviceType type)
{
	GVIDevice * device;
	GVBool result;

	// check if the device already exists
	if(GVIPS2Spu2Device)
		return NULL;

	// check the ID
	if(deviceID != GVPS2Spu2DeviceID)
		return NULL;

	// check the type
	if(type != GV_PLAYBACK)
		return NULL;

	// create a new device
	device = gviNewDevice(deviceID, GVHardwarePS2Spu2, type, sizeof(GVIPS2Spu2Data));
	if(!device)
		return NULL;

	// init the device
	result = gviPS2Spu2InitDevice(device, deviceID, type);
	if(result == GVFalse)
	{
		gviFreeDevice(device);
		return NULL;
	}

	// store the pointers
	device->m_methods.m_freeDevice = gviPS2Spu2FreeDevice;
	device->m_methods.m_startDevice = gviPS2Spu2StartDevice;
	device->m_methods.m_stopDevice = gviPS2Spu2StopDevice;
	device->m_methods.m_isDeviceStarted = gviPS2Spu2IsDeviceStarted;
	device->m_methods.m_setDeviceVolume = gviPS2Spu2SetDeviceVolume;
	device->m_methods.m_getDeviceVolume = gviPS2Spu2GetDeviceVolume;
	device->m_methods.m_playPacket = gviPS2Spu2PlayPacket;
	device->m_methods.m_isSourceTalking = gviPS2Spu2IsSourceTalking;
	device->m_methods.m_listTalkingSources = gviPS2Spu2ListTalkingSources;

	// store a pointer to the device
	GVIPS2Spu2Device = device;

	return device;
}

#endif  //!defined(GV_NO_PS2_SPU2)
