#include "stdafx.h"
#include "StreamPlayerA.h"
#include "SoundVoiceChat.h"
#include "SoundRender_Core.h"

CStreamPlayerA::CStreamPlayerA(ALuint sampleRate, ALenum format, ALCcontext* context)
	: m_sampleRate(sampleRate), m_format(format), m_pContext(context), m_source(0)
{
	alcMakeContextCurrent(m_pContext);

	alGenBuffers(NUM_BUFFERS, m_buffers);
	for (uint16_t i = 0; i < NUM_BUFFERS; ++i)
		m_freeBuffers.push_back(m_buffers[i]);

	alGenSources(1, &m_source);

	alSourcef(m_source, AL_SOURCE_TYPE, AL_STREAMING);
	alSourcei(m_source, AL_LOOPING, AL_FALSE);
	alSourcef(m_source, AL_MIN_GAIN, 0.f);
	alSourcef(m_source, AL_MAX_GAIN, 1.0f);
	alSourcef(m_source, AL_ROLLOFF_FACTOR, 0.f);
	alSourcef(m_source, AL_GAIN, 1.f);
	alSourcei(m_source, AL_BUFFER, NULL);


	int error;
	m_pOpusDecoder = opus_decoder_create(VOICE_SAMPLE_RATE, 1, &error);

	if (error != OPUS_OK)
	{
		opus_decoder_destroy(m_pOpusDecoder);
		R_ASSERT2(0, "Opus error");
	}
}

CStreamPlayerA::~CStreamPlayerA()
{
	if (alIsSource(m_source))
	{
		alSourceStop(m_source);
		alSourcei(m_source, AL_BUFFER, NULL);
		alDeleteBuffers(NUM_BUFFERS, m_buffers);
		alDeleteSources(1, &m_source);
	}
	m_source = 0;


	if (m_pOpusDecoder != nullptr)
	{
		opus_decoder_destroy(m_pOpusDecoder);
		m_pOpusDecoder = nullptr;
	}
}

void CStreamPlayerA::SetDistance(float value)
{
	m_isRelative = (value == 0);
	alSourcei(m_source, AL_SOURCE_RELATIVE, m_isRelative);
	m_distance = value;
	clamp(m_distance, 0.f, 1000.f);
}

void CStreamPlayerA::SetPosition(const Fvector& pos)
{
	alSource3f(m_source, AL_POSITION, pos.x, pos.y, -pos.z);
	m_position = pos;
}

void CStreamPlayerA::PushToPlay(const void* data, int count)
{
	R_ASSERT2(m_source != 0 && alIsSource(m_source), "Not initialized sound source");

	opus_int16 decoded_buf[1024];

	int decoded_len = opus_decode(m_pOpusDecoder, (BYTE*)data, count, decoded_buf, VOICE_SAMPLES_PER_BUFFER, 0);
	if (decoded_len <= 0)
	{
		return;
	}

	m_ringBuffer.Write(decoded_buf, decoded_len);
}

void CStreamPlayerA::UpdateVolume()
{
	if (!alIsSource(m_source))
		return;

	if (!IsPlaying())
		return;

	float volume = 1.f;

	if (!m_isRelative)
	{
		float distance = SoundRender->listener_position().distance_to(m_position);

		const float max_distance = m_distance;
		const float min_distance = (m_distance / 3);

		if (distance <= min_distance)
		{
			volume = 1.f;
		}
		else if (distance > max_distance)
		{
			volume = 0.f;
		}
		else
		{
			volume = (max_distance - distance) / (max_distance - min_distance);
		}

		volume = volume * psSoundVPlayers;
		clamp(volume, 0.01f, 1.f);
	}

	alSourcef(m_source, AL_GAIN, volume);
}

void CStreamPlayerA::Update()
{
	if (m_source == 0 || !alIsSource(m_source))
		return;

	ALint processed, state;
	alGetSourcei(m_source, AL_SOURCE_STATE, &state);
	alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);

	if (alGetError() != AL_NO_ERROR)
	{
		return;
	}

	const bool isPlaying = state == AL_PLAYING;

	if (!m_ringBuffer.BytesToRead() && !isPlaying)
	{
		while (processed > 0)
		{
			ALuint bufid;
			alSourceUnqueueBuffers(m_source, 1, &bufid);
			m_freeBuffers.push_back(bufid);
			processed--;
		}
		return;
	}

	if (m_ringBuffer.IsHalfFull())
		alSourcef(m_source, AL_PITCH, 1.05f);
	else
		alSourcef(m_source, AL_PITCH, 1.f);

	while (processed > 0)
	{
		ALuint bufferId;
		alSourceUnqueueBuffers(m_source, 1, &bufferId);
		m_freeBuffers.push_back(bufferId);
		processed--;
	}

	while (m_ringBuffer.BytesToRead() && m_freeBuffers.size())
	{
		opus_int16 tempBuffer[4096];
		const ALuint bufferId = m_freeBuffers.front();
		m_freeBuffers.pop_front();

		ALsizei readed = 0;
		int bufferIndex = 0;
		readed = (ALsizei)m_ringBuffer.Read(tempBuffer, 4096);
		if (readed > 0)
		{
			alBufferData(bufferId, m_format, tempBuffer, readed * sizeof(opus_int16), m_sampleRate);
			alSourceQueueBuffers(m_source, 1, &bufferId);
		}
		else
			break;
	}

	UpdateVolume();

	if (!isPlaying)
	{
		alSourcePlay(m_source);
	}
}

bool CStreamPlayerA::IsPlaying()
{
	ALint state;
	alGetSourcei(m_source, AL_SOURCE_STATE, &state);
	return state == AL_PLAYING;
}