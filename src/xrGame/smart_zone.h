////////////////////////////////////////////////////////////////////////////
//	Module 		: script_zone.h
//	Created 	: 10.10.2003
//  Modified 	: 10.10.2003
//	Author		: Dmitriy Iassenev
//	Description : Script zone object
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "space_restrictor.h"

class CSmartZone : public CSpaceRestrictor {
public:
	virtual	bool	register_schedule	() const {return true;}
};
