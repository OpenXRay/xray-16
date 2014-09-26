////////////////////////////////////////////////////////////////////////////
//	Module 		: script_flags.h
//	Created 	: 19.07.2004
//  Modified 	: 19.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script flags
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

typedef class_exporter<Flags32> CScriptFlags;
add_to_type_list(CScriptFlags)
#undef script_type_list
#define script_type_list save_type_list(CScriptFlags)
