////////////////////////////////////////////////////////////////////////////
//	Module 		: level_changer.cpp
//	Created 	: 10.07.2003
//  Modified 	: 10.07.2003
//	Author		: Dmitriy Iassenev
//	Description : Level change object
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "level_changer.h"
#include "Hit.h"
#include "Actor.h"
#include "xrServer_Objects_ALife.h"
#include "Level.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrNetServer/NET_Messages.h"

#include "UIGameSP.h"
#include "xrEngine/xr_collide_form.h"

xr_vector<CLevelChanger*> g_lchangers;

CLevelChanger::~CLevelChanger() {}
void CLevelChanger::Center(Fvector& C) const { XFORM().transform_tiny(C, GetCForm()->getSphere().P); }
float CLevelChanger::Radius() const { return GetCForm()->getRadius(); }
void CLevelChanger::net_Destroy()
{
    inherited::net_Destroy();
    xr_vector<CLevelChanger*>::iterator it = std::find(g_lchangers.begin(), g_lchangers.end(), this);
    if (it != g_lchangers.end())
        g_lchangers.erase(it);
}
#define DEF_INVITATION "level_changer_invitation"

BOOL CLevelChanger::net_Spawn(CSE_Abstract* DC)
{
    m_entrance_time = 0;
    m_b_enabled = true;
    m_invite_str = DEF_INVITATION;
    CCF_Shape* l_pShape = new CCF_Shape(this);
    SetCForm(l_pShape);

    CSE_Abstract* l_tpAbstract = (CSE_Abstract*)(DC);
    CSE_ALifeLevelChanger* l_tpALifeLevelChanger = smart_cast<CSE_ALifeLevelChanger*>(l_tpAbstract);
    R_ASSERT(l_tpALifeLevelChanger);

    m_game_vertex_id = l_tpALifeLevelChanger->m_tNextGraphID;
    m_level_vertex_id = l_tpALifeLevelChanger->m_dwNextNodeID;
    m_position = l_tpALifeLevelChanger->m_tNextPosition;
    m_angles = l_tpALifeLevelChanger->m_tAngles;

    m_bSilentMode = !!l_tpALifeLevelChanger->m_bSilentMode;
    if (ai().get_level_graph())
    {
        //. this information should be computed in xrAI
        ai_location().level_vertex(ai().level_graph().vertex(u32(-1), Position()));
        ai_location().game_vertex(ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id());
    }

    feel_touch.clear();

    for (u32 i = 0; i < l_tpALifeLevelChanger->shapes.size(); ++i)
    {
        CSE_Shape::shape_def& S = l_tpALifeLevelChanger->shapes[i];
        switch (S.type)
        {
        case 0:
        {
            l_pShape->add_sphere(S.data.sphere);
            break;
        }
        case 1:
        {
            l_pShape->add_box(S.data.box);
            break;
        }
        }
    }

    BOOL bOk = inherited::net_Spawn(DC);
    if (bOk)
    {
        l_pShape->ComputeBounds();
        Fvector P;
        XFORM().transform_tiny(P, GetCForm()->getSphere().P);
        setEnabled(TRUE);
    }
    g_lchangers.push_back(this);
    return (bOk);
}

void CLevelChanger::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    const Fsphere& s = GetCForm()->getSphere();
    Fvector P;
    XFORM().transform_tiny(P, s.P);
    feel_touch_update(P, s.R);

    update_actor_invitation();
}
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
void CLevelChanger::feel_touch_new(IGameObject* tpObject)
{
    CActor* l_tpActor = smart_cast<CActor*>(tpObject);
    VERIFY(l_tpActor);
    if (!l_tpActor->g_Alive())
        return;

    if (m_bSilentMode)
    {
        NET_Packet p;
        p.w_begin(M_CHANGE_LEVEL);
        p.w(&m_game_vertex_id, sizeof(m_game_vertex_id));
        p.w(&m_level_vertex_id, sizeof(m_level_vertex_id));
        p.w_vec3(m_position);
        p.w_vec3(m_angles);
        Level().Send(p, net_flags(TRUE));
        return;
    }
    Fvector p, r;
    bool b = get_reject_pos(p, r);
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (pGameSP)
        pGameSP->ChangeLevel(
            m_game_vertex_id, m_level_vertex_id, m_position, m_angles, p, r, b, m_invite_str, m_b_enabled);

    m_entrance_time = Device.fTimeGlobal;
}

bool CLevelChanger::get_reject_pos(Fvector& p, Fvector& r)
{
    p.set(0, 0, 0);
    r.set(0, 0, 0);
    //--		db.actor:set_actor_position(patrol("t_way"):point(0))
    //--		local dir = patrol("t_look"):point(0):sub(patrol("t_way"):point(0))
    //--		db.actor:set_actor_direction(-dir:getH())

    if (m_ini_file && m_ini_file->section_exist("pt_move_if_reject"))
    {
        LPCSTR p_name = m_ini_file->r_string("pt_move_if_reject", "path");
        const CPatrolPath* patrol_path = ai().patrol_paths().path(p_name);
        VERIFY(patrol_path);

        const CPatrolPoint* pt;
        pt = &patrol_path->vertex(0)->data();
        p = pt->position();

        Fvector tmp;
        pt = &patrol_path->vertex(1)->data();
        tmp.sub(pt->position(), p);
        tmp.getHP(r.y, r.x);
        return true;
    }
    return false;
}

bool CLevelChanger::feel_touch_contact(IGameObject* object)
{
    bool bRes = (((CCF_Shape*)GetCForm())->Contact(object));
    bRes = bRes && smart_cast<CActor*>(object) && smart_cast<CActor*>(object)->g_Alive();
    return bRes;
}

void CLevelChanger::update_actor_invitation()
{
    if (m_bSilentMode)
        return;
    xr_vector<IGameObject*>::iterator it = feel_touch.begin();
    xr_vector<IGameObject*>::iterator it_e = feel_touch.end();

    for (; it != it_e; ++it)
    {
        CActor* l_tpActor = smart_cast<CActor*>(*it);
        VERIFY(l_tpActor);

        if (!l_tpActor->g_Alive())
            continue;

        if (m_entrance_time + 5.0f < Device.fTimeGlobal)
        {
            CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
            Fvector p, r;
            bool b = get_reject_pos(p, r);

            if (pGameSP)
                pGameSP->ChangeLevel(
                    m_game_vertex_id, m_level_vertex_id, m_position, m_angles, p, r, b, m_invite_str, m_b_enabled);

            m_entrance_time = Device.fTimeGlobal;
        }
    }
}

void CLevelChanger::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    output_packet.w_stringZ(m_invite_str);
    output_packet.w_u8(m_b_enabled ? 1 : 0);
}

void CLevelChanger::load(IReader& input_packet)
{
    inherited::load(input_packet);
    input_packet.r_stringZ(m_invite_str);
    m_b_enabled = !!input_packet.r_u8();
}

BOOL CLevelChanger::net_SaveRelevant()
{
    if (!m_b_enabled || m_invite_str != DEF_INVITATION)
        return TRUE;
    else
        return inherited::net_SaveRelevant();
}
