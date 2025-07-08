#include "stdafx.h"
#include "SoundVoiceChat.h"

SoundVoiceChat::SoundVoiceChat(ALCcontext* pContext)
	: m_pContext(pContext)
{
}

SoundVoiceChat::~SoundVoiceChat()
{
	Destroy();
}

void SoundVoiceChat::Destroy()
{
	if (m_pVoicePacker)
	{
		xr_delete(m_pVoicePacker);
		m_pVoicePacker = nullptr;
	}

	if (m_pRecorder)
	{
		xr_delete(m_pRecorder);
		m_pRecorder = nullptr;
	}
}

ISoundRecorder* SoundVoiceChat::CreateRecorder(IVoicePacketSender* sender)
{
	Destroy();
	m_pVoicePacker = xr_new<CVoicePacketsPacker>();
	m_pVoicePacker->AddSender(sender);

	m_pRecorder = xr_new<CSoundRecorderA>(VOICE_SAMPLE_RATE, VOICE_FORMAT, VOICE_SAMPLES_PER_BUFFER);

	if (!m_pRecorder->Init(m_pVoicePacker))
	{
		Destroy();
		return nullptr;
	}

	return (ISoundRecorder*)m_pRecorder;
}

IStreamPlayer* SoundVoiceChat::CreateStreamPlayer()
{
	IStreamPlayer* player = (IStreamPlayer*)xr_new<CStreamPlayerA>(VOICE_SAMPLE_RATE, VOICE_FORMAT, m_pContext);
	m_players.push_back(player);
	return player;
}

void SoundVoiceChat::DestroySoundPlayer(IStreamPlayer* player)
{
	xr_vector<IStreamPlayer*>::iterator I = m_players.begin(), J;
	xr_vector<IStreamPlayer*>::iterator E = m_players.end();

	for (; I != E;)
	{
		if ((*I) == player)
		{
			xr_delete(*I);

			J = I;
			++I;
			m_players.erase(J);
		}
		else {
			++I;
		}
	}
}

void SoundVoiceChat::Update(const Fvector& P, const Fvector& D, const Fvector& N)
{
	if (m_pRecorder)
		m_pRecorder->Update();

	if (m_pVoicePacker)
		m_pVoicePacker->Update();

	for (auto& player : m_players)
		player->Update();

}