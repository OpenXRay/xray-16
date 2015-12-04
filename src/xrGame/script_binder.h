////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder.h
//	Created 	: 26.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script objects binder
////////////////////////////////////////////////////////////////////////////

#pragma once

class CSE_Abstract;
class CScriptBinderObject;
class NET_Packet;
class CGameObject;

class CScriptBinder {
protected:
	CScriptBinderObject			*m_object;
    CGameObject *owner;
public:
								CScriptBinder	(CGameObject *owner);
	virtual						~CScriptBinder	();
			void				init			();
			void				clear			();
	virtual void				reinit			();
	virtual void				reload			(LPCSTR section);
	virtual BOOL				net_Spawn		(CSE_Abstract* DC);
	virtual void				net_Destroy		();
	virtual void				shedule_Update	(u32 time_delta);
	virtual void				save			(NET_Packet &output_packet);
	virtual void				load			(IReader &input_packet);
	virtual BOOL				net_SaveRelevant();
	virtual void				net_Relcase		(IGameObject *object);
			void				set_object		(CScriptBinderObject *object);
	IC		CScriptBinderObject	*object			();
};

#include "script_binder_inline.h"