////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_events.cpp
//	Created 	: 26.02.2003
//  Modified 	: 26.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Events handling for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ai_stalker.h"
#include "PDA.h"
#include "Inventory.h"
#include "xrServerEntities/xrMessages.h"
#include "ShootingObject.h"
#include "Level.h"
#include "ai_monster_space.h"
#include "CharacterPhysicsSupport.h"

using namespace StalkerSpace;
using namespace MonsterSpace;

#define SILENCE

void CAI_Stalker::OnEvent(NET_Packet& P, u16 type)
{
    inherited::OnEvent(P, type);
    CInventoryOwner::OnEvent(P, type);

    switch (type)
    {
    case GE_TRADE_BUY:
    case GE_OWNERSHIP_TAKE:
    {
        u16 id;
        P.r_u16(id);
        IGameObject* O = Level().Objects.net_Find(id);

        R_ASSERT(O);

#ifndef SILENCE
        Msg("Trying to take - %s (%d)", *O->cName(), O->ID());
#endif
        CGameObject* _O = smart_cast<CGameObject*>(O);
        if (inventory().CanTakeItem(smart_cast<CInventoryItem*>(_O)))
        {
            O->H_SetParent(this);
            inventory().Take(_O, true, false);
            if (!inventory().ActiveItem() && GetScriptControl() && smart_cast<CShootingObject*>(O))
                CObjectHandler::set_goal(eObjectActionIdle, _O);

            on_after_take(_O);
#ifndef SILENCE
            Msg("TAKE - %s (%d)", *O->cName(), O->ID());
#endif
        }
        else
        {
            //				DropItemSendMessage(O);
            NET_Packet P;
            u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
            P.w_u16(u16(O->ID()));
            u_EventSend(P);

#ifndef SILENCE
            Msg("TAKE - can't take! - Dropping for valid server information %s (%d)", *O->cName(), O->ID());
#endif
        }
        break;
    }
    case GE_TRADE_SELL:
    case GE_OWNERSHIP_REJECT:
    {
        u16 id;
        P.r_u16(id);

        IGameObject* O = Level().Objects.net_Find(id);

#pragma todo("Dima to Oles : how can this happen?")
        if (!O)
            break;

        bool just_before_destroy = !P.r_eof() && P.r_u8();
        bool dont_create_shell = (type == GE_TRADE_SELL) || just_before_destroy;

        O->SetTmpPreDestroy(just_before_destroy);
        on_ownership_reject(O, dont_create_shell);

        break;
    }
    }
}

void CAI_Stalker::on_ownership_reject(IGameObject* O, bool just_before_destroy)
{
    m_pPhysics_support->in_UpdateCL();
    IKinematics* const kinematics = smart_cast<IKinematics*>(Visual());
    kinematics->CalculateBones_Invalidate();
    kinematics->CalculateBones(true);

    CGameObject* const game_object = smart_cast<CGameObject*>(O);
    VERIFY(game_object);

    if (!inventory().DropItem(game_object, just_before_destroy, just_before_destroy))
        return;

    if (O->getDestroy())
        return;

    feel_touch_deny(O, 2000);
}

void CAI_Stalker::generate_take_event(IGameObject const* const object) const
{
    NET_Packet packet;
    u_EventGen(packet, GE_OWNERSHIP_TAKE, ID());
    packet.w_u16(object->ID());
    u_EventSend(packet);
}

void CAI_Stalker::DropItemSendMessage(IGameObject* O)
{
    if (!O || !O->H_Parent() || (this != O->H_Parent()))
        return;

#ifndef SILENCE
    Msg("Dropping item!");
#endif
    // We doesn't have similar weapon - pick up it
    NET_Packet P;
    u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
    P.w_u16(u16(O->ID()));
    u_EventSend(P);
}

void CAI_Stalker::UpdateAvailableDialogs(CPhraseDialogManager* partner)
{
    CAI_PhraseDialogManager::UpdateAvailableDialogs(partner);
}

void CAI_Stalker::feel_touch_new(IGameObject* O)
{
    //	Msg					("FEEL_TOUCH::NEW : %s",*O->cName());
    if (!g_Alive())
        return;
    if (Remote())
        return;
    if ((O->GetSpatialData().type | STYPE_VISIBLEFORAI) != O->GetSpatialData().type)
        return;

    // Now, test for game specific logical objects to minimize traffic
    CInventoryItem* I = smart_cast<CInventoryItem*>(O);

    if (!wounded() && !critically_wounded() && I && I->useful_for_NPC() && can_take(I))
    {
#ifndef SILENCE
        Msg("Taking item %s (%d)!", I->object().cName().c_str(), I->object().ID());
#endif
        generate_take_event(O);
        return;
    }

    VERIFY2(std::find(m_ignored_touched_objects.begin(), m_ignored_touched_objects.end(), O) ==
            m_ignored_touched_objects.end(),
        make_string("object %s is already in ignroed touched objects list", O->cName().c_str()));
    m_ignored_touched_objects.push_back(O);
}
