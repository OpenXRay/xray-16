#pragma once
#include "al.h"
#include "alc.h"
#include "ISoundVoiceChat.h"
#include "SoundRecorderA.h"
#include "StreamPlayerA.h"
#include "VoicePacketsPacker.h"

#define VOICE_SAMPLE_RATE 24000
#define VOICE_SAMPLES_PER_BUFFER 960
#define VOICE_FORMAT AL_FORMAT_MONO16

class SoundVoiceChat : ISoundVoiceChat
{
public:
	SoundVoiceChat(ALCcontext* pContext);
	~SoundVoiceChat();

	ISoundRecorder* CreateRecorder(IVoicePacketSender* sender);

	IStreamPlayer* CreateStreamPlayer();
	void DestroySoundPlayer(IStreamPlayer* player);

	void Update(const Fvector& P, const Fvector& D, const Fvector& N);

private:
	void Destroy();

private:
	ALCcontext* m_pContext;

	CSoundRecorderA* m_pRecorder = nullptr;
	CVoicePacketsPacker* m_pVoicePacker = nullptr;

	xr_vector<IStreamPlayer*> m_players;
};