#include "stdafx.h"
#pragma hdrstop

#include "iniStreamImpl.h"

LPCSTR SIniFileStream::gen_name()
{
	++counter;
	sprintf(tmp_buff,"%06d",counter);
	return tmp_buff;
}

void SIniFileStream::w_float( float a)
{
	ini->w_float(sect.c_str(),gen_name(),a);
}

void SIniFileStream::w_vec3( const Fvector& a)
{
	ini->w_fvector3(sect.c_str(),gen_name(),a);
}

void SIniFileStream::w_vec4( const Fvector4& a)
{
	ini->w_fvector4(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_u64( u64 a)
{
	ini->w_u64(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_s64( s64 a)
{
	ini->w_s64(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_u32( u32 a)
{
	ini->w_u32(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_s32( s32 a)
{
	ini->w_s32(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_u16( u16 a)
{
	ini->w_u16(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_s16( s16 a)
{
	ini->w_s16(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_u8( u8 a)
{
	ini->w_u8(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_s8( s8 a)
{
	ini->w_s8(sect.c_str(),gen_name(),a);
}
void SIniFileStream::w_stringZ( LPCSTR S)
{
	string4096		buff;
	xr_sprintf	    (buff, sizeof(buff), "\"%s\"",(S)?S:"");
	ini->w_string(sect.c_str(),gen_name(),buff);
}

void SIniFileStream::r_vec3(Fvector& A)
{
	A = ini->r_fvector3(sect.c_str(),gen_name());
}
void SIniFileStream::r_vec4(Fvector4& A)
{
	A = ini->r_fvector4(sect.c_str(),gen_name());
}
void SIniFileStream::r_float(float& A)
{
	A = ini->r_float(sect.c_str(),gen_name());
}
void SIniFileStream::r_u8(u8& A)
{
	A = ini->r_u8(sect.c_str(),gen_name());
}
void SIniFileStream::r_u16(u16& A)
{
	A = ini->r_u16(sect.c_str(),gen_name());
}
void SIniFileStream::r_u32(u32& A)
{
	A = ini->r_u32(sect.c_str(),gen_name());
}
void SIniFileStream::r_u64(u64& A)
{
	A = ini->r_u64(sect.c_str(),gen_name());
}

void SIniFileStream::r_s8(s8& A)
{
	A = ini->r_s8(sect.c_str(),gen_name());
}
void SIniFileStream::r_s16(s16& A)
{
	A = ini->r_s16(sect.c_str(),gen_name());
}
void SIniFileStream::r_s32(s32& A)
{
	A = ini->r_s32(sect.c_str(),gen_name());
}
void SIniFileStream::r_s64(s64& A)
{
	A = ini->r_s64(sect.c_str(),gen_name());
}

void SIniFileStream::r_string(LPSTR dest, u32 dest_size)
{
    shared_str S;
	S = ini->r_string_wb(sect.c_str(),gen_name());
    R_ASSERT(dest_size>=S.size());
    xr_strcpy(dest, dest_size, S.c_str());
//.    Msg("[%s] [%s]=[%s]",sect.c_str(),tmp_buff,dest);
}

void SIniFileStream::skip_stringZ()
{
	gen_name();
}

