#pragma once
#include "VoicePacket.h"
#include "IVoicePacketSender.h"

struct OpusEncoder;

class CVoicePacketsPacker
{
public:
	CVoicePacketsPacker();
	~CVoicePacketsPacker();

	virtual void AddSender(IVoicePacketSender* ref);
	virtual void RemoveSender(IVoicePacketSender* ref);

	void AddPacket(const void* data, const int length);
	void Update();

private:
	static constexpr size_t MAX_VOICE_PACKETS = 4;
	xr_vector<IVoicePacketSender*> m_senders;

	OpusEncoder* m_pEncoder = nullptr;
	VoicePacket* m_voicePackets[MAX_VOICE_PACKETS];
	u8 m_packetsCount = 0;
};