#include "stdafx.h"
#pragma hdrstop

#include "soundrender_TargetA.h"
#include "soundrender_emitter.h"
#include "soundrender_source.h"

xr_vector<u8> g_target_temp_data;

CSoundRender_TargetA::CSoundRender_TargetA():CSoundRender_Target()
{
    cache_gain			= 0.f;
    cache_pitch			= 1.f;
    pSource				= 0;
}

CSoundRender_TargetA::~CSoundRender_TargetA()
{
}

BOOL	CSoundRender_TargetA::_initialize		()
{
	inherited::_initialize();
    // initialize buffer
	A_CHK(alGenBuffers	(sdef_target_count, pBuffers));	
    alGenSources		(1, &pSource);
    ALenum error		= alGetError();
    if (AL_NO_ERROR==error){
        A_CHK(alSourcei	(pSource, AL_LOOPING, AL_FALSE));
        A_CHK(alSourcef	(pSource, AL_MIN_GAIN, 0.f));
        A_CHK(alSourcef	(pSource, AL_MAX_GAIN, 1.f));
        A_CHK(alSourcef	(pSource, AL_GAIN, 	cache_gain));
        A_CHK(alSourcef	(pSource, AL_PITCH,	cache_pitch));
        return			TRUE;
    }else{
    	Msg				("! sound: OpenAL: Can't create source. Error: %s.",(LPCSTR)alGetString(error));
        return 			FALSE;
    }
}

void	CSoundRender_TargetA::_destroy		()
{
	// clean up target
	if (alIsSource(pSource))	
		alDeleteSources	(1, &pSource);
	A_CHK(alDeleteBuffers		(sdef_target_count, pBuffers));
}

void CSoundRender_TargetA::_restart()
{
	_destroy();
	_initialize();
}

void CSoundRender_TargetA::start			(CSoundRender_Emitter* E)
{
    inherited::start(E);

	// Calc storage
	buf_block		= sdef_target_block * E->source()->m_wformat.nAvgBytesPerSec / 1000;
    g_target_temp_data.resize(buf_block);
}

void	CSoundRender_TargetA::render		()
{
	for (u32 buf_idx=0; buf_idx<sdef_target_count; buf_idx++)
		fill_block	(pBuffers[buf_idx]);

	A_CHK			(alSourceQueueBuffers	(pSource, sdef_target_count, pBuffers));	
	A_CHK			(alSourcePlay			(pSource));

    inherited::render();
}

void	CSoundRender_TargetA::stop			()
{
	if (rendering)
	{
		A_CHK		(alSourceStop(pSource));
		A_CHK		(alSourcei	(pSource, AL_BUFFER,   NULL));
		A_CHK		(alSourcei	(pSource, AL_SOURCE_RELATIVE,	TRUE));
	}
    inherited::stop	();
}

void	CSoundRender_TargetA::rewind			()
{
	inherited::rewind();

	A_CHK			(alSourceStop(pSource));
	A_CHK			(alSourcei	(pSource, AL_BUFFER,   NULL));
	for (u32 buf_idx=0; buf_idx<sdef_target_count; buf_idx++)
		fill_block	(pBuffers[buf_idx]);
	A_CHK			(alSourceQueueBuffers	(pSource, sdef_target_count, pBuffers));	
	A_CHK			(alSourcePlay			(pSource));
}

void	CSoundRender_TargetA::update			()
{
	inherited::update();

	ALint			processed;
    // Get status
    A_CHK			(alGetSourcei(pSource, AL_BUFFERS_PROCESSED, &processed));

    if (processed > 0)
	{
        while (processed)
		{
			ALuint			BufferID;
            A_CHK			(alSourceUnqueueBuffers(pSource, 1, &BufferID));
            fill_block		(BufferID);
            A_CHK			(alSourceQueueBuffers(pSource, 1, &BufferID));
            --processed;
        }
    }else{ 
    	// processed == 0
        // check play status -- if stopped then queue is not being filled fast enough
        ALint		state;
	    A_CHK		(alGetSourcei(pSource, AL_SOURCE_STATE, &state));
        if (state != AL_PLAYING)
		{
//			Log		("Queuing underrun detected.");
			A_CHK	(alSourcePlay(pSource));
        }
    }
}

void	CSoundRender_TargetA::fill_parameters()
{
	CSoundRender_Emitter* SE = m_pEmitter; VERIFY(SE);

	inherited::fill_parameters();

    // 3D params
	VERIFY2(m_pEmitter, SE->source()->file_name());
    A_CHK(alSourcef	(pSource, AL_REFERENCE_DISTANCE, 	m_pEmitter->p_source.min_distance));

	VERIFY2(m_pEmitter,SE->source()->file_name());
    A_CHK(alSourcef	(pSource, AL_MAX_DISTANCE, 			m_pEmitter->p_source.max_distance));

	VERIFY2(m_pEmitter,SE->source()->file_name                                       ());
	A_CHK(alSource3f(pSource, AL_POSITION,	 			m_pEmitter->p_source.position.x,m_pEmitter->p_source.position.y,-m_pEmitter->p_source.position.z));

	VERIFY2(m_pEmitter,SE->source()->file_name());
    A_CHK(alSourcei	(pSource, AL_SOURCE_RELATIVE,		m_pEmitter->b2D));

	A_CHK(alSourcef	(pSource, AL_ROLLOFF_FACTOR,		psSoundRolloff));

	VERIFY2(m_pEmitter,SE->source()->file_name());
    float	_gain	= m_pEmitter->smooth_volume;			clamp	(_gain,EPS_S,1.f);
    if (!fsimilar(_gain,cache_gain, 0.01f))
	{
        cache_gain	= _gain;
        A_CHK(alSourcef	(pSource, AL_GAIN,				_gain));
    }

	VERIFY2(m_pEmitter,SE->source()->file_name());
    float	_pitch	= m_pEmitter->p_source.freq;			clamp	(_pitch,EPS_L,2.f);
    if (!fsimilar(_pitch,cache_pitch)){
        cache_pitch	= _pitch;
        A_CHK(alSourcef	(pSource, AL_PITCH,				_pitch));
    }
	VERIFY2(m_pEmitter,SE->source()->file_name());
}

void	CSoundRender_TargetA::fill_block	(ALuint BufferID)
{
	R_ASSERT			(m_pEmitter);

	m_pEmitter->fill_block(&g_target_temp_data.front(),buf_block);
	ALuint format 		= (m_pEmitter->source()->m_wformat.nChannels==1)?AL_FORMAT_MONO16:AL_FORMAT_STEREO16;
    A_CHK				(alBufferData(BufferID, format, &g_target_temp_data.front(), buf_block, m_pEmitter->source()->m_wformat.nSamplesPerSec));
}
void CSoundRender_TargetA::source_changed()
{
	dettach();
	attach();
}