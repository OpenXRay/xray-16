#pragma once

#include "../xrphysics/iphysics_scripted.h"
#include "../xrphysics/iphworld.h"
#include "script_export_space.h"

class CPHCondition;
class CPHAction;

class cphysics_world_scripted:
public cphysics_game_scripted<IPHWorld>
{
public:
					cphysics_world_scripted		(IPHWorld* imp ):cphysics_game_scripted<IPHWorld>(imp){}				
			
		float		Gravity						( )									{	return physics_impl().Gravity(); }
		void		SetGravity					( float	g )							{	return physics_impl().SetGravity( g ); }
		void		AddCall						(	CPHCondition*c, CPHAction*a )	;
DECLARE_SCRIPT_REGISTER_FUNCTION	
};

add_to_type_list(cphysics_world_scripted)
#undef script_type_list
#define script_type_list save_type_list(cphysics_world_scripted)