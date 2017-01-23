#include "stdafx.h"
#pragma hdrstop
/*
#include "soundrender_targetD.h"
#include "soundrender.h"
//. #include "soundrender_coreD.h"
#include "soundrender_emitter.h"

CSoundRender_TargetD::CSoundRender_TargetD():CSoundRender_Target()
{
	pBuffer_base	= NULL;
	pBuffer			= NULL;
	pControl		= NULL;

	cache_hw_volume	= DSBVOLUME_MIN;
	cache_hw_freq	= 11025;

	pos_write		= 0;
}

CSoundRender_TargetD::~CSoundRender_TargetD()
{
}

BOOL CSoundRender_TargetD::_initialize	()
{
	inherited::_initialize();

	// Calc storage
	buf_size				= sdef_target_size*wfx.nAvgBytesPerSec/1000;
	buf_block				= sdef_target_block*wfx.nAvgBytesPerSec/1000;

	// Fill caps structure
	DSBUFFERDESC	dsBD	= {0};
	dsBD.dwSize				= sizeof(dsBD);
	dsBD.dwFlags			=
		DSBCAPS_CTRL3D | 
		DSBCAPS_CTRLFREQUENCY | 
		DSBCAPS_CTRLVOLUME |
		DSBCAPS_GETCURRENTPOSITION2 |
		(psSoundFlags.test(ss_Hardware) 	? 0 				: (DSBCAPS_LOCSOFTWARE));
	dsBD.dwBufferBytes		= buf_size;
	dsBD.dwReserved			= 0;
	dsBD.lpwfxFormat		= &wfx;

	switch (psSoundModel) 
	{
	case sq_DEFAULT:	dsBD.guid3DAlgorithm = DS3DALG_DEFAULT; 			break;
	case sq_NOVIRT:		dsBD.guid3DAlgorithm = DS3DALG_NO_VIRTUALIZATION; 	break;
	case sq_LIGHT:		dsBD.guid3DAlgorithm = DS3DALG_HRTF_LIGHT;			break;
	case sq_HIGH:		dsBD.guid3DAlgorithm = DS3DALG_HRTF_FULL;			break;
	default:			FATAL("Unknown 3D-ref_sound algorithm");			break;
	}
    if (psSoundFlags.test(ss_Hardware)) 
    	dsBD.guid3DAlgorithm = DS3DALG_HRTF_FULL;

	// Create
	bDX7				= FALSE;
	R_CHK	(SoundRenderD->pDevice->CreateSoundBuffer(&dsBD, &pBuffer_base, NULL));
	R_CHK	(pBuffer_base->QueryInterface(IID_IDirectSoundBuffer8,(void **)&pBuffer));
	R_CHK	(pBuffer->QueryInterface(IID_IDirectSound3DBuffer8,	(void **)&pControl));
	R_ASSERT(pBuffer_base && pBuffer && pControl);

	R_CHK	(pControl->SetConeAngles		(DS3D_DEFAULTCONEANGLE,DS3D_DEFAULTCONEANGLE,DS3D_DEFERRED));
	R_CHK	(pControl->SetConeOrientation	(0,0,1,DS3D_DEFERRED));
	R_CHK	(pControl->SetConeOutsideVolume	(0,DS3D_DEFERRED));
	R_CHK	(pControl->SetVelocity			(0,0,0,DS3D_DEFERRED));

    return TRUE;
}

void	CSoundRender_TargetD::_destroy		()
{
	_RELEASE(pControl);
	_RELEASE(pBuffer);
	_RELEASE(pBuffer_base);
}

void	CSoundRender_TargetD::start			(CSoundRender_Emitter* E)
{
    inherited::start(E);
	pos_write		= 0;
}

void	CSoundRender_TargetD::render			()
{
	fill_block		();
	fill_block		();

    R_CHK			(pBuffer->SetCurrentPosition	(0));
	HRESULT _hr		= pBuffer->Play(0,0,DSBPLAY_LOOPING);
	if (DSERR_BUFFERLOST==_hr)	{
		R_CHK(pBuffer->Restore());
		R_CHK(pBuffer->Play(0,0,DSBPLAY_LOOPING));
	}else{
		R_CHK		(_hr);
	}
    inherited::render();
}

void	CSoundRender_TargetD::stop			()
{
	if (rendering){
		R_CHK		(pBuffer->Stop());
		R_CHK		(pControl->SetMode(DS3DMODE_HEADRELATIVE,DS3D_DEFERRED));
//		R_CHK		(pControl->SetMode(DS3DMODE_DISABLE,DS3D_DEFERRED));
	}
    inherited::stop	();
}

void	CSoundRender_TargetD::rewind			()
{
	inherited::rewind();

	R_CHK			(pBuffer->Stop	());
	pos_write		= 0;
	fill_block		();
	fill_block		();

	R_CHK			(pBuffer->SetCurrentPosition	(0));
	HRESULT _hr		= pBuffer->Play(0,0,DSBPLAY_LOOPING);
	if (DSERR_BUFFERLOST==_hr)	{
		R_CHK(pBuffer->Restore());
		R_CHK(pBuffer->Play(0,0,DSBPLAY_LOOPING));
	}else{
		R_CHK		(_hr);
	}
}

u32		CSoundRender_TargetD::calc_interval	(u32 ptr)
{
	u32		norm_ptr	= ptr%buf_size;
	u32		range		= norm_ptr/buf_block;
	return	range;
}

void	CSoundRender_TargetD::update			()
{
	inherited::update();

	// Analyze if we really need more data to stream them ahead
	u32				cursor_write;
	R_CHK			(pBuffer->GetCurrentPosition(0,LPDWORD(&cursor_write)));
	u32				r_write		= calc_interval(pos_write);
	u32				r_cursor	= (calc_interval(cursor_write)+1)%sdef_target_count;
	if (r_write==r_cursor)	fill_block	();     
//	Msg				("write: 0x%8x",cursor_write);
}

void	CSoundRender_TargetD::fill_parameters()
{
	inherited::fill_parameters();

	// 1. Set 3D params (including mode)
	{
		Fvector&			p_pos	= pEmitter->p_source.position;

		R_CHK(pControl->SetMode			(pEmitter->b2D ? DS3DMODE_HEADRELATIVE : DS3DMODE_NORMAL,DS3D_DEFERRED));
		R_CHK(pControl->SetMinDistance	(pEmitter->p_source.min_distance,	DS3D_DEFERRED));
		R_CHK(pControl->SetMaxDistance	(pEmitter->p_source.max_distance,	DS3D_DEFERRED));
		R_CHK(pControl->SetPosition		(p_pos.x,p_pos.y,p_pos.z,			DS3D_DEFERRED));
	}
	
	// 2. Set 2D params (volume, freq) + position(rewind)
	{
		float	_volume				= pEmitter->smooth_volume;				clamp	(_volume,EPS_S,1.f);
		s32		hw_volume			= iFloor	(7000.f*logf(_volume)/5.f);	clamp	(hw_volume,DSBVOLUME_MIN,DSBVOLUME_MAX);
		if (_abs(hw_volume-cache_hw_volume)>50){
			cache_hw_volume			= hw_volume;
			R_CHK(pBuffer->SetVolume(hw_volume));
		}

		float	_freq				= pEmitter->p_source.freq;
		s32		hw_freq				= iFloor	(_freq * float(wfx.nSamplesPerSec) + EPS);
		if (_abs(hw_freq-cache_hw_freq)>50)	{
			cache_hw_freq			= hw_freq;
			s32		hw_freq_set		= hw_freq;
			clamp	(hw_freq_set,s32(SoundRenderD->dsCaps.dwMinSecondarySampleRate),s32(SoundRenderD->dsCaps.dwMaxSecondarySampleRate));
			R_CHK	(pBuffer->SetFrequency	( hw_freq_set	));
		}
	}
}

void	CSoundRender_TargetD::fill_block		()
{
#pragma todo("check why pEmitter is NULL")
	if (0==pEmitter)					return;

	// Obtain memory address of write block. This will be in two parts if the block wraps around.
    LPVOID			ptr1, ptr2;
    u32				bytes1,bytes2;
    R_CHK			(pBuffer->Lock(pos_write%buf_size, buf_block, &ptr1, LPDWORD(&bytes1), &ptr2, LPDWORD(&bytes2), 0));
	R_ASSERT		(0==ptr2 && 0==bytes2);

	// Copy data (and clear the end)
	pEmitter->fill_block(ptr1,buf_block);
	pos_write		+= buf_block;

	// wrap around for the sake of sanity
	// ??? is it possible to do this at every iteration ???
	if (pos_write > (2ul<<24ul))	pos_write	%= buf_size;	

	R_CHK			(pBuffer->Unlock(ptr1, bytes1, ptr2, bytes2));
}
*/
