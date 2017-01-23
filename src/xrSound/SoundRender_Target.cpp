#include "stdafx.h"
#pragma hdrstop

#include "soundrender_target.h"
#include "soundrender_core.h"
#include "soundrender_emitter.h"
#include "soundrender_source.h"

CSoundRender_Target::CSoundRender_Target(void)
{
	m_pEmitter		= 0;
	rendering		= FALSE;
	wave			= 0;
}

CSoundRender_Target::~CSoundRender_Target(void)
{
	VERIFY				(wave==0);
}

BOOL CSoundRender_Target::_initialize()
{
	/*
	// Calc format
	wfx.wFormatTag			= WAVE_FORMAT_PCM;
	wfx.nChannels			= 2; //1;
	wfx.nSamplesPerSec		= SoundRender->wfm.nSamplesPerSec;
	wfx.wBitsPerSample		= 16;
	wfx.nBlockAlign			= wfx.nChannels * wfx.wBitsPerSample / 8;
	wfx.nAvgBytesPerSec		= wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize				= 0;

	*/	
/*
		wfx.wFormatTag=WAVE_FORMAT_PCM;
		wfx.nChannels=2;
		wfx.wBitsPerSample=16;
		wfx.nBlockAlign=4;
		wfx.nSamplesPerSec=44100;
		wfx.nAvgBytesPerSec=176400;
		wfx.cbSize=0;
*/
    return					TRUE;
}

void	CSoundRender_Target::start(CSoundRender_Emitter* E)
{
	R_ASSERT		(E);

	// *** Initial buffer startup ***
	// 1. Fill parameters
	// 4. Load 2 blocks of data (as much as possible)
	// 5. Deferred-play-signal (emitter-exist, rendering-false)
	m_pEmitter		= E;
	rendering		= FALSE;
	//attach		();
}

void	CSoundRender_Target::render			()
{
	rendering		= TRUE;
}

void	CSoundRender_Target::stop			()
{
	dettach			();
	m_pEmitter		= NULL;
	rendering		= FALSE;
}

void	CSoundRender_Target::rewind			()
{
	R_ASSERT		(rendering);
}

void	CSoundRender_Target::update			()
{
	R_ASSERT		(m_pEmitter);
}

void	CSoundRender_Target::fill_parameters()
{
	VERIFY			(m_pEmitter);
//.	if (pEmitter->b2D){
//.		pEmitter->set_position(SoundRender->listener_position());
//.	}
}

extern int		ov_seek_func	(void *datasource, s64 offset, int whence);
extern size_t	ov_read_func	(void *ptr, size_t size, size_t nmemb, void *datasource);
extern int		ov_close_func	(void *datasource);
extern long		ov_tell_func	(void *datasource);

void	CSoundRender_Target::attach()
{
	VERIFY			(0==wave);
	VERIFY			(m_pEmitter);
	ov_callbacks ovc= {ov_read_func,ov_seek_func,ov_close_func,ov_tell_func};
	wave			= FS.r_open		(m_pEmitter->source()->pname.c_str()); 
	R_ASSERT3		(wave&&wave->length(),"Can't open wave file:", m_pEmitter->source()->pname.c_str());
 	ov_open_callbacks(wave,&ovf,NULL,0,ovc);
	VERIFY			(0!=wave);
}

void	CSoundRender_Target::dettach()
{
	if (wave)		{
		ov_clear		(&ovf);
		FS.r_close		(wave);
	}
}

