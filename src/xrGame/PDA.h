#pragma once

#include "xrEngine/Feel_Touch.h"
#include "inventory_item_object.h"

#include "InfoPortionDefs.h"
#include "character_info_defs.h"

#include "PdaMsg.h"

class CInventoryOwner;
class CPda;

using PDA_LIST = xr_vector<CPda*>;

class CPda : public CInventoryItemObject, public Feel::Touch
{
    typedef CInventoryItemObject inherited;

public:
    CPda();
    virtual ~CPda();

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void Load(LPCSTR section);
    virtual void net_Destroy();

    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);

    virtual void shedule_Update(u32 dt);

    virtual void feel_touch_new(IGameObject* O);
    virtual void feel_touch_delete(IGameObject* O);
    virtual bool feel_touch_contact(IGameObject* O);

    virtual u16 GetOriginalOwnerID() { return m_idOriginalOwner; }
    virtual CInventoryOwner* GetOriginalOwner();
    virtual IGameObject* GetOwnerObject();

    void TurnOn() { m_bTurnedOff = false; }
    void TurnOff() { m_bTurnedOff = true; }
    bool IsActive() { return IsOn(); }
    bool IsOn() { return !m_bTurnedOff; }
    bool IsOff() { return m_bTurnedOff; }
    void ActivePDAContacts(xr_vector<CPda*>& res);
    CPda* GetPdaFromOwner(IGameObject* owner);
    u32 ActiveContactsNum() { return m_active_contacts.size(); }
    void PlayScriptFunction();
    bool CanPlayScriptFunction()
    {
        if (!xr_strcmp(m_functor_str, ""))
            return false;
        return true;
    };

    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);

    //*	virtual LPCSTR							Name					();

protected:
    void UpdateActiveContacts();

    xr_vector<IGameObject*> m_active_contacts;
    float m_fRadius;

    u16 m_idOriginalOwner;
    shared_str m_SpecificChracterOwner;
    xr_string m_sFullName;

    bool m_bTurnedOff;
    shared_str m_functor_str;
};
