#include "../gv.h"
#include <stdio.h>
#include <math.h>
#if defined(_WIN32)
#include <conio.h>
#elif defined(_PS2)
#include "../../common/ps2/ps2pad.h"
#elif defined(_PS3)
//#define USER_CREATED_SPURS_INSTANCE
#include <cell/pad.h>
#if defined(USER_CREATED_SPURS_INSTANCE)
#include "spursConfiguration.h"
#include <sys/spu_initialize.h>
int iReturn, iNumSpus, ppuThreadPriority, spuThreadPriority;
bool exitIfNoWork;
CellSpurs* myCellSpurs;
uint8_t auiLocalPriorities[8]={1,1,1,1,1,1,1,1};
#endif
#endif

#define NET_PORT              55123
#define MAX_DEVICES           10
#define LOCAL_ECHO            1
#define SEND_PACKETS          0
#define CUSTOM_CODEC          0
#define CUSTOM_CODEC_SAMPLES_PER_FRAME  100
#define SHOW_TALKERS          0
#define SHOW_VOLUME           0
#define SHOW_FRAMESTAMPS      0
#define CAPTURE_THRESHOLD     0.05
#define CAPTURE_FILTER        0
//#define CODEC                 GVCodecRaw
//#define CODEC                 GVCodecSuperHighQuality
//#define CODEC                 GVCodecHighQuality
#define CODEC                 GVCodecAverage
//#define CODEC                 GVCodecLowBandwidth
//#define CODEC                 GVCodecSuperLowBandwidth

GVDevice capDevice;
GVDevice playDevice;

SOCKET sock = INVALID_SOCKET;
SOCKADDR_IN sendAddr;
SOCKADDR_IN localSource;

int SamplesPerFrame;
int BytesPerFrame;
int EncodedFrameSize;
int BitsPerSecond;

static GVBool CheckInput(void);

#ifdef _PS3
gsi_bool controllerConnected = gsi_false;
#endif

#if CUSTOM_CODEC
static void CustomEncodeCallback(GVByte * out, const GVSample * in)
{
	GVSample * sampleOut = (GVSample *)out;
	int i;

	for(i = 0 ; i < SamplesPerFrame ; i++)
		sampleOut[i] = htons(in[i]);
}

static void CustomDecodeAddCallback(GVSample * out, const GVByte * in, GVDecoderData data)
{
	const GVSample * sampleIn = (const GVSample *)in;
	int i;

	for(i = 0 ; i < SamplesPerFrame ; i++)
		out[i] += ntohs(sampleIn[i]);
}

static void CustomDecodeSetCallback(GVSample * out, const GVByte * in, GVDecoderData data)
{
	const GVSample * sampleIn = (const GVSample *)in;
	int i;

	for(i = 0 ; i < SamplesPerFrame ; i++)
		out[i] = ntohs(sampleIn[i]);
}
#endif

static void PlayPacket(const GVByte * packet, int len, GVSource source)
{
	GVFrameStamp frameStamp;

	if(!playDevice)
		return;

	if((len % EncodedFrameSize) != sizeof(GVFrameStamp))
		return;

	// get out the frameStamp
	len -= sizeof(GVFrameStamp);
	memcpy(&frameStamp, packet + len, sizeof(GVFrameStamp));
	frameStamp = ntohs(frameStamp);
#if SHOW_FRAMESTAMPS
	printf("Playing frameStamp %d\n", frameStamp);
#endif

	// queue it
	gvPlayPacket(playDevice, packet, len, source, frameStamp, 0);
}

static void HandleCapturedPacket(const GVByte * packet, int len, GVFrameStamp frameStamp, GVScalar volume)
{
#if SHOW_VOLUME
	printf("%.00f%%\n", (float)(volume * 100));
#endif

#if LOCAL_ECHO
	PlayPacket(packet, len, localSource);
#endif

#if SEND_PACKETS
	sendto(sock, (char *)packet, len, 0, (SOCKADDR *)&sendAddr, sizeof(SOCKADDR_IN));
#endif

	GSI_UNUSED(packet);
	GSI_UNUSED(len);
	GSI_UNUSED(frameStamp);
	GSI_UNUSED(volume);
}

static void UnpluggedCallback(GVDevice device)
{
	printf("Halting - device unplugged\n");
	while(CheckInput())
		msleep(10);
	exit(1);
	
	GSI_UNUSED(device);
}

