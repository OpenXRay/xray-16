#include "gvSource.h"
#include "gvCodec.h"
#include "gvFrame.h"
#include "gvUtil.h"
#include "gvMain.h"
#include <stdio.h>

// some code based on:
// "Skew Detection and Compensation for Internet Audio Applications"
// http://csperkins.org/publications/icme2000.pdf

#define GVI_SYNCHRONIZATION_DELAY                 200

#define GVI_MAX_SOURCES                           8

#define GVI_CLOCK_OFFSET_AVERAGING_FACTOR         0.96875  //(31/32)

#define GVI_DIVERGENCE_HIGH_WATERMARK             10
#define GVI_DIVERGENCE_LOW_WATERMARK              ((GVFrameStamp)-5)

#define GVI_SHOW_SKEW_CORRECTIONS                 0
#define GVI_SHOW_SOURCELIST_CHANGES               0

typedef struct GVISource
{
	GVBool          m_inUse;
	GVSource        m_source;
	GVBool          m_isTalking;
	GVFrameStamp    m_finishedTalkingTime;
	GVFrameStamp    m_clockOffset;
	float           m_clockOffsetAverage;
	GVDecoderData   m_decoderData;
	GVIPendingFrame m_frameHead;
} GVISource;

static int GVISynchronizationDelayFrames;
GVBool     GVIGlobalMute;


static void gviFreeSource(GVISource * source)
{
	GVIPendingFrame * frame = source->m_frameHead.m_next;
	GVIPendingFrame * next;

	// make sure it is in use
	if(!source->m_inUse)
		return;

	// put all of the pending frames back into the list
	while(frame)
	{
		next = frame->m_next;
		gviPutPendingFrame(frame);
		frame = next;
	}

	// free the decoder data
	gviFreeDecoder(source->m_decoderData);

#if GVI_SHOW_SOURCELIST_CHANGES
	printf("Freed source\n");
#endif

	// it is no longer in use
	source->m_inUse = GVFalse;
}

GVISourceList gviNewSourceList(void)
{
	int size;
	GVISourceList sourceList;

	if(!GVISynchronizationDelayFrames)
	{
		int delay;
		
		delay = gviMultiplyByBytesPerMillisecond(GVI_SYNCHRONIZATION_DELAY);
		delay = gviRoundUpToNearestMultiple(delay, GVIBytesPerFrame);

		GVISynchronizationDelayFrames = (delay / GVIBytesPerFrame);
	}

	size = (sizeof(GVISource) * GVI_MAX_SOURCES);
	sourceList = (GVISourceList)gsimalloc((unsigned int)size);
	if(sourceList)
		memset(sourceList, 0, (unsigned int)size);

	return sourceList;
}

void gviFreeSourceList(GVISourceList sourceList)
{
	gviClearSourceList(sourceList);
	gsifree(sourceList);
}

void gviClearSourceList(GVISourceList sourceList)
{
	int i;

	assert(sourceList);

	for(i = 0 ; i < GVI_MAX_SOURCES ; i++)
		gviFreeSource(&sourceList[i]);
}

static GVISource * gviFindSourceInList(GVISourceList sourceList, GVSource source)
{
	GVISource * gviSource;
	int i;

	// check if this source is in the list
	for(i = 0 ; i < GVI_MAX_SOURCES ; i++)
	{
		gviSource = &sourceList[i];
		if(gviSource->m_inUse)
		{
			if(memcmp(&gviSource->m_source, &source, sizeof(GVSource)) == 0)
			{
				return gviSource;
			}
		}
	}

	return NULL;
}

GVBool gviIsSourceTalking(GVISourceList sourceList, GVSource source)
{
	GVISource * gviSource = gviFindSourceInList(sourceList, source);
	if(!gviSource)
		return GVFalse;

	return gviSource->m_isTalking;
}

int gviListTalkingSources(GVISourceList sourceList, GVSource sources[], int maxSources)
{
	GVISource * gviSource;
	int numTalking = 0;
	int i;

	// loop through the sources
	for(i = 0 ; i < GVI_MAX_SOURCES ; i++)
	{
		gviSource = &sourceList[i];

		// check if the source is in use and talking
		if(gviSource->m_inUse && gviSource->m_isTalking)
		{
			// add it to the list
			memcpy(&sources[numTalking], &gviSource->m_source, sizeof(GVSource));

			// one more talker
			numTalking++;

			// check for the max
			if(numTalking == maxSources)
				break;
		}
	}

	return numTalking;
}

void gviSetGlobalMute(GVBool mute)

{
	GVIGlobalMute = mute;
}

GVBool gviGetGlobalMute(void)
{
	return GVIGlobalMute;
}

