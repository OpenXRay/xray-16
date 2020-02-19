#pragma once

bool ReadRegistry_StrValue(const char* rKeyName, char* value);
void WriteRegistry_StrValue(const char* rKeyName, const char* value);

u32 const ReadRegistry_BinaryValue(const char* rKeyName, u8* buffer_dest, u32 const buffer_size);
void WriteRegistry_BinaryValue(const char* rKeyName, u8 const* buffer_src, u32 const buffer_size);

void ReadRegistry_DWValue(const char* rKeyName, unsigned int& value);
void WriteRegistry_DWValue(const char* rKeyName, const unsigned int& value);
