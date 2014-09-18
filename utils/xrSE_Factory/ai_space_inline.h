////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_space_inline.h
//	Created 	: 12.11.2003
//  Modified 	: 18.06.2004
//	Author		: Dmitriy Iassenev
//	Description : AI space class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptEngine				&CAI_Space::script_engine	() const
{
	VERIFY					(m_script_engine);
	return					(*m_script_engine);
}

IC	CAI_Space					&ai							()
{
	if (!g_ai_space) {
		g_ai_space			= xr_new<CAI_Space>();
		g_ai_space->init	();
	}
	return					(*g_ai_space);
}