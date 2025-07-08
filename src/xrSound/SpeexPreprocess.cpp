#include "stdafx.h"
#include "SpeexPreprocess.h"

CSpeexPreprocess::CSpeexPreprocess(int sampleRate, int samplesPerBuffer)
{
	m_pState = speex_preprocess_state_init(samplesPerBuffer, sampleRate);

	// DEFAULT VALUES
	int useDenoise = 0;
	int useAcg = 0;
	float agcLevel = 30000;

	speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_SET_DENOISE, &useDenoise);
	speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_SET_AGC, &useAcg);
	speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agcLevel);
}

CSpeexPreprocess::~CSpeexPreprocess()
{
	speex_preprocess_state_destroy(m_pState);
	m_pState = nullptr;
}

bool CSpeexPreprocess::EnableDenoise(bool enable)
{
	int value = enable ? 1 : 0;
	if (m_pState)
	{
		return speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_SET_DENOISE, &value) == 0;
	}
	return false;
}

bool CSpeexPreprocess::EnableAGC(bool enable)
{
	int value = enable ? 1 : 0;
	if (m_pState)
	{
		return speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_SET_AGC, &value) == 0;
	}
	return false;
}

bool CSpeexPreprocess::IsAGCEnabled()
{
	int result = 0;
	if (m_pState)
	{
		speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_GET_AGC, &result);
	}
	return result != 0;
}

bool CSpeexPreprocess::IsDenoiseEnabled()
{
	int result = 0;
	if (m_pState)
	{
		speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_GET_DENOISE, &result);
	}
	return result != 0;
}

bool CSpeexPreprocess::SetDenoiseLevel(int level)
{
	if (m_pState)
	{
		return speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &level) == 0;
	}
	return false;
}

int CSpeexPreprocess::GetDenoiseLevel()
{
	int n = 0;
	if (m_pState)
	{
		speex_preprocess_ctl(m_pState, SPEEX_PREPROCESS_GET_NOISE_SUPPRESS, &n);
	}
	return n;
}

void CSpeexPreprocess::RunPreprocess(short* buffer)
{
	speex_preprocess_run(m_pState, buffer);
}