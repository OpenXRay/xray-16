#pragma once

#include "script_export_space.h"

struct console_registrator{
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(console_registrator)
#undef script_type_list
#define script_type_list save_type_list(console_registrator)