static GVBool Initialize(const char * remoteIP)
{
	SOCKADDR_IN localAddr;
	GVBool result;
#ifdef _PS3
	int32_t ret;
#endif	
#if CUSTOM_CODEC
	GVCustomCodecInfo customCodecInfo;
#else
	GVCodec codec;
#endif

#if defined(_PS2)
	if(!PadInit())
	{
		printf("Failed to initialize the controller\n");
		return GVFalse;
	}
#elif defined(_PS3)
	// Intialize one controller for testing purposes
	ret = cellPadInit(1);
	if (ret != CELL_OK)
	{
		printf ("Failed to initialize cell pad 0x%08x\n", ret);
		return(GVFalse);
	}
#endif

	SocketStartUp();

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock == INVALID_SOCKET)
	{
		printf("Failed to create socket\n");
		return GVFalse;
	}
	if(SetSockBlocking(sock, 0) == 0)
	{
		printf("Failed to set socket to non-blocking\n");
		return GVFalse;
	}

	memset(&localAddr, 0, sizeof(SOCKADDR_IN));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(NET_PORT);
	if(gsiSocketIsError(bind(sock, (SOCKADDR *)&localAddr, sizeof(SOCKADDR_IN))))
	{
		printf("Failed to bind socket\n");
		return GVFalse;
	}

	memset(&sendAddr, 0, sizeof(SOCKADDR_IN));
	sendAddr.sin_family = AF_INET;
	sendAddr.sin_addr.s_addr = inet_addr(remoteIP);
	sendAddr.sin_port = htons(NET_PORT);

#if SEND_PACKETS
	printf("Sending to %s:%d\n", remoteIP, NET_PORT);
#endif

#if defined(_PS3) && defined(USER_CREATED_SPURS_INSTANCE)
	
	iNumSpus = 1;
	spuThreadPriority = 200;
	ppuThreadPriority = 1000;
	exitIfNoWork = false;
	myCellSpurs = (CellSpurs*) gsimemalign(128, sizeof(CellSpurs));

	// initializing spus themselves before using spurs
	sys_spu_initialize(iNumSpus,0);

	iReturn=cellSpursInitialize(myCellSpurs, iNumSpus,spuThreadPriority,ppuThreadPriority,exitIfNoWork);
	if (iReturn!=CELL_OK)
	{
		printf("Error initializing spurs\n");
		return GVFalse;
	}
	spursConfiguration_initWithSpurs(myCellSpurs, iNumSpus,auiLocalPriorities);
#endif 

#if defined(_WIN32)
	result = gvStartup(NULL);
#else
	result = gvStartup();
#endif
	if(!result)
	{
		printf("Startup failed\n");
		return GVFalse;
	}
	printf("Started\n");

#if CUSTOM_CODEC
	customCodecInfo.m_samplesPerFrame = CUSTOM_CODEC_SAMPLES_PER_FRAME;
	customCodecInfo.m_encodedFrameSize = (CUSTOM_CODEC_SAMPLES_PER_FRAME * GV_BYTES_PER_SAMPLE);
	customCodecInfo.m_newDecoderCallback = NULL;
	customCodecInfo.m_freeDecoderCallback = NULL;
	customCodecInfo.m_encodeCallback = CustomEncodeCallback;
	customCodecInfo.m_decodeAddCallback = CustomDecodeAddCallback;
	customCodecInfo.m_decodeSetCallback = CustomDecodeSetCallback;
	gvSetCustomCodec(&customCodecInfo);
#else
	codec = CODEC;
#if defined(_PS3) && defined(_WIN32)
	gvSetSampleRate(GVRate_16KHz);
#elif defined(_PSP)
	gvSetSampleRate(GVRate_11KHz);
#else
	gvSetSampleRate(GVRate_8KHz);
#endif
	if(!gvSetCodec(codec))
	{
		printf("Failed to set the codec\n");
		return GVFalse;
	}
#endif

	gvGetCodecInfo(&SamplesPerFrame, &EncodedFrameSize, &BitsPerSecond);
	BytesPerFrame = (SamplesPerFrame * GV_BYTES_PER_SAMPLE);

	printf("SamplesPerFrame=%d EncodedFrameSize=%d BitsPerSecond=%d\n",
		SamplesPerFrame, EncodedFrameSize, BitsPerSecond);

	return GVTrue;
}

