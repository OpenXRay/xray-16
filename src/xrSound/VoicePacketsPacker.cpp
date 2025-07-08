#include "stdafx.h"
#include "opus/opus.h"
#include "VoicePacketsPacker.h"
#include "SoundVoiceChat.h"
#include "VoicePacket.h"

#define OPUS_BITRATE 16000

CVoicePacketsPacker::CVoicePacketsPacker()
{
	int error;
	m_pEncoder = opus_encoder_create(VOICE_SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP, &error);
	if (error != OPUS_OK)
	{
		opus_encoder_destroy(m_pEncoder);
		R_ASSERT2(0, "Opus error");
	}

	opus_encoder_ctl(m_pEncoder, OPUS_SET_BITRATE(OPUS_BITRATE));
	opus_encoder_ctl(m_pEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

	for (int i = 0; i < MAX_VOICE_PACKETS; ++i)
	{
		m_voicePackets[i] = xr_new<VoicePacket>();
	}
}

CVoicePacketsPacker::~CVoicePacketsPacker()
{
	if (m_pEncoder != nullptr)
	{
		opus_encoder_destroy(m_pEncoder);
		m_pEncoder = nullptr;
	}

	for (int i = 0; i < MAX_VOICE_PACKETS; ++i)
		xr_delete(m_voicePackets[i]);
}

void CVoicePacketsPacker::AddSender(IVoicePacketSender* ref)
{
	m_senders.push_back(ref);
}

void CVoicePacketsPacker::RemoveSender(IVoicePacketSender* ref)
{
	m_senders.erase(std::remove(m_senders.begin(), m_senders.end(), ref));
}

void CVoicePacketsPacker::AddPacket(const void* data, const int length)
{
	if (m_packetsCount >= MAX_VOICE_PACKETS)
		return;

	VoicePacket* packet = m_voicePackets[m_packetsCount];
	packet->length = opus_encode(m_pEncoder,
		(opus_int16*)data,
		length / sizeof(opus_int16),
		packet->data,
		VoicePacket::MAX_DATA_SIZE
	);

	packet->time = GetTickCount();
	++m_packetsCount;
}

void CVoicePacketsPacker::Update()
{
	if (m_packetsCount > 0)
	{
		VoicePacket* lastPacket = m_voicePackets[m_packetsCount - 1];
		if (m_packetsCount > MAX_VOICE_PACKETS - 1 || (lastPacket->time + 50) < GetTickCount())
		{
			for (auto it = m_senders.begin(); it != m_senders.end(); ++it)
			{
				(*it)->Send(m_voicePackets, m_packetsCount);
				m_packetsCount = 0;
			}
			m_packetsCount = 0;
		}
	}
}