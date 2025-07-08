#pragma once
#include "speex/speex_preprocess.h"

class CSpeexPreprocess
{
public:
	CSpeexPreprocess(int sampleRate, int samplesPerBuffer);
	~CSpeexPreprocess();

	bool EnableAGC(bool enable);
	bool IsAGCEnabled();

	bool IsDenoiseEnabled();
	bool EnableDenoise(bool enable);

	int GetDenoiseLevel();
	bool SetDenoiseLevel(int level);

	void RunPreprocess(short* buffer);

private:
	SpeexPreprocessState* m_pState = nullptr;
};