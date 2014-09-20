#ifndef SoundRender_SourceH
#define SoundRender_SourceH
#pragma once

#include "soundrender_cache.h"

// refs
struct OggVorbis_File;

class XRSOUND_EDITOR_API 	CSoundRender_Source	: public CSound_source
{
public:
	shared_str				pname;
	shared_str				fname;
	cache_cat				CAT;

	float					fTimeTotal;
	u32						dwBytesTotal;

	WAVEFORMATEX			m_wformat; //= SoundRender->wfm;


	float					m_fBaseVolume;
	float					m_fMinDist;
	float					m_fMaxDist;
	float					m_fMaxAIDist;
	u32						m_uGameType;
private:
	void 					i_decompress_fr			(OggVorbis_File* ovf, char* dest, u32 size);    
	void					LoadWave 				(LPCSTR name);
public:
							CSoundRender_Source		();
							~CSoundRender_Source	();

	void					load					(LPCSTR name);
    void					unload					();
	void					decompress				(u32 line, OggVorbis_File* ovf);
	
	virtual	float			length_sec				() const	{return fTimeTotal;}
	virtual u32				game_type				() const	{return m_uGameType;}
	virtual LPCSTR			file_name				() const	{return *fname;}
	virtual float			base_volume				() const	{return m_fBaseVolume;}
	virtual u16				channels_num			() const	{return m_wformat.nChannels;}
	virtual u32				bytes_total				() const	{return dwBytesTotal;}
};
#endif