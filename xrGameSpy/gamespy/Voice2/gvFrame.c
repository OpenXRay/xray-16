#include "gvFrame.h"
#include "gvCodec.h"

// packets are only accepted if they are frameStamped to be played
// within this many frames from the current play clock
#define GVI_INCOMING_PACKET_TIMEFRAME_FRAMES      (GVI_FRAMESTAMP_MAX / 2)

#define GVI_PREALLOCATED_FRAMES                   200

#if defined(_PS2) || defined(_PSP)
#define GVI_DYNAMICALLY_ALLOCATE_FRAMES           0
#else
#define GVI_DYNAMICALLY_ALLOCATE_FRAMES           1
#endif

// list of available frames
static GVIPendingFrame * GVIAvailableFrames;

GVBool gviIsFrameStampGT(GVFrameStamp a, GVFrameStamp b)
{
	return ((GVFrameStamp)(b - a) > GVI_INCOMING_PACKET_TIMEFRAME_FRAMES);
}

GVBool gviIsFrameStampGTE(GVFrameStamp a, GVFrameStamp b)
{
	return ((GVFrameStamp)(b - a - 1) > GVI_INCOMING_PACKET_TIMEFRAME_FRAMES);
}

static void gviFreePendingFrame(GVIPendingFrame * frame)
{
	gsifree(frame);
}

static GVIPendingFrame * gviNewPendingFrame(void)
{
	GVIPendingFrame * frame;

	// allocate a new frame
#if GVI_PRE_DECODE
	frame = (GVIPendingFrame *)gsimalloc(sizeof(GVIPendingFrame) + GVIBytesPerFrame - sizeof(GVSample));
#else
	frame = (GVIPendingFrame *)gsimalloc(sizeof(GVIPendingFrame) + GVIEncodedFrameSize - 1);
#endif

	// return it
	return frame;
}

void gviPutPendingFrame(GVIPendingFrame * frame)
{
	// put the frame back in the available frames list
	frame->m_next = GVIAvailableFrames;
	GVIAvailableFrames = frame;
}

GVIPendingFrame * gviGetPendingFrame(void)
{
	GVIPendingFrame * frame;

	// check the available frames list
	if(GVIAvailableFrames)
	{
		frame = GVIAvailableFrames;
		GVIAvailableFrames = frame->m_next;
		return frame;
	}

#if GVI_DYNAMICALLY_ALLOCATE_FRAMES
	// allocate a new frame
	return gviNewPendingFrame();
#else
	// we can't dynamically allocate frames
	return NULL;
#endif
}

void gviFramesStartup(void)
{
	GVIPendingFrame * frame;
	int i;

	if(GVIAvailableFrames)
		gviFramesCleanup();

	GVIAvailableFrames = NULL;

	for(i = 0 ; i < GVI_PREALLOCATED_FRAMES ; i++)
	{
		frame = gviNewPendingFrame();
		if(!frame)
			return;
		frame->m_next = GVIAvailableFrames;
		GVIAvailableFrames = frame;
	}
}

void gviFramesCleanup(void)
{
	GVIPendingFrame * next;

	while(GVIAvailableFrames)
	{
		next = GVIAvailableFrames->m_next;
		gviFreePendingFrame(GVIAvailableFrames);
		GVIAvailableFrames = next;
	}
}
