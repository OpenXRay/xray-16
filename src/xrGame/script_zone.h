////////////////////////////////////////////////////////////////////////////
//	Module 		: script_zone.h
//	Created 	: 10.10.2003
//  Modified 	: 10.10.2003
//	Author		: Dmitriy Iassenev
//	Description : Script zone object
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "space_restrictor.h"
#include "../xrEngine/feel_touch.h"
#include "script_space_forward.h"
#include "script_export_space.h"

class CScriptGameObject;

class CScriptZone : public CSpaceRestrictor, public Feel::Touch {
public:
	typedef	CSpaceRestrictor inherited;

					CScriptZone			();
	virtual			~CScriptZone		();
	virtual void	reinit				();
	virtual BOOL	net_Spawn			(CSE_Abstract* DC);
	virtual void	net_Destroy			();
	virtual void	net_Relcase			(CObject* O);
	virtual void	shedule_Update		(u32 dt);
	virtual void	feel_touch_new		(CObject* O);
	virtual void	feel_touch_delete	(CObject* O);
	virtual BOOL	feel_touch_contact	(CObject* O);
			bool	active_contact		(u16 id) const;
	virtual bool	IsVisibleForZones	()				{return false;}
	virtual	bool	register_schedule	() const		{return true;}
#ifdef DEBUG
	virtual	void	OnRender			();
#endif
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CScriptZone)
#undef script_type_list
#define script_type_list save_type_list(CScriptZone)
