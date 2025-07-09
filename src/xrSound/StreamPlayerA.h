#pragma once
#include "al.h"
#include "alc.h"
#include "opus/opus.h"
#include "RingBuffer.h"
#include "IStreamPlayer.h"
#include "xrCommon/xr_deque.h"

class CStreamPlayerA : public IStreamPlayer
{
public:
	CStreamPlayerA(ALuint sampleRate, ALenum format, ALCcontext* context);
	~CStreamPlayerA();

	virtual void PushToPlay(const void* data, int count);
	virtual bool IsPlaying();

	virtual void Update();

	virtual void SetDistance(float value);
	virtual void SetPosition(const Fvector& pos);
private:
	void UpdateVolume();

private:

	static constexpr ALsizei RING_BUFFER_SIZE = 262144;
	CRingBuffer<opus_int16, RING_BUFFER_SIZE> m_ringBuffer;

	static constexpr ALsizei NUM_BUFFERS = 16;
	ALuint m_buffers[NUM_BUFFERS];

	xr_deque<ALuint> m_freeBuffers;

	ALuint m_sampleRate;
	ALenum m_format;

	ALCcontext* m_pContext;
	ALuint m_source;

	OpusDecoder* m_pOpusDecoder;

	Fvector m_position {0, 0, 0};

	float m_distance = 0;
	bool m_isRelative = false;
};