void Destroy()
{
#ifdef _PS3
	int32_t ret;
	ret = cellPadEnd();
	if (ret != CELL_OK)
	{
		printf ("Failed to destroy cell pad 0x%08x\n", ret);
	}
#endif
	playDevice = NULL;
	capDevice = NULL;
	closesocket(sock);
	gvCleanup();

#if defined(_PS3) && defined(USER_CREATED_SPURS_INSTANCE)
	cellSpursFinalize(myCellSpurs);
	gsifree(myCellSpurs);
#endif
}

#if CAPTURE_FILTER
#define M_2PI 6.283185307179586476925286766559
static void Filter(GVDevice device, GVSample * audio, GVFrameStamp frameStamp)
{
	int i;
	double t;
	double v;
	for(i = 0 ; i < SamplesPerFrame ; i++)
	{
		t = ((double)i / SamplesPerFrame);
		v = sin(t * M_2PI);
		audio[i] = (GVSample)(audio[i] * v);
	}
}
#endif

static void ListChannels(GVDevice device, GVDeviceType type)
{
	gsi_char name[GV_CHANNEL_NAME_LEN];
	int num;
	int i;

	num = gvGetNumChannels(device, type);
	if(num < 2)
		return;

	printf("\tChannels:\n");

	for(i = 0 ; i < num ; i++)
	{
		gvGetChannelName(device, type, i, name);
		_tprintf(_T("\t\t%d: %s\n"), i, name);
	}
}

#if defined(_PSP)
#include <libwave.h>
#define PSP_STEREO 1
#if PSP_STEREO
	#define CHANNELS 2
	#define PSP_WAVE_FORMAT SCE_WAVE_AUDIO_FMT_S16_STEREO
#else
	#define CHANNELS 1
	#define PSP_WAVE_FORMAT SCE_WAVE_AUDIO_FMT_S16_MONO
#endif
#define PSP_CHANNEL 0
#define PSP_BUFFER_SIZE 640
#define PSP_TEMP_BUFFER_SIZE 160
#define PSP_NUM_BUFFERS 2
#define PSP_VOLUME 0x8000
static int PSPBufferIndex = 0;
static GVSample PSPBuffer[PSP_NUM_BUFFERS][PSP_BUFFER_SIZE * CHANNELS];
static GVSample PSPTempBuffer[PSP_TEMP_BUFFER_SIZE];
static SceUID PSPPlaybackThreadID;

static int PSPPlaybackThread(SceSize args, void * argp)
{
	int rcode;
	int i;

	while(1)
	{
		// get the playback audio
		gvGetCustomPlaybackAudio(playDevice, PSPTempBuffer, PSP_TEMP_BUFFER_SIZE);

		// upsample from 11khz to 44khz
		// also interleave for stereo
		for(i = 0; i < PSP_TEMP_BUFFER_SIZE ; i++)
		{
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS]   = PSPTempBuffer[i];
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS+1] = PSPTempBuffer[i];
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS+2] = PSPTempBuffer[i];
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS+3] = PSPTempBuffer[i];
#if PSP_STEREO
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS+4] = PSPTempBuffer[i];
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS+5] = PSPTempBuffer[i];
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS+6] = PSPTempBuffer[i];
			PSPBuffer[PSPBufferIndex][i*4*CHANNELS+7] = PSPTempBuffer[i];
#endif
		}

		// write it
		rcode = sceWaveAudioWriteBlocking(PSP_CHANNEL, PSP_VOLUME, PSP_VOLUME, PSPBuffer[PSPBufferIndex]);
		if(rcode < 0)
		{
			printf("sceWaveAudioWrite Error = %X\n", rcode);
			break;
		}

		// move the index
		PSPBufferIndex++;
		PSPBufferIndex %= PSP_NUM_BUFFERS;
	}

	// wait for playback to finish
	sceWaveAudioWriteBlocking(PSP_CHANNEL, PSP_VOLUME, PSP_VOLUME, NULL);

	sceKernelExitThread(0);

	GSI_UNUSED(args);
	GSI_UNUSED(argp);

	return 0;
}

