#pragma once

bool ReadRegistry_StrValue(LPCSTR rKeyName, char* value);
void WriteRegistry_StrValue(LPCSTR rKeyName, const char* value);

u32 const ReadRegistry_BinaryValue(LPCSTR rKeyName, u8* buffer_dest, u32 const buffer_size);
void WriteRegistry_BinaryValue(LPCSTR rKeyName, u8 const* buffer_src, u32 const buffer_size);

void ReadRegistry_DWValue(LPCSTR rKeyName, u32& value);
void WriteRegistry_DWValue(LPCSTR rKeyName, const u32& value);
