#pragma once
#include "al.h"
#include "alc.h"
#include "ISoundRecorder.h"

class CSpeexPreprocess;
class CVoicePacketsPacker;

class CSoundRecorderA : public ISoundRecorder
{
public:
	CSoundRecorderA(ALuint sampleRate, ALenum format, ALuint samplesPerBuffer);
	~CSoundRecorderA();

	bool Init(CVoicePacketsPacker* packetsPacker);
	void Destroy();

	virtual bool IsStarted() { return m_started; }

	virtual void Start();
	virtual void Stop();

	void Update();

private:
	void ChangeGain(ALbyte* buffer, ALint length);

private:
	ALuint m_sampleRate;
	ALenum m_format;
	ALint m_samplesPerBuffer;

	ALuint m_bytesPerSample;

	ALbyte* m_buffer;
	bool m_started = false;

	CSpeexPreprocess* m_speexPreprocess = nullptr;
	CVoicePacketsPacker* m_packetsPacker = nullptr;

	ALCdevice* m_pCaptureDevice = nullptr;
};