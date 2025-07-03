#pragma once
#include "VoicePacket.h"

class IVoicePacketSender
{
public:
	virtual void Send(VoicePacket** packets, u8 count) = 0;
};