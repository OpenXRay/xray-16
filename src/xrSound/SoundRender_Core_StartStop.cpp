#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Core.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Target.h"
#include "SoundRender_Source.h"

void	CSoundRender_Core::i_start		(CSoundRender_Emitter* E)
{
	R_ASSERT	(E);

	// Search lowest-priority target
	float					Ptest	= E->priority	();
	float					Ptarget	= flt_max;
	CSoundRender_Target*	T		= 0;
	for (u32 it=0; it<s_targets.size(); it++)
	{
		CSoundRender_Target*	Ttest	= s_targets[it];
		if (Ttest->priority < Ptarget)
		{
			T		= Ttest;
			Ptarget	= Ttest->priority;
		}
	}

	// Stop currently playing
	if (T->get_emitter())
		T->get_emitter()->cancel();

	// Associate
	E->target			= T;
	E->target->start	(E);
	T->priority			= Ptest;
}

void	CSoundRender_Core::i_stop		(CSoundRender_Emitter* E)
{
	// Msg					("- %10s : %3d[%1.4f] : %s","i_stop",E->dbg_ID,E->priority(),E->source->fname);
	R_ASSERT			(E);
	R_ASSERT			(E == E->target->get_emitter());
	E->target->stop		();
	E->target			= NULL;
}

void	CSoundRender_Core::i_rewind		(CSoundRender_Emitter* E)
{
	// Msg					("- %10s : %3d[%1.4f] : %s","i_rewind",E->dbg_ID,E->priority(),E->source->fname);
	R_ASSERT			(E);
	R_ASSERT			(E == E->target->get_emitter());
	E->target->rewind	();
}

BOOL	CSoundRender_Core::i_allow_play	(CSoundRender_Emitter* E)
{
	// Search available target
	float	Ptest	= E->priority	();
	for (u32 it=0; it<s_targets.size(); it++)
	{
		CSoundRender_Target*	T		= s_targets	[it];
		if (T->priority<Ptest)			return TRUE;
	}
	return FALSE;
}
