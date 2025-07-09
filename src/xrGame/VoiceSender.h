#pragma once
#include "../xrSound/IVoicePacketSender.h"

class CVoiceSender : IVoicePacketSender
{
public:
	u8 GetDistance() const { return m_distance; }
	void SetDistance(u8 value) { m_distance = value; }
	virtual void Send(VoicePacket** packets, u8 count) override;
private:
	u8 m_distance = 10;
};