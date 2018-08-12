////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_feel.cpp
//	Created 	: 25.02.2003
//  Modified 	: 25.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Feelings for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ai_stalker.h"
#include "inventory_item.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sight_manager.h"
#include "stalker_movement_manager_smart_cover.h"
#include "stalker_animation_manager.h"

#ifdef DEBUG
#include "ai_debug.h"
extern Flags32 psAI_Flags;
#endif // DEBUG

bool CAI_Stalker::feel_vision_isRelevant(IGameObject* O)
{
    if (!g_Alive())
        return false;
    CEntityAlive* E = smart_cast<CEntityAlive*>(O);
    CInventoryItem* I = smart_cast<CInventoryItem*>(O);
    if (!E && !I)
        return (false);
    //	if (E && (E->g_Team() == g_Team()))			return false;
    return (true);
}

void CAI_Stalker::renderable_Render()
{
    inherited::renderable_Render();

    if (!already_dead())
        CInventoryOwner::renderable_Render();

#ifdef DEBUG
    if (g_Alive())
    {
        if (psAI_Flags.test(aiAnimationStats))
            animation().add_animation_stats();
    }
#endif // DEBUG
}

void CAI_Stalker::Exec_Look(float dt) { sight().Exec_Look(dt); }
bool CAI_Stalker::bfCheckForNodeVisibility(u32 dwNodeID, bool bIfRayPick)
{
    return (memory().visual().visible(dwNodeID, movement().m_head.current.yaw, ffGetFov()));
}

bool CAI_Stalker::feel_touch_contact(IGameObject* O)
{
    if (!m_take_items_enabled && smart_cast<CInventoryItem*>(O))
        return (false);

    if (O == this)
        return (false);

    if (!inherited::feel_touch_contact(O))
        return (false);

    CGameObject* game_object = smart_cast<CGameObject*>(O);
    if (!game_object)
        return (false);

    return (game_object->feel_touch_on_contact(this));
}

bool CAI_Stalker::feel_touch_on_contact(IGameObject* O)
{
    VERIFY(O != this);

    if ((O->GetSpatialData().type | STYPE_VISIBLEFORAI) != O->GetSpatialData().type)
        return (false);

    return (inherited::feel_touch_on_contact(O));
}

void CAI_Stalker::feel_touch_delete(IGameObject* O)
{
    ignored_touched_objects_type::iterator i =
        std::find(m_ignored_touched_objects.begin(), m_ignored_touched_objects.end(), O);
    if (i == m_ignored_touched_objects.end())
        return;

    m_ignored_touched_objects.erase(i);
}
