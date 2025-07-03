#pragma once
#include "ui_defs.h"
#include "../xrSound/ISoundVoiceChat.h"
#include "VoiceSender.h"

class CVoiceChat
{
private:

	struct SVoiceIconInfo
	{
		u64 time;

		SVoiceIconInfo() = default;
		SVoiceIconInfo(u64 _time)
		{
			time = _time;
		}
	};

	typedef xr_map<u16, IStreamPlayer*> SOUND_PLAYERS;
	typedef xr_map<u16, SVoiceIconInfo> PLAYERS_VOICE_TIME;

public:
	CVoiceChat();
	~CVoiceChat();

	bool CreateRecorder();

	void Start();
	void Stop();
	bool IsStarted();

	u8 GetDistance() const;
	u8 SwitchDistance();

	void Update();
	void OnRender();

	void ReceiveMessage(NET_Packet* P);

private:
	const ui_shader& GetVoiceIndicatorShader();

	IStreamPlayer* GetStreamPlayer(u16 clientId);

	void CheckAndClearPlayers(SOUND_PLAYERS& players);

private:
	byte m_buffer[1024];

	ISoundVoiceChat* m_pSoundVoiceChat = nullptr;
	ISoundRecorder* m_pRecorder = nullptr;

	CVoiceSender* m_pSender = nullptr;
	SOUND_PLAYERS m_soundPlayersMap;

	PLAYERS_VOICE_TIME m_voiceTimeMap;

	ui_shader m_voiceIndicatorShader;
};