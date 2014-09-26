////////////////////////////////////////////////////////////////////////////
//	Module 		: script_reader.h
//	Created 	: 05.10.2004
//  Modified 	: 05.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Script reader
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

typedef class_exporter<IReader> CScriptReader;
add_to_type_list(CScriptReader)
#undef script_type_list
#define script_type_list save_type_list(CScriptReader)
