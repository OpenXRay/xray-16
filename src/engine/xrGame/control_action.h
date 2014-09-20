////////////////////////////////////////////////////////////////////////////
//	Module 		: animation_action.h
//	Created 	: 05.04.2004
//  Modified 	: 05.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Control action
////////////////////////////////////////////////////////////////////////////

#pragma once

class CAI_Stalker;

class CControlAction {
protected:
	CAI_Stalker			*m_object;

public:
	IC					CControlAction	();
	IC		void		set_object		(CAI_Stalker *object);
	IC		bool		applicable		() const;
	IC		bool		completed		() const;
	IC		void		initialize		();
	IC		void		execute			();
	IC		void		finalize		();
	IC		CAI_Stalker &object			() const;
	IC		void		remove_links	(CObject *object);
};

#include "control_action_inline.h"