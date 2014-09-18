////////////////////////////////////////////////////////////////////////////
//	Module 		: script_value.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script value
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_space.h"

class CSE_Abstract;

class CScriptValue {
protected:
	luabind::object			m_object;
	shared_str					m_name;

public:
	IC						CScriptValue	(luabind::object object, LPCSTR name);
	virtual	void			assign			() = 0;
	IC		shared_str			name			();
};

#include "script_value_inline.h"