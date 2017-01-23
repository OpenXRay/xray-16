////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value_inline.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CScriptValue::CScriptValue	(luabind::object object, LPCSTR name)
{
	m_object	= object;
	m_name		= name;
}

IC	shared_str	CScriptValue::name		()
{
	return		(m_name);
}
