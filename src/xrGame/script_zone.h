////////////////////////////////////////////////////////////////////////////
//	Module 		: script_zone.h
//	Created 	: 10.10.2003
//  Modified 	: 10.10.2003
//	Author		: Dmitriy Iassenev
//	Description : Script zone object
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "space_restrictor.h"
#include "xrEngine/Feel_Touch.h"
#include "xrScriptEngine/script_space_forward.hpp"

class CScriptGameObject;

class CScriptZone : public CSpaceRestrictor, public Feel::Touch
{
public:
    typedef CSpaceRestrictor inherited;

    CScriptZone();
    virtual ~CScriptZone();
    virtual void reinit();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Relcase(IGameObject* O);
    virtual void shedule_Update(u32 dt);
    virtual void feel_touch_new(IGameObject* O);
    virtual void feel_touch_delete(IGameObject* O);
    virtual bool feel_touch_contact(IGameObject* O);
    bool active_contact(u16 id) const;
    virtual bool IsVisibleForZones() { return false; }
    virtual bool register_schedule() const { return true; }
#ifdef DEBUG
    virtual void OnRender();
#endif
};
