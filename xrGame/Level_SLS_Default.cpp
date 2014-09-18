#include "stdafx.h"
#include "level.h"
#include "xrserver.h"

void	CLevel::SLS_Default				()					// Default/Editor Load
{
	// Signal main actor spawn
/*
	LPCSTR		s_cmd			= Engine.Params;
	string64	s_name			= "actor";
	if (strstr(s_cmd,"-actor "))	{
		sscanf(strstr(s_cmd,"-actor ")+xr_strlen("-actor "),"%s",s_name);
		ph_world			= xr_new<CPHWorld> ();
		ph_world->Create	();
	}
	g_cl_Spawn			(s_name, -1, 0, 0, 0);
*/
	if (Server)	Server->SLS_Default		();
}