static GVISource * gviAddSourceToList(GVISourceList sourceList, GVSource source)
{
	GVBool result;
	GVISource * gviSource = NULL;
	int i;

	// loop through the sources
	for(i = 0 ; i < GVI_MAX_SOURCES ; i++)
	{
		// check if this source is available
		if(!sourceList[i].m_inUse)
		{
			gviSource = &sourceList[i];
			break;
		}

		// also look for a source without frames
		// if we don't find a totally free one, we can take this one over
		if(!gviSource && !sourceList[i].m_frameHead.m_next)
			gviSource = &sourceList[i];
	}

	// check if we didn't find anything
	if(!gviSource)
		return NULL;

	// if this source is already in use, free it first
	if(gviSource->m_inUse)
		gviFreeSource(gviSource);

	// make sure we can get a new decoder before moving on
	result = gviNewDecoder(&gviSource->m_decoderData);
	if(!result)
		return NULL;

	// mark as in use
	gviSource->m_inUse = GVTrue;

	// set the rest of the info
	memcpy(&gviSource->m_source, &source, sizeof(GVSource));
	gviSource->m_clockOffset = 0;
	gviSource->m_clockOffsetAverage = 0;
	gviSource->m_isTalking = GVFalse;
	gviSource->m_finishedTalkingTime = 0;
	gviSource->m_frameHead.m_next = NULL;

#if GVI_SHOW_SOURCELIST_CHANGES
	printf("Added source\n");
#endif

	return gviSource;
}

static void gviAddPacketToSource(GVISource * source, GVFrameStamp frameStamp, const GVByte * packet, int len, GVBool mute)
{
	GVIPendingFrame * frame;
	GVIPendingFrame * nextFrame;
	GVIPendingFrame * newFrame;
	GVFrameStamp packetFinishedTime;
	int numFrames;
	int i;

	// the packet len should be a multiple of the encoded frame size
	assert(!(len % GVIEncodedFrameSize));

	// calculate the number of frames in this packet
	numFrames = (len / GVIEncodedFrameSize);

	// use the clock offset to adjust the frameStamp
	// Expanded to remove warnings in VS2K5
	frameStamp = frameStamp + (GVFrameStamp)source->m_clockOffset;

	// figure out when this packet will be finished
	packetFinishedTime = (GVFrameStamp)(frameStamp + numFrames);

	// update the time at which this source is done talking
	if(gviIsFrameStampGT(packetFinishedTime, source->m_finishedTalkingTime))
		source->m_finishedTalkingTime = packetFinishedTime;

	// if muted, don't add.
	if(mute || GVIGlobalMute)
	{
		//Flag that they are currently talking because we skip this step in the hardware.
		source->m_isTalking = GVTrue;
		return;
	}

	// find where to add it
	// it must be in chronological order
	for(frame = &source->m_frameHead ; frame->m_next ; frame = frame->m_next)
	{
		// store a pointer to the next frame
		// this is the frame we will compare to this time through the loop
		nextFrame = frame->m_next;

		// check if the framestamp is the same (a repeated packet)
		if(nextFrame->m_frameStamp == frameStamp)
			return;

		// check if the new frame should be placed in front of the next frame
		if(gviIsFrameStampGT(nextFrame->m_frameStamp, frameStamp))
		{
			// check that the packet's finish time doesn't cross over the next frame
			assert(!gviIsFrameStampGT(packetFinishedTime, nextFrame->m_frameStamp));

			// everything is good, break out so we can insert the new frames
			break;
		}
	}

	// loop through the frames in the packet
	for(i = 0 ; i < numFrames ; i++)
	{
		// get a new frame
		newFrame = gviGetPendingFrame();
		if(!newFrame)
			return;

		// fill it in
		newFrame->m_frameStamp = frameStamp;
#if GVI_PRE_DECODE
		gviDecodeSet(newFrame->m_frame, packet, source->m_decoderData);
#else
		memcpy(newFrame->m_frame, packet, (unsigned int)GVIEncodedFrameSize);
#endif
		newFrame->m_next = frame->m_next;

		// setup the previous frame's next pointer to point to this one
		frame->m_next = newFrame;

		// adjust vars
		frameStamp++;
		packet += GVIEncodedFrameSize;
		frame = newFrame;
	}
}

