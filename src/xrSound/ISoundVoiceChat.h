#pragma once
#include "ISoundRecorder.h"
#include "IStreamPlayer.h"
#include "IVoicePacketSender.h"

class ISoundVoiceChat
{
public:
	virtual ISoundRecorder* CreateRecorder(IVoicePacketSender* sender) = 0;
	virtual IStreamPlayer* CreateStreamPlayer() = 0;
	virtual void DestroySoundPlayer(IStreamPlayer* player) = 0;
};