static GVBool PSPStartup(void)
{
	int rcode;
	GVBool result;

	rcode = sceWaveInit();
	if(rcode != SCE_OK)
	{
		printf("sceWaveInit = %X\n", rcode);
		return GVFalse;
	}

	rcode = sceWaveAudioSetSample(PSP_CHANNEL, PSP_BUFFER_SIZE);
	if(rcode != SCE_OK)
	{
		printf("sceWaveAudioSetSample = %X\n", rcode);
		return GVFalse;
	}

	rcode = sceWaveAudioSetFormat(PSP_CHANNEL, PSP_WAVE_FORMAT);
	if(rcode != SCE_OK)
	{
		printf("sceWaveAudioSetFormat = %X\n", rcode);
		return GVFalse;
	}

	playDevice = gvNewCustomDevice(GV_PLAYBACK);
	if(!playDevice)
	{
		printf("gvNewCustomDevice failed\n");
		return GVFalse;
	}

	result = gvStartDevice(playDevice, GV_PLAYBACK);
	if(!result)
	{
		printf("gvStartDevice failed\n");
		return GVFalse;
	}

	// create playback thread
	PSPPlaybackThreadID = sceKernelCreateThread("playback",PSPPlaybackThread,
		SCE_KERNEL_USER_HIGHEST_PRIORITY, 4096, 0, NULL);
	if(PSPPlaybackThreadID < 0)
	{
		printf("Error creating playback thread\n");
		return GVFalse;
	}

	// start playback thread
	rcode = sceKernelStartThread(PSPPlaybackThreadID, 0, NULL);
	if(rcode < 0)
	{
		printf("Error starting playback thread\n");
		return GVFalse;
	}

	return GVTrue;
}
#endif

static void Startup(void)
{
	int num;
	GVDeviceInfo devices[MAX_DEVICES];
	GVDeviceInfo * info;
	GVDevice device;
	GVBool pd, cd, dpd, dcd;
	GVDeviceType types;
	const char * typesString;
	int i;
	GVBool result;
	const char * hardwareType;

#if defined(_PS2) || defined(_PS3)
	printf("Waiting for devices to be detected...\n");
	msleep(2000);
#endif

#if defined(_PSP)
	PSPStartup();
#endif

	printf("Checking for devices...\n");
	do
	{
		num = gvListDevices(devices, MAX_DEVICES, GV_CAPTURE_AND_PLAYBACK);
		msleep(10);
	}
	while(num == 0);
	printf("Found %d devices\n", num);

	for(i = 0 ; i < num ; i++)
	{
		info = &devices[i];
		pd = (info->m_deviceType & GV_PLAYBACK);
		cd = (info->m_deviceType & GV_CAPTURE);
		dpd = (info->m_defaultDevice & GV_PLAYBACK);
		dcd = (info->m_defaultDevice & GV_CAPTURE);
		if(info->m_hardwareType == GVHardwareDirectSound)
			hardwareType = "DirectSound";
		else if(info->m_hardwareType == GVHardwarePS2Spu2)
			hardwareType = "SPU2";
		else if(info->m_hardwareType == GVHardwarePS2Headset)
			hardwareType = "Headset";
		else if(info->m_hardwareType == GVHardwarePS2Microphone)
			hardwareType = "Microphone";
		else if(info->m_hardwareType == GVHardwarePS2Speakers)
			hardwareType = "Speakers";
		else if(info->m_hardwareType == GVHardwarePS2Eyetoy)
			hardwareType = "Eyetoy";
		else if(info->m_hardwareType == GVHardwarePSPHeadset)
			hardwareType = "Headset";
		else if(info->m_hardwareType == GVHardwarePS3Headset)
			hardwareType = "Headset";
		else if(info->m_hardwareType == GVHardwareMacOSX)
			hardwareType = "MacOS X";
		else
			hardwareType = "Unknown";

		_tprintf(_T("%d: %s"),
			i, info->m_name);
		printf(" (%s)\n\tPlayback: %s\n\tCapture: %s\n",
			hardwareType,
			pd?(dpd?"Default":"Yes"):"No",
			cd?(dcd?"Default":"Yes"):"No");

#if defined(_PS2) || defined(_PS3)

		if (!capDevice && cd)
			dcd = cd;
		if (!playDevice && pd)
			dpd = pd;

		if((pd && !playDevice) || (cd && !capDevice))
#else
		if(dpd || dcd)
#endif
		{ 
			types = 0;
			if(dpd && !playDevice)
				types |= GV_PLAYBACK;
			if(dcd && !capDevice)
				types |= GV_CAPTURE;

			if(types == GV_CAPTURE)
				typesString = "capture";
			else if(types == GV_PLAYBACK)
				typesString = "playback";
			else
				typesString = "capture & playback";

			printf("\tUsing Types: %s\n", typesString);

			device = gvNewDevice(devices[i].m_id, types);
			printf("\tCreation: %s\n", device?"succeeded":"failed");
			if(!device)
				continue;

			gvSetUnpluggedCallback(device, UnpluggedCallback);

			if(types & GV_CAPTURE)
			{
				capDevice = device;
				gvSetCaptureThreshold(device, CAPTURE_THRESHOLD);
#if CAPTURE_FILTER
				gvSetFilter(capDevice, GV_CAPTURE, Filter);
#endif
				ListChannels(device, GV_CAPTURE);
			}
			if(types & GV_PLAYBACK)
			{
				playDevice = device;
				ListChannels(device, GV_PLAYBACK);
			}

			result = gvStartDevice(device, types);
			printf("\tStarting: %s\n", result?"succeeded":"failed");
		}
	}

#if defined(_WIN32)
	printf("Press Q to quit\n");
#elif defined(_PS2)
	printf("Press X to quit\n");
#endif
}

