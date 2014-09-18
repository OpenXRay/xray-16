#include "stdafx.h"
#pragma hdrstop
/*
#include "soundrender_coreD.h"
#include "soundrender_targetD.h"
#include <eax/eax.h>

CSoundRender_CoreD*	SoundRenderD= 0;

CSoundRender_CoreD::CSoundRender_CoreD	():CSoundRender_Core()
{
	pDevice						= NULL;
	pBuffer						= NULL;
	pListener					= NULL;
	pExtensions					= NULL;
}

CSoundRender_CoreD::~CSoundRender_CoreD()
{
}

BOOL CSoundRender_CoreD::EAXQuerySupport(const GUID* guid, u32 prop)
{
    ULONG ulSupport = 0;
    HRESULT hr = pExtensions->QuerySupport(*guid, prop, &ulSupport);
    if (FAILED(hr)) return FALSE;
 
    if ((ulSupport&(KSPROPERTY_SUPPORT_GET|KSPROPERTY_SUPPORT_SET)) == (KSPROPERTY_SUPPORT_GET|KSPROPERTY_SUPPORT_SET))
        return TRUE;
 
    return FALSE;
}

BOOL CSoundRender_CoreD::EAXTestSupport	(BOOL bDeferred)
{
	BOOL support 	= TRUE;
    u32 ext			= bDeferred?DSPROPERTY_EAXLISTENER_DEFERRED:0;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_NONE)) 					support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_ALLPARAMETERS)) 			support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_ROOM)) 					support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_ROOMHF)) 			  	support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_ROOMROLLOFFFACTOR)) 		support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_DECAYTIME)) 				support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_DECAYHFRATIO)) 			support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_REFLECTIONS)) 			support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_REFLECTIONSDELAY)) 		support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_REVERB)) 		      	support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_REVERBDELAY)) 		    support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_ENVIRONMENT)) 		    support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_ENVIRONMENTSIZE)) 	    support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_ENVIRONMENTDIFFUSION)) 	support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF)) 		support = FALSE;
    if (!EAXQuerySupport(&DSPROPSETID_EAX_ListenerProperties, ext | DSPROPERTY_EAXLISTENER_FLAGS)) 					support = FALSE;
    return support;
}


void CSoundRender_CoreD::_initialize	(u64 window)
{
	bPresent			= FALSE;

	// DirectX device
	if( FAILED			( EAXDirectSoundCreate8( NULL, &pDevice, NULL ) ) )
		if( FAILED		( DirectSoundCreate8( NULL, &pDevice, NULL ) ) )	return;
	if( FAILED			( pDevice->SetCooperativeLevel(  (HWND)window, DSSCL_PRIORITY ) ) )	
	{
		_destroy();
		return;
	}

	// Create primary buffer.
	DSBUFFERDESC		dsbd;
	ZeroMemory			( &dsbd, sizeof( DSBUFFERDESC ) );
	dsbd.dwSize			= sizeof( DSBUFFERDESC );
	dsbd.dwFlags		= DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes	= 0;
	if( FAILED	( pDevice->CreateSoundBuffer( &dsbd, &pBuffer, NULL ) ) )
	{
		_destroy();
		return;
	}

	// Calculate primary buffer format.
	dsCaps.dwSize				= sizeof(DSCAPS);
	R_ASSERT					(pDevice);
	R_CHK						(pDevice->GetCaps (&dsCaps));

	ZeroMemory					( &wfm, sizeof( WAVEFORMATEX ) );
	wfm.nSamplesPerSec			= 44100;
	wfm.wFormatTag				= WAVE_FORMAT_PCM;
	wfm.nChannels				= (dsCaps.dwFlags&DSCAPS_PRIMARYSTEREO)?2:1;
	wfm.wBitsPerSample			= (dsCaps.dwFlags&DSCAPS_PRIMARY16BIT)?16:8;
	wfm.nBlockAlign				= wfm.wBitsPerSample / 8 * wfm.nChannels;
	wfm.nAvgBytesPerSec			= wfm.nSamplesPerSec * wfm.nBlockAlign;

	// For safety only :)
	R_CHK(pBuffer->SetFormat	(&wfm));
	R_CHK(pBuffer->Play			(0,0,DSBPLAY_LOOPING));

	// Get listener interface.
	R_CHK(pBuffer->QueryInterface( IID_IDirectSound3DListener8, (VOID**)&pListener ));
	R_ASSERT					(pListener);

    // Create property set
	{
        IDirectSoundBuffer*		pTempBuf;
        WAVEFORMATEX 			wave;
        Memory.mem_fill			(&wave, 0, sizeof(WAVEFORMATEX));
        wave.wFormatTag 		= WAVE_FORMAT_PCM;
        wave.nChannels 			= 1; 
        wave.nSamplesPerSec 	= 22050; 
        wave.wBitsPerSample 	= 16; 
        wave.nBlockAlign 		= wave.wBitsPerSample / 8 * wave.nChannels;
        wave.nAvgBytesPerSec	= wave.nSamplesPerSec * wave.nBlockAlign;

        DSBUFFERDESC 			desc;
        Memory.mem_fill			(&desc, 0, sizeof(DSBUFFERDESC));
        desc.dwSize 			= sizeof(DSBUFFERDESC); 
        desc.dwFlags 			= DSBCAPS_STATIC|DSBCAPS_CTRL3D; 
        desc.dwBufferBytes 		= 64;  
        desc.lpwfxFormat 		= &wave; 

		if (DS_OK==pDevice->CreateSoundBuffer(&desc, &pTempBuf, NULL)){
        	bEAX				= TRUE;
			if (FAILED(pTempBuf->QueryInterface(IID_IKsPropertySet, (LPVOID *)&pExtensions))){
                bEAX			= FALSE;
            }else{
            	bEAX 			= EAXTestSupport(FALSE);
            	bDeferredEAX	= EAXTestSupport(TRUE);
            }
        }
        _RELEASE				(pTempBuf);
    }

	// Initialize listener data
	Listener.dwSize				= sizeof(DS3DLISTENER);
	Listener.vPosition.set		( 0.0f, 0.0f, 0.0f );
	Listener.vVelocity.set		( 0.0f, 0.0f, 0.0f );
	Listener.vOrientFront.set	( 0.0f, 0.0f, 1.0f );
	Listener.vOrientTop.set		( 0.0f, 1.0f, 0.0f );
	Listener.fDistanceFactor	= 1.0f;
	Listener.fRolloffFactor		= DS3D_DEFAULTROLLOFFFACTOR;
	Listener.fDopplerFactor		= DS3D_DEFAULTDOPPLERFACTOR;

	Msg				        	("DirectSound: Create device successfully.");
    // inherited initialize
    inherited::_initialize		(window);

	// Pre-create targets
	CSoundRender_Target*	T	= 0;
	for (u32 tit=0; tit<u32(psSoundTargets); tit++)
	{
		T						= xr_new<CSoundRender_TargetD>();
		T->_initialize			();	
		s_targets.push_back		(T);
	}
}

void CSoundRender_CoreD::set_master_volume(float f )
{
	if		(pBuffer)	
	{
		float	_volume		= f;									clamp	(_volume,EPS_S,1.f);
		s32		hw_volume	= iFloor	(7000.f*logf(_volume)/5.f);	clamp	(hw_volume,DSBVOLUME_MIN,DSBVOLUME_MAX);
		pBuffer->SetVolume	(hw_volume);
	}
}

void CSoundRender_CoreD::_clear	()
{
	inherited::_clear			();

    // remove targets
	CSoundRender_Target*	T	= 0;
	for (u32 tit=0; tit<s_targets.size(); tit++)
	{
		T						= s_targets[tit];
		T->_destroy				();
        xr_delete				(T);
	}
    // cleanup dx
	_RELEASE					( pExtensions	);
	_RELEASE					( pListener		);
	_RELEASE					( pBuffer		);
	_RELEASE					( pDevice		);
}

void	CSoundRender_CoreD::i_eax_set			(const GUID* guid, u32 prop, void* val, u32 sz)
{
	VERIFY	(pExtensions);
	R_CHK	(pExtensions->Set		(*guid, prop, NULL, 0, val, sz));
}
void	CSoundRender_CoreD::i_eax_get			(const GUID* guid, u32 prop, void* val, u32 sz)
{
	unsigned long total_bytes;
	VERIFY	(pExtensions);
	R_CHK	(pExtensions->Get		(*guid, prop, NULL, 0, val, sz, &total_bytes));
}

void CSoundRender_CoreD::update_listener( const Fvector& P, const Fvector& D, const Fvector& N, float dt )
{
	inherited::update_listener(P,D,N,dt);
	Listener.vVelocity.sub			(P, Listener.vPosition );
	Listener.vVelocity.div			(dt);
	if (!Listener.vPosition.similar(P)){
		Listener.vPosition.set		(P);
		bListenerMoved				= TRUE;
	}
	//last_pos						= P;
	Listener.vOrientFront.set		(D);
	Listener.vOrientTop.set			(N);
	Listener.fDopplerFactor			= EPS_S;
	Listener.fRolloffFactor			= psSoundRolloff;
	// apply listener params
    pListener->SetAllParameters		((DS3DLISTENER*)&Listener, DS3D_DEFERRED );
    // commit deffered settings
    pListener->CommitDeferredSettings	();
}
*/