void gviAddPacketToSourceList(GVISourceList sourceList,
							  const GVByte * packet, int len, GVSource source, GVFrameStamp frameStamp, GVBool mute,
							  GVFrameStamp currentPlayClock)
{
	GVFrameStamp playTime;
	GVISource * gviSource;
	GVFrameStamp clockOffset;
	GVFrameStamp divergence;
	int unwrappedClockOffset;

	// add the sync delay to the clock to get the play time
	playTime = (GVFrameStamp)(currentPlayClock + GVISynchronizationDelayFrames);

	// calculate the clock offset
	clockOffset = (GVFrameStamp)(playTime - frameStamp);

	// check if this source already exists
	gviSource = gviFindSourceInList(sourceList, source);
	if(!gviSource)
	{
		// it isn't, so add it
		gviSource = gviAddSourceToList(sourceList, source);

		// drop the packet if it couldn't be added
		if(!gviSource)
			return;

		// store the clock offset
		gviSource->m_clockOffset = clockOffset;
		gviSource->m_clockOffsetAverage = (float)clockOffset;

		// init the finished talking time
		gviSource->m_finishedTalkingTime = (GVFrameStamp)(frameStamp + gviSource->m_clockOffset);
	}
	else
	{
		// unwrap the clock offset if needed
		if((clockOffset < gviSource->m_clockOffsetAverage) && gviIsFrameStampGT(clockOffset, (GVFrameStamp)gviSource->m_clockOffsetAverage))
		{
			unwrappedClockOffset = (clockOffset + GVI_FRAMESTAMP_MAX);
		}
		else if((gviSource->m_clockOffsetAverage < clockOffset) && gviIsFrameStampGT((GVFrameStamp)gviSource->m_clockOffsetAverage, clockOffset))
		{
			unwrappedClockOffset = (clockOffset - GVI_FRAMESTAMP_MAX);
		}
		else
		{
			unwrappedClockOffset = clockOffset;
		}

		// update the running average of the clock offset
		gviSource->m_clockOffsetAverage =
			(float)((gviSource->m_clockOffsetAverage * GVI_CLOCK_OFFSET_AVERAGING_FACTOR) +
			(unwrappedClockOffset * (1.0 - GVI_CLOCK_OFFSET_AVERAGING_FACTOR)));
		if(gviSource->m_clockOffsetAverage < 0)
			gviSource->m_clockOffsetAverage += GVI_FRAMESTAMP_MAX;
		else if(gviSource->m_clockOffsetAverage >= GVI_FRAMESTAMP_MAX)
			gviSource->m_clockOffsetAverage -= GVI_FRAMESTAMP_MAX;

		// calculate the divergence
		divergence = (GVFrameStamp)(gviSource->m_clockOffset - (GVFrameStamp)gviSource->m_clockOffsetAverage);

		// check against the high-water mark
		if(gviIsFrameStampGT(divergence, GVI_DIVERGENCE_HIGH_WATERMARK))
		{
#if GVI_SHOW_SKEW_CORRECTIONS
			static int dropCount;
			printf("DROP: %d\n", ++dropCount);
#endif

			// update the clock offset
			gviSource->m_clockOffset--;

			// if this is a one frame packet, just drop it
			if(len == GVIEncodedFrameSize)
				return;

			// otherwise update the params
			packet += GVIEncodedFrameSize;
			len -= GVIEncodedFrameSize;
		}
		// check against the low-water mark
		else if(gviIsFrameStampGT(GVI_DIVERGENCE_LOW_WATERMARK, divergence))
		{
#if GVI_SHOW_SKEW_CORRECTIONS
			static int insertCount;
			printf("INSERT: %d\n", ++insertCount);
#endif

			// update the clock offset
			// this will basically add a frame of silence
			gviSource->m_clockOffset++;
		}

		// check if this packet is too old or too far ahead to play
		if(gviIsFrameStampGT(currentPlayClock, (GVFrameStamp)(frameStamp + gviSource->m_clockOffset)))
			return;
	}

	// add the packet to the source
	gviAddPacketToSource(gviSource, frameStamp, packet, len, mute);
}

GVBool gviWriteSourcesToBuffer(GVISourceList sourceList, GVFrameStamp startTime,
                               GVSample * sampleBuffer, int numFrames)
{
	GVISource * source;
	GVFrameStamp timeSliceEnd;
	GVIPendingFrame * frame;
	int i;
	GVBool result = GVFalse;

	// calculate the end of the time slice
	timeSliceEnd = (GVFrameStamp)(startTime + numFrames);

	// clear the sample buffer
	memset(sampleBuffer, 0, (unsigned int)numFrames * GVIBytesPerFrame);

	// loop through the sources
	for(i = 0 ; i < GVI_MAX_SOURCES ; i++)
	{
		// get the next source
		source = &sourceList[i];

		// check if it is in use
		if(!source->m_inUse)
			continue;

		// keep going while there are pending frames for this source
		while(source->m_frameHead.m_next)
		{
			// cache a pointer to the first frame
			frame = source->m_frameHead.m_next;

			// check if this frame is too far ahead
			if(gviIsFrameStampGTE(frame->m_frameStamp, timeSliceEnd))
				break;

			// make sure this buffer's timeslice hasn't already elapsed
			if(gviIsFrameStampGTE(frame->m_frameStamp, startTime))
			{
				// add the frame to the buffer
				GVSample * writePtr = (sampleBuffer + ((GVFrameStamp)(frame->m_frameStamp - startTime) * GVISamplesPerFrame));
#if GVI_PRE_DECODE
				int j;
				for(j = 0 ; j < GVISamplesPerFrame ; j++)
					writePtr[j] += frame->m_frame[j];
#else
				gviDecodeAdd(writePtr, frame->m_frame, source->m_decoderData);
#endif

				// this source is talking
				source->m_isTalking = GVTrue;

				// set the return value to indicate that decoding took place
				result = GVTrue;
			}

			// update the source to point to the next frame
			source->m_frameHead.m_next = frame->m_next;

			// free the frame we just used
			gviPutPendingFrame(frame);
		}

		// remove the source if it has no more frames and is marked as finished
		if(!source->m_frameHead.m_next && gviIsFrameStampGTE(timeSliceEnd, source->m_finishedTalkingTime))
			gviFreeSource(source);
	}

	return result;

}
