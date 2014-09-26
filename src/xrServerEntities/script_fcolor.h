////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fcolor.h
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float color
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

typedef class_exporter<Fcolor> CScriptFcolor;
add_to_type_list(CScriptFcolor)
#undef script_type_list
#define script_type_list save_type_list(CScriptFcolor)
