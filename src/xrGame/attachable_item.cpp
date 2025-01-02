////////////////////////////////////////////////////////////////////////////
//	Module 		: attachable_item.cpp
//	Created 	: 11.02.2004
//  Modified 	: 11.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Attachable item
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PhysicsShellHolder.h"
#include "attachable_item.h"
#include "InventoryOwner.h"
#include "Inventory.h"
#include "xrEngine/xr_input.h"
#include "xrUICore/ui_base.h"
#include "xrEngine/GameFont.h"

#ifdef DEBUG
CAttachableItem* CAttachableItem::m_dbgItem = NULL;
#endif

CPhysicsShellHolder& CAttachableItem::object() const { return (item().object()); }
IFactoryObject* CAttachableItem::_construct()
{
    VERIFY(!m_item);
    m_item = smart_cast<CInventoryItem*>(this);
    VERIFY(m_item);
    return (&item().object());
}

CAttachableItem::~CAttachableItem() {}
void CAttachableItem::reload(LPCSTR section)
{
#ifdef DEBUG
    m_valid = true;
#endif

    if (load_attach_position(section))
        enable(false);
}

bool CAttachableItem::load_attach_position(LPCSTR section)
{
    if (!pSettings->line_exist(section, "attach_angle_offset"))
        return false;

    Fvector angle_offset = pSettings->r_fvector3(section, "attach_angle_offset");
    Fvector position_offset = pSettings->r_fvector3(section, "attach_position_offset");
    m_offset.setHPB(VPUSH(angle_offset));
    m_offset.c = position_offset;
    m_bone_name = pSettings->r_string(section, "attach_bone_name");
    return true;
}

void CAttachableItem::OnH_A_Chield()
{
    const CInventoryOwner* inventory_owner = smart_cast<const CInventoryOwner*>(object().H_Parent());
    if (inventory_owner && inventory_owner->attached(&item()))
        object().setVisible(true);
}

void CAttachableItem::renderable_Render(u32 context_id, IRenderable* root)
{
    GEnv.Render->add_Visual(context_id, root, object().Visual(), object().XFORM());
}

void CAttachableItem::OnH_A_Independent() { enable(false); }
void CAttachableItem::enable(bool value)
{
    if (!object().H_Parent())
    {
        m_enabled = value;
        return;
    }

    if (value && !enabled() && object().H_Parent())
    {
        CGameObject* game_object = smart_cast<CGameObject*>(object().H_Parent());
        CAttachmentOwner* owner = smart_cast<CAttachmentOwner*>(game_object);
        if (owner)
        {
            m_enabled = value;
            owner->attach(&item());
            object().setVisible(true);
        }
    }

    if (!value && enabled() && object().H_Parent())
    {
        CGameObject* game_object = smart_cast<CGameObject*>(object().H_Parent());
        CAttachmentOwner* owner = smart_cast<CAttachmentOwner*>(game_object);
        if (owner)
        {
            m_enabled = value;
            owner->detach(&item());
            object().setVisible(false);
        }
    }
}

bool CAttachableItem::can_be_attached() const
{
    if (!item().m_pInventory)
        return (false);

    if (!item().m_pInventory->IsBeltUseful())
        return (true);

    if (item().m_ItemCurrPlace.type != eItemPlaceBelt)
        return (false);

    return (true);
}

void CAttachableItem::afterAttach()
{
    VERIFY(m_valid);
    object().processing_activate();
}

void CAttachableItem::afterDetach()
{
    VERIFY(m_valid);
    object().processing_deactivate();
}

bool CAttachableItem::use_parent_ai_locations() const
{
    return !enabled();
}
