////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_velocity_holder_inline.h
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Stalker velocity holder inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CStalkerVelocityHolder &stalker_velocity_holder	()
{
	if (g_stalker_velocity_holder) 
		return					(*g_stalker_velocity_holder);

	g_stalker_velocity_holder	= xr_new<CStalkerVelocityHolder>();
	return						(*g_stalker_velocity_holder);
}
