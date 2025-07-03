#pragma once

class ISoundRecorder
{
public:
	virtual bool IsStarted() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
};