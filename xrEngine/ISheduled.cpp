#include "stdafx.h"
#include "xrSheduler.h"
#include "xr_object.h"

ISheduled::ISheduled	()	
{
	shedule.t_min		= 20;
	shedule.t_max		= 1000;
	shedule.b_locked	= FALSE;
#ifdef DEBUG
	dbg_startframe		= 1;
	dbg_update_shedule	= 0;
#endif
}

extern		BOOL		g_bSheduleInProgress;
ISheduled::~ISheduled	()
{
	VERIFY2				(
		!Engine.Sheduler.Registered(this),
		make_string("0x%08x : %s",this,*shedule_Name())
	);

	// sad, but true
	// we need this to become MASTER_GOLD
#ifndef DEBUG
	Engine.Sheduler.Unregister				(this);
#endif // DEBUG
}

void	ISheduled::shedule_register			()
{
	Engine.Sheduler.Register				(this);
}

void	ISheduled::shedule_unregister		()
{
	Engine.Sheduler.Unregister				(this);
}

void	ISheduled::shedule_Update			(u32 dt)
{
#ifdef DEBUG
	if (dbg_startframe==dbg_update_shedule)	
	{
		LPCSTR		name	= "unknown";
		CObject*	O		= dynamic_cast<CObject*>	(this);
		if			(O)		name	= *O->cName();
		Debug.fatal	(DEBUG_INFO,"'shedule_Update' called twice per frame for %s",name);
	}
	dbg_update_shedule	= dbg_startframe;
#endif
}