static void ShowInfo(void)
{
#if SHOW_TALKERS
	static gsi_time lastPrintTime;
	gsi_time now;
	GVSource sources[10];
	int num;
	int i;

	if(!playDevice)
		return;

	// check if we should show info
	now = current_time();
	if((now - lastPrintTime) > 1000)
	{
		lastPrintTime = now;

		// see who's talking
		num = gvListTalkingSources(playDevice, sources, sizeof(sources) / sizeof(sources[0]));
		printf("Talking sources:");
		for(i = 0 ; i < num ; i++)
			printf(" %s:%d", inet_ntoa(sources[i].sin_addr), ntohs(sources[i].sin_port));
		printf("\n");
	}
#endif
}

GVByte packet[1024] POST_ALIGN(128);
static void Think(void)
{
	GVBool result;
	int rcode;
	
	int packetLen;
	GVFrameStamp frameStamp;
	SOCKADDR_IN fromAddr;
	int fromLen;
	GVScalar volume;

	memset(packet, 0, sizeof(packet));

	// handle incoming packets
	do
	{
		fromLen = sizeof(SOCKADDR_IN);

		rcode = recvfrom(sock, (char *)packet, sizeof(packet), 0, (SOCKADDR *)&fromAddr, &fromLen);
		if(rcode > 0)
			PlayPacket(packet, rcode, fromAddr);
		else if(rcode < 0)
		{
			int error = GOAGetLastError(sock);
			if(error != WSAEWOULDBLOCK)
				printf("recv error: %d\n", error);
		}
	}
	while(rcode > 0);

	// let gv process the packets
	gvThink();

	// check for captured packets
	if(gvIsDeviceStarted(capDevice, GV_CAPTURE))
	{
		do
		{
			packetLen = (sizeof(packet) - sizeof(GVFrameStamp));
			memset(packet, 0, sizeof(packet));
			result = gvCapturePacket(capDevice, packet, &packetLen, &frameStamp, &volume);
			if(result)
			{
				// handle the capture packet
				frameStamp = htons(frameStamp);
				memcpy(packet + packetLen, &frameStamp, sizeof(GVFrameStamp));
				HandleCapturedPacket(packet, packetLen + sizeof(GVFrameStamp), frameStamp, volume);
			}
		}
		while(result);
	}
}

#if defined(_UNIX)
#include <sys/select.h>
#include <termios.h>
#include <curses.h>
//#include <stropts.h>
int _kbhit(void)
{
    static const int STDIN = 0;
    static int initialized = 0;
    struct timeval timeout;
    struct fd_set rdset;

    if (initialized == 0)
	{
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }

    FD_ZERO(&rdset);
    FD_SET(STDIN, &rdset);
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;
    return select(STDIN + 1, &rdset, NULL, NULL, &timeout); 
}
#endif

#if !defined(_PS2) && !defined(_PSP) && !defined(_PS3)
int gsiGetChar(void)
{
	int c;

	if(_kbhit() == 0)
		return -1;

#if defined(_WIN32)
	c = _getch();
#else
	c = getchar();
#endif

	return tolower(c);
}
#endif

