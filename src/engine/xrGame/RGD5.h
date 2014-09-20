#pragma once

#include "grenade.h"
#include "script_export_space.h"

class CRGD5 :
	public CGrenade
{
	typedef CGrenade inherited;
public:
	CRGD5(void);
	virtual ~CRGD5(void);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CRGD5)
#undef script_type_list
#define script_type_list save_type_list(CRGD5)
