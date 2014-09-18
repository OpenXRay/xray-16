#pragma once

#include "grenade.h"
#include "script_export_space.h"

class CF1 :
	public CGrenade
{
	typedef CGrenade inherited;
public:
	CF1(void);
	virtual ~CF1(void);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CF1)
#undef script_type_list
#define script_type_list save_type_list(CF1)
