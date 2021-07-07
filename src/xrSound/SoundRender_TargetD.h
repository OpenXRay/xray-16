#ifndef SoundRender_TargetDH
#define SoundRender_TargetDH
#pragma once

#include "soundrender_Target.h"
#include "soundrender_CoreD.h"

class CSoundRender_TargetD: public CSoundRender_Target
{
	typedef CSoundRender_Target	inherited;

	IDirectSoundBuffer*			pBuffer_base;
	IDirectSoundBuffer8*		pBuffer;
	IDirectSound3DBuffer8*		pControl;

	BOOL						bDX7;

	u32							buf_size;		// bytes
	u32							buf_block;

	s32							cache_hw_volume;
	s32							cache_hw_freq;
	u32							pos_write;		// bytes
private:
	void						fill_block				();
	u32							calc_interval			(u32 ptr);
public:
								CSoundRender_TargetD	();
	virtual 					~CSoundRender_TargetD	();

	virtual BOOL				_initialize				();
	virtual void				_destroy				();

	virtual void				start					(CSoundRender_Emitter* E);
	virtual void				render					();
	virtual void				rewind					();
	virtual void				stop					();
	virtual void				update					();
	virtual void				fill_parameters			();
};
#endif