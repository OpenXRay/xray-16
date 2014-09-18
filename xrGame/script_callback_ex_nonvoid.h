////////////////////////////////////////////////////////////////////////////
//	Module 		: script_callback_ex_non_void.h
//	Created 	: 06.02.2004
//  Modified 	: 11.01.2005
//	Author		: Sergey Zhemeitsev and Dmitriy Iassenev
//	Description : Script callbacks with return value
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename _return_type>
class CScriptCallbackEx : public CScriptCallbackEx_<_return_type> {
public:
#	define	macros_return_operator		return return_type
#	undef	SCRIPT_CALLBACK_EX_GENERATORS
#	include "script_callback_ex_generators.h"
#	undef	macros_return_operator
};