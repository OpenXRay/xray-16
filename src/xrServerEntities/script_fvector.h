////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fvector.h
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float vector
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

typedef class_exporter<Fvector> CScriptFvector;
add_to_type_list(CScriptFvector)
#undef script_type_list
#define script_type_list save_type_list(CScriptFvector)
