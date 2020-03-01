////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "eatable_item.h"
#include "xrMessages.h"
#include "physic_item.h"
#include "Level.h"
#include "entity_alive.h"
#include "EntityCondition.h"
#include "InventoryOwner.h"
#include "UIGameCustom.h"
#include "ui/UIActorMenu.h"

CEatableItem::CEatableItem() : m_fWeightFull(0), m_fWeightEmpty(0)
{
    m_physic_item = 0;
    m_iMaxUses = 1;
    m_iRemainingUses = 1;
    m_bRemoveAfterUse = true;
}

CEatableItem::~CEatableItem() {}
IFactoryObject* CEatableItem::_construct()
{
    m_physic_item = smart_cast<CPhysicItem*>(this);
    return (inherited::_construct());
}

void CEatableItem::Load(LPCSTR section)
{
    inherited::Load(section);

    m_iRemainingUses = m_iMaxUses = READ_IF_EXISTS(pSettings, r_u8, section, "max_uses", 1);
    m_bRemoveAfterUse = READ_IF_EXISTS(pSettings, r_bool, section, "remove_after_use", true);
    m_fWeightFull = m_weight;
    m_fWeightEmpty = READ_IF_EXISTS(pSettings, r_float, section, "empty_weight", 0.0f);

    if (IsUsingCondition())
    {
        if (m_iMaxUses > 0)
            SetCondition(static_cast<float>(m_iRemainingUses / m_iMaxUses));
        else
            SetCondition(0.f);
    }
}

void CEatableItem::load(IReader& packet)
{
    inherited::load(packet);

    m_iRemainingUses = packet.r_u8();
}

void CEatableItem::save(NET_Packet& packet)
{
    inherited::save(packet);

    packet.w_u8(m_iRemainingUses);
}

BOOL CEatableItem::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return FALSE;

    if (IsUsingCondition())
    {
        if (m_iMaxUses > 0)
            SetCondition(static_cast<float>(m_iRemainingUses / m_iMaxUses));
        else
            SetCondition(0);
    }

    return TRUE;
};

bool CEatableItem::Useful() const
{
    if (!inherited::Useful())
        return false;

    //проверить не все ли еще съедено
    if (m_iRemainingUses == 0 && CanDelete())
        return false;

    return true;
}

void CEatableItem::OnH_A_Independent()
{
    inherited::OnH_A_Independent();
    if (!Useful())
    {
        if (object().Local() && OnServer())
            object().DestroyObject();
    }
}

void CEatableItem::OnH_B_Independent(bool just_before_destroy)
{
    if (!Useful())
    {
        object().setVisible(FALSE);
        object().setEnabled(FALSE);
        if (m_physic_item)
            m_physic_item->m_ready_to_destroy = true;
    }
    inherited::OnH_B_Independent(just_before_destroy);
}

bool CEatableItem::UseBy(CEntityAlive* entity_alive)
{
    SMedicineInfluenceValues V;
    V.Load(m_physic_item->cNameSect());

    CInventoryOwner* IO = smart_cast<CInventoryOwner*>(entity_alive);
    R_ASSERT(IO);
    R_ASSERT(m_pInventory == IO->m_inventory);
    R_ASSERT(object().H_Parent()->ID() == entity_alive->ID());

    entity_alive->conditions().ApplyInfluence(V, m_physic_item->cNameSect());

    for (u8 i = 0; i < (u8)eBoostMaxCount; i++)
    {
        if (pSettings->line_exist(m_physic_item->cNameSect().c_str(), ef_boosters_section_names[i]))
        {
            SBooster B;
            B.Load(m_physic_item->cNameSect(), (EBoostParams)i);
            entity_alive->conditions().ApplyBooster(B, m_physic_item->cNameSect());
        }
    }

    if (!IsGameTypeSingle() && OnServer())
    {
        NET_Packet tmp_packet;
        CGameObject::u_EventGen(tmp_packet, GEG_PLAYER_USE_BOOSTER, entity_alive->ID());
        tmp_packet.w_u16(object_id());
        Level().Send(tmp_packet);
    }

    // If uses 255, then skip the decrement for infinite usages
    if (m_iRemainingUses != -1)
    {
        if (m_iRemainingUses > 0)
            --m_iRemainingUses;
        else
            m_iRemainingUses = 0;
    }

    if (IsUsingCondition())
    {
        if (m_iMaxUses > 0)
            SetCondition(static_cast<float>(m_iRemainingUses / m_iMaxUses));
        else
            SetCondition(0.f);
    }

    return true;
}

float CEatableItem::Weight() const
{
    float res = inherited::Weight();

    if (IsUsingCondition())
    {
        const float net_weight = m_fWeightFull - m_fWeightEmpty;
        const float use_weight = m_iMaxUses > 0 ? net_weight / m_iMaxUses : 0.f;

        res = m_fWeightEmpty + m_iRemainingUses * use_weight;
    }

    return res;
}
