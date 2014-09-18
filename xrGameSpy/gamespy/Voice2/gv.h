/*
GameSpy Voice2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2004 GameSpy Industries, Inc

devsupport@gamespy.com
http://gamespy.net
*/

/************************
** GameSpy Voice 2 SDK **
************************/

#ifndef _GV_H_
#define _GV_H_

#include "../common/gsCommon.h"

#if defined(__cplusplus)
extern "C" {
#endif

//DEFINES
/////////
#define GV_BYTES_PER_SAMPLE          (sizeof(GVSample) / sizeof(GVByte))
#define GV_BITS_PER_SAMPLE           (GV_BYTES_PER_SAMPLE * 8)

#define GV_DEVICE_NAME_LEN   64
#define GV_CHANNEL_NAME_LEN  64

#define GV_CAPTURE   1
#define GV_PLAYBACK  2
#define GV_CAPTURE_AND_PLAYBACK  (GV_CAPTURE|GV_PLAYBACK)

#define GVRate_8KHz   8000
#define GVRate_16KHz 16000
//Used by PSP
#define GVRate_11KHz 11025

//For backwards compatability
#define GV_SAMPLES_PER_SECOND    (gvGetSampleRate())
#define GV_BYTES_PER_SECOND      (gvGetSampleRate() * GV_BYTES_PER_SAMPLE)

//TYPES
///////
typedef enum
{
	GVCodecRaw,
	GVCodecSuperHighQuality,
	GVCodecHighQuality,
	GVCodecAverage,
	GVCodecLowBandwidth,
	GVCodecSuperLowBandwidth
} GVCodec;

typedef enum
{
	GVHardwareDirectSound,   // Win32
	GVHardwarePS2Spu2,       // PS2 (System output)
	GVHardwarePS2Headset,    // PS2 (USB)
	GVHardwarePS2Microphone, // PS2 (USB)
	GVHardwarePS2Speakers,   // PS2 (USB)
	GVHardwarePS2Eyetoy,     // PS2 (USB)
	GVHardwarePS3Headset,    // PS3 (USB)
	GVHardwarePSPHeadset,    // PSP
	GVHardwareMacOSX,        // MacOSX
	GVHardwareCustom         // Any
} GVHardwareType;

typedef enum
{
	GVCaptureModeThreshold,
	GVCaptureModePushToTalk
} GVCaptureMode;

typedef int GVBool;
#define GVFalse 0
#define GVTrue 1

typedef gsi_u8                 GVByte;
typedef gsi_i16                GVSample;

typedef int                    GVRate;

#if defined(_PSP) || defined(_PS2) || defined(_PS3)
	typedef float              GVScalar;  // range 0-1
#else
	typedef double             GVScalar;  // range 0-1
#endif
typedef gsi_u16                GVFrameStamp;
typedef void *                 GVDecoderData;
typedef int                    GVDeviceType;
typedef struct GVIDevice *     GVDevice;

#if defined(GV_CUSTOM_SOURCE_TYPE)
typedef GV_CUSTOM_SOURCE_TYPE  GVSource;
#else
typedef int                    GVSource;
#endif

#if defined(_WIN32)
typedef GUID                   GVDeviceID;
#else
typedef int                    GVDeviceID;
#endif

typedef struct
{
	GVDeviceID m_id;
	gsi_char m_name[GV_DEVICE_NAME_LEN];
	GVDeviceType m_deviceType;
	GVDeviceType m_defaultDevice;    // not supported on PS2
	GVHardwareType m_hardwareType;
} GVDeviceInfo;

typedef struct
{
	int m_samplesPerFrame;  // number of samples in an unencoded frame
	int m_encodedFrameSize; // number of bytes in an encoded frame

	GVBool (* m_newDecoderCallback)(GVDecoderData * data);
	void (* m_freeDecoderCallback)(GVDecoderData data);

	void (* m_encodeCallback)(GVByte * out, const GVSample * in);
	void (* m_decodeAddCallback)(GVSample * out, const GVByte * in, GVDecoderData data);  // adds to output (required)
	void (* m_decodeSetCallback)(GVSample * out, const GVByte * in, GVDecoderData data);  // sets output (optional)
} GVCustomCodecInfo;

//GLOBALS
/////////
#if defined(_WIN32)
extern const GVDeviceID GVDefaultCaptureDeviceID;
extern const GVDeviceID GVDefaultPlaybackDeviceID;
#elif defined(_PS2)
extern const GVDeviceID GVPS2Spu2DeviceID;
#endif

//GENERAL
/////////
#if defined(_WIN32)
GVBool gvStartup(HWND hWnd);
#else
GVBool gvStartup(void);
#endif
void gvCleanup(void);

void gvThink(void);

//CODEC
///////
GVBool gvSetCodec(GVCodec codec);
void gvSetCustomCodec(GVCustomCodecInfo * info);

void gvGetCodecInfo(int * samplesPerFrame, int * encodedFrameSize, int * bitsPerSecond);

//SAMPLE RATE
/////////////
void gvSetSampleRate(GVRate sampleRate);
GVRate gvGetSampleRate(void);

//DEVICES
/////////
int gvListDevices(GVDeviceInfo devices[], int maxDevices, GVDeviceType types);

#if defined(_WIN32)
GVBool gvRunSetupWizard(GVDeviceID captureDeviceID, GVDeviceID playbackDeviceID);
GVBool gvAreDevicesSetup(GVDeviceID captureDeviceID, GVDeviceID playbackDeviceID);
#endif

GVDevice gvNewDevice(GVDeviceID deviceID, GVDeviceType type);
void gvFreeDevice(GVDevice device);

GVBool gvStartDevice(GVDevice device, GVDeviceType type);
void gvStopDevice(GVDevice device, GVDeviceType type);
GVBool gvIsDeviceStarted(GVDevice device, GVDeviceType type);

void gvSetDeviceVolume(GVDevice device, GVDeviceType type, GVScalar volume);
GVScalar gvGetDeviceVolume(GVDevice device, GVDeviceType type);

typedef void (* gvUnpluggedCallback)(GVDevice device);
void gvSetUnpluggedCallback(GVDevice device, gvUnpluggedCallback unpluggedCallback);

typedef void (* gvFilterCallback)(GVDevice device, GVSample * audio, GVFrameStamp frameStamp);
void gvSetFilter(GVDevice device, GVDeviceType type, gvFilterCallback callback);

//CAPTURE
/////////
// len is both an input and output parameter
GVBool gvCapturePacket(GVDevice device, GVByte * packet, int * len, GVFrameStamp * frameStamp, GVScalar * volume);
int gvGetAvailableCaptureBytes(GVDevice device);

void gvSetCaptureThreshold(GVDevice device, GVScalar threshold);
GVScalar gvGetCaptureThreshold(GVDevice device);

void gvSetCaptureMode(GVDevice device, GVCaptureMode captureMode);
GVCaptureMode gvGetCaptureMode(GVDevice device);

void gvSetPushToTalk(GVDevice device, GVBool talkOn);
GVBool gvGetPushToTalk(GVDevice device);

//PLAYBACK
//////////
void gvPlayPacket(GVDevice device, const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute);

GVBool gvIsSourceTalking(GVDevice device, GVSource source);
int gvListTalkingSources(GVDevice device, GVSource sources[], int maxSources);

void gvSetGlobalMute(GVBool mute);
GVBool gvGetGlobalMute(void);

//CUSTOM DEVICE
///////////////
GVDevice gvNewCustomDevice(GVDeviceType type);

// for both of these, numSamples must be a multiple of the codec's samplesPerFrame
// this ensures that no data needs to be buffered by the SDK
GVBool gvGetCustomPlaybackAudio(GVDevice device, GVSample * audio, int numSamples);
GVBool gvSetCustomCaptureAudio(GVDevice device, const GVSample * audio, int numSamples,
                               GVByte * packet, int * packetLen, GVFrameStamp * frameStamp, GVScalar * volume);

//CHANNELS
//////////
int gvGetNumChannels(GVDevice device, GVDeviceType type);
void gvGetChannelName(GVDevice device, GVDeviceType type, int channel, gsi_char name[GV_CHANNEL_NAME_LEN]);
void gvSetChannel(GVDevice device, GVDeviceType type, int channel);
int gvGetChannel(GVDevice device, GVDeviceType type);

#if defined(__cplusplus)
}
#endif

#endif
