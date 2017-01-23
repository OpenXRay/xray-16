#ifndef SoundRender_CoreDH
#define SoundRender_CoreDH
#pragma once

#include "SoundRender_Core.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <dsound.h>
#pragma warning(pop)

#define _RELEASE(x)		{ if(x) { (x)->Release();       (x)=NULL; } }

class CSoundRender_CoreD: public CSoundRender_Core
{
	typedef CSoundRender_Core inherited;
	struct SListener 
	{
		u32				dwSize;
		Fvector			vPosition;
		Fvector			vVelocity;
		Fvector			vOrientFront; 
		Fvector			vOrientTop; 
		float			fDistanceFactor;
		float			fRolloffFactor;
		float			fDopplerFactor;
	};
    BOOL 				EAXQuerySupport			(const GUID* guid, u32 prop);
	BOOL 				EAXTestSupport			(BOOL bDeferred);
public:
	// DSound interface
	IDirectSound8*				pDevice;		// The device itself
	IDirectSoundBuffer*			pBuffer;		// The primary buffer (mixer destination)
	IDirectSound3DListener8*	pListener;
	LPKSPROPERTYSET				pExtensions;
	DSCAPS						dsCaps;
	SListener					Listener;
private:
	virtual void			update_listener			(const Fvector& P, const Fvector& D, const Fvector& N, float dt);
	virtual void			i_eax_set				(const GUID* guid, u32 prop, void* val, u32 sz);
	virtual void			i_eax_get				(const GUID* guid, u32 prop, void* val, u32 sz);
public:
							CSoundRender_CoreD		();
    virtual					~CSoundRender_CoreD		();

	virtual void			_initialize				( u64 window	);
	virtual void			_clear					( );

	virtual void			set_master_volume		( float f		);
    
	virtual const Fvector&	listener_position		( )				{ return Listener.vPosition; }
};
extern CSoundRender_CoreD* SoundRenderD;
#endif
