////////////////////////////////////////////////////////////////////////////
//	Module 		: script_process_inline.h
//	Created 	: 19.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script process class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	const CScriptProcess::SCRIPT_REGISTRY &CScriptProcess::scripts	() const
{
	return	(m_scripts);
}

IC	shared_str CScriptProcess::name									() const
{
	return	(m_name);
}
