////////////////////////////////////////////////////////////////////////////
//	Module 		: script_rtoken_list.h
//	Created 	: 16.07.2004
//  Modified 	: 16.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script rtoken list class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

class CScriptRTokenList {
public:
	typedef xr_vector<shared_str> RTOKEN_LIST;
protected:
	RTOKEN_LIST			m_values;

public:
	IC		void		add		(LPCSTR value);
	IC		void		remove	(u32 index);
	IC		LPCSTR		get		(u32 index);
	IC		u32			size	();
	IC		void		clear	();
	IC		RTOKEN_LIST	&tokens	();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptRTokenList)
#undef script_type_list
#define script_type_list save_type_list(CScriptRTokenList)

#include "script_rtoken_list_inline.h"