static GVBool CheckInput(void)
{
#if defined(_PS2)
	int events[NumPadEvents];
	memset(events, 0, sizeof(events));
	PadReadInput(events);
	if(events[PadX])
	{
		return GVFalse;
	}
	if(events[PadSquare])
	{
		if(capDevice && !gvIsDeviceStarted(capDevice, GV_CAPTURE))
		{
			gvStartDevice(capDevice, GV_CAPTURE);
			puts("Started capture");
		}
	}
	if(events[PadCircle])
	{
		if(capDevice && gvIsDeviceStarted(capDevice, GV_CAPTURE))
		{
			gvStopDevice(capDevice, GV_CAPTURE);
			puts("Stopped capture");
		}
	}
	return GVTrue;
#elif defined(_PSP)
	return GVTrue;
#elif defined(_PS3)
	// On PS3, capture and playback device point to the same device 
	// i.e. the USB Headset

	int ret;
	CellPadData PadData;
	CellPadInfo PadInfo;

	ret = cellPadGetInfo (&PadInfo);
	if (ret != 0) 
	{
		printf ("Error obtaining cell pad info: (%08X)\n", ret);
		return GVFalse;
	}

	if (PadInfo.status[0] == CELL_PAD_STATUS_DISCONNECTED) 
	{
		if (controllerConnected)
		{
			printf("The controller has been disconnected\n");
			controllerConnected = gsi_false;
		}
		return GVTrue;
	}
	else 
	{
		if (!controllerConnected)
		{
			printf("The controller has been connected\n");
			controllerConnected = gsi_true;
		}
	}
	ret = cellPadGetData (0, &PadData);
	if (PadData.len > 0)
	{
		if (PadData.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS)
		{
			return GVFalse;
		}

		if (PadData.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE)
		{
			if ((capDevice && playDevice) && !gvIsDeviceStarted(capDevice, GV_CAPTURE_AND_PLAYBACK))
			{
				gvStartDevice(capDevice, GV_CAPTURE_AND_PLAYBACK);
				printf("Started USB Headset capture/playback");
			}			
		}
		if (PadData.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE)
		{
			if ((capDevice && playDevice) && gvIsDeviceStarted(capDevice, GV_CAPTURE_AND_PLAYBACK))
			{
				gvStopDevice(capDevice, GV_CAPTURE_AND_PLAYBACK);
				printf("Stopped USB Headset capture/playback\n");
			}
		}
	}
	return GVTrue;
	
#else
	int c;
	GVScalar volume;
	int num;
	int channel;
	gsi_char name[GV_CHANNEL_NAME_LEN];

	while((c = gsiGetChar()) != -1)
	{
		switch(c)
		{
		case'/':
		case'?':
			puts("q:     quit\n");
			puts("c/v:   start/stop capture device\n");
			puts("b/n:   start/stop playback device\n");
			puts("d/f/g: get/decrease/increase capture volume\n");
			puts("e/r/t: get/decrease/increase playback volume\n");
			puts("-/+:   decrease/increase capture channel\n");
			puts("[/]:   decrease/increase playback channel\n");
			puts("?:     help\n");
			break;
		case 'q':
			return GVFalse;
		case 'c':
			if(capDevice && !gvIsDeviceStarted(capDevice, GV_CAPTURE))
			{
				gvStartDevice(capDevice, GV_CAPTURE);
				puts("Started capture");
			}
			break;
		case 'b':
			if(playDevice && !gvIsDeviceStarted(playDevice, GV_PLAYBACK))
			{
				gvStartDevice(playDevice, GV_PLAYBACK);
				puts("Started playback");
			}
			break;
		case 'v':
			if(capDevice && gvIsDeviceStarted(capDevice, GV_CAPTURE))
			{
				gvStopDevice(capDevice, GV_CAPTURE);
				puts("Stopped capture");
			}
			break;
		case 'n':
			if(playDevice && gvIsDeviceStarted(playDevice, GV_PLAYBACK))
			{
				gvStopDevice(playDevice, GV_PLAYBACK);
				puts("Stopped playback");
			}
			break;
		case 'd':
			if(capDevice)
			{
				volume = gvGetDeviceVolume(capDevice, GV_CAPTURE);
				printf("Capture volume: %.f%%\n", (float)volume * 100);
			}
			break;
		case 'e':
			if(playDevice)
			{
				volume = gvGetDeviceVolume(playDevice, GV_PLAYBACK);
				printf("Playback volume: %.f%%\n", (float)volume * 100);
			}
			break;
		case 'f':
			if(capDevice)
			{
				volume = gvGetDeviceVolume(capDevice, GV_CAPTURE);
				volume = max(0.0, (volume - 0.2));
				gvSetDeviceVolume(capDevice, GV_CAPTURE, volume);
				printf("Capture volume set: %.f%%\n", (float)volume * 100);
			}
			break;
		case 'g':
			if(capDevice)
			{
				volume = gvGetDeviceVolume(capDevice, GV_CAPTURE);
				volume = min(1.0, (volume + 0.2));
				gvSetDeviceVolume(capDevice, GV_CAPTURE, volume);
				printf("Capture volume set: %.f%%\n", (float)volume * 100);
			}
			break;
		case 'r':
			if(playDevice)
			{
				volume = gvGetDeviceVolume(playDevice, GV_PLAYBACK);
				volume = max(0.0, (volume - 0.2));
				gvSetDeviceVolume(playDevice, GV_PLAYBACK, volume);
				printf("Playback volume set: %.f%%\n", (float)volume * 100);
			}
			break;
		case 't':
			if(playDevice)
			{
				volume = gvGetDeviceVolume(playDevice, GV_PLAYBACK);
				volume = min(1.0, (volume + 0.2));
				gvSetDeviceVolume(playDevice, GV_PLAYBACK, volume);
				printf("Playback volume set: %.f%%\n", (float)volume * 100);
			}
			break;
		case '-':
			if(capDevice)
			{
				num = gvGetNumChannels(capDevice, GV_CAPTURE);
				if(num == 0)
				{
					printf("Capture device has no channels\n");
				}
				else
				{
					channel = gvGetChannel(capDevice, GV_CAPTURE);
					if(channel > 0)
						gvSetChannel(capDevice, GV_CAPTURE, --channel);
					gvGetChannelName(capDevice, GV_CAPTURE, channel, name);
					_tprintf(_T("Capture channel: %s [%d] (%d total)\n"), name, channel, num);
				}
			}
			break;
		case '=':
		case '+':
			if(capDevice)
			{
				num = gvGetNumChannels(capDevice, GV_CAPTURE);
				if(num == 0)
				{
					printf("Capture device has no channels\n");
				}
				else
				{
					channel = gvGetChannel(capDevice, GV_CAPTURE);
					if(channel < (num - 1))
						gvSetChannel(capDevice, GV_CAPTURE, ++channel);
					gvGetChannelName(capDevice, GV_CAPTURE, channel, name);
					_tprintf(_T("Capture channel: %s [%d] (%d total)\n"), name, channel, num);
				}
			}
			break;
		case '[':
			if(playDevice)
			{
				num = gvGetNumChannels(playDevice, GV_PLAYBACK);
				if(num == 0)
				{
					printf("Playback device has no channels\n");
				}
				else
				{
					channel = gvGetChannel(playDevice, GV_PLAYBACK);
					if(channel > 0)
						gvSetChannel(playDevice, GV_PLAYBACK, --channel);
					gvGetChannelName(playDevice, GV_PLAYBACK, channel, name);
					_tprintf(_T("Playback channel: %s [%d] (%d total)\n"), name, channel, num);
				}
			}
			break;
		case ']':
			if(playDevice)
			{
				num = gvGetNumChannels(playDevice, GV_PLAYBACK);
				if(num == 0)
				{
					printf("Playback device has no channels\n");
				}
				else
				{
					channel = gvGetChannel(playDevice, GV_PLAYBACK);
					if(channel < (num - 1))
						gvSetChannel(playDevice, GV_PLAYBACK, ++channel);
					gvGetChannelName(playDevice, GV_PLAYBACK, channel, name);
					_tprintf(_T("Playback channel: %s [%d] (%d total)\n"), name, channel, num);
				}
			}
			break;
		}
	}
	return GVTrue;
#endif
}

#ifdef __MWERKS__ // CodeWarrior will warn if not prototyped
	int test_main(int argc, char **argp);
#endif	
int test_main(int argc, char **argp)
{
	char remoteIP[64];

	if(argc >= 2)
		strcpy(remoteIP, argp[1]);
	else
	{
		// Put in your own IP address
		strcpy(remoteIP, "192.168.0.100");
	}
	if(!Initialize(remoteIP))
		return 1;

	Startup();

	while(CheckInput())
	{
		Think();

		ShowInfo();

		msleep(1);
	}

	Destroy();
	
	return 0;
}
