#pragma once

struct VoicePacket
{
public:
	static constexpr size_t MAX_DATA_SIZE = 512;

	BYTE data[MAX_DATA_SIZE];
	u32 length;
	DWORD time;
};