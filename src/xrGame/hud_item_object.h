#pragma once

#include "inventory_item_object.h"
#include "HudItem.h"

class CHudItemObject : public CInventoryItemObject, public CHudItem
{
protected: //чтоб нельзя было вызвать на прямую
    CHudItemObject();
    virtual ~CHudItemObject();

public:
    virtual IFactoryObject* _construct();

public:
    virtual CHudItem* cast_hud_item() { return this; }
public:
    virtual void Load(LPCSTR section);
    virtual bool Action(u16 cmd, u32 flags);
    virtual void SwitchState(u32 S);
    virtual void OnStateSwitch(u32 S, u32 oldState);
    virtual void OnEvent(NET_Packet& P, u16 type);
    virtual void OnH_A_Chield();
    virtual void OnH_B_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void OnH_A_Independent();
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual bool ActivateItem();
    virtual void DeactivateItem();
    virtual void UpdateCL();
    virtual void renderable_Render();
    virtual void on_renderable_Render();
    virtual void OnMoveToRuck(const SInvItemPlace& prev);

    virtual bool use_parent_ai_locations() const
    {
        return CInventoryItemObject::use_parent_ai_locations() && (Device.dwFrame != dwXF_Frame);
    }
};
