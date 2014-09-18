////////////////////////////////////////////////////////////////////////////
//	Module 		: script_callback_ex_void.h
//	Created 	: 06.02.2004
//  Modified 	: 11.01.2005
//	Author		: Sergey Zhemeitsev and Dmitriy Iassenev
//	Description : Script callbacks with return value but specialized with void
////////////////////////////////////////////////////////////////////////////

#pragma once

template <>
class CScriptCallbackEx<void> : public CScriptCallbackEx_<void> {
public:
#	define	macros_return_operator
#	undef	SCRIPT_CALLBACK_EX_GENERATORS
#	include "script_callback_ex_generators.h"
#	undef	macros_return_operator
};