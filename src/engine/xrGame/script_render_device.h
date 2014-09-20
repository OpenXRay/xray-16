////////////////////////////////////////////////////////////////////////////
//	Module 		: script_render_device.h
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script render device
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

typedef class_exporter<CRenderDevice> CScriptRenderDevice;
add_to_type_list(CScriptRenderDevice)
#undef script_type_list
#define script_type_list save_type_list(CScriptRenderDevice)
