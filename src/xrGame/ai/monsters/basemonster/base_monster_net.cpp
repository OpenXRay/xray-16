#include "StdAfx.h"
#include "base_monster.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrAICore/Navigation/game_graph.h"
#include "ai_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "CharacterPhysicsSupport.h"

void CBaseMonster::net_Save(NET_Packet& P)
{
    inherited::net_Save(P);
    m_pPhysics_support->in_NetSave(P);
}

BOOL CBaseMonster::net_SaveRelevant() { return (inherited::net_SaveRelevant() || BOOL(PPhysicsShell() != NULL)); }
void CBaseMonster::net_Export(NET_Packet& P)
{
    R_ASSERT(Local());

    // export last known packet
    R_ASSERT(!NET.empty());
    net_update& N = NET.back();
    P.w_float(GetfHealth());
    P.w_u32(N.dwTimeStamp);
    P.w_u8(0);
    P.w_vec3(N.p_pos);
    P.w_float /*w_angle8*/ (N.o_model);
    P.w_float /*w_angle8*/ (N.o_torso.yaw);
    P.w_float /*w_angle8*/ (N.o_torso.pitch);
    P.w_float /*w_angle8*/ (N.o_torso.roll);
    P.w_u8(u8(g_Team()));
    P.w_u8(u8(g_Squad()));
    P.w_u8(u8(g_Group()));

    GameGraph::_GRAPH_ID l_game_vertex_id = ai_location().game_vertex_id();
    P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
    P.w(&l_game_vertex_id, sizeof(l_game_vertex_id));
    //	P.w						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
    //	P.w						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
    float f1 = 0;
    if (ai().game_graph().valid_vertex_id(l_game_vertex_id))
    {
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.w(&f1, sizeof(f1));
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.w(&f1, sizeof(f1));
    }
    else
    {
        P.w(&f1, sizeof(f1));
        P.w(&f1, sizeof(f1));
    }
}

void CBaseMonster::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote());
    net_update N;

    u8 flags;

    float health;
    P.r_float(health);
    SetfHealth(health);

    P.r_u32(N.dwTimeStamp);
    P.r_u8(flags);
    P.r_vec3(N.p_pos);
    P.r_float /*r_angle8*/ (N.o_model);
    P.r_float /*r_angle8*/ (N.o_torso.yaw);
    P.r_float /*r_angle8*/ (N.o_torso.pitch);
    P.r_float /*r_angle8*/ (N.o_torso.roll);
    id_Team = P.r_u8();
    id_Squad = P.r_u8();
    id_Group = P.r_u8();

    GameGraph::_GRAPH_ID l_game_vertex_id = ai_location().game_vertex_id();
    P.r(&l_game_vertex_id, sizeof(l_game_vertex_id));
    P.r(&l_game_vertex_id, sizeof(l_game_vertex_id));

    if (NET.empty() || (NET.back().dwTimeStamp < N.dwTimeStamp))
    {
        NET.push_back(N);
        NET_WasInterpolating = TRUE;
    }

    //	P.r						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
    //	P.r						(&m_fGoingSpeed,			sizeof(m_fGoingSpeed));
    float f1 = 0;
    if (ai().game_graph().valid_vertex_id(l_game_vertex_id))
    {
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.r(&f1, sizeof(f1));
        f1 = Position().distance_to(ai().game_graph().vertex(l_game_vertex_id)->level_point());
        P.r(&f1, sizeof(f1));
    }
    else
    {
        P.r(&f1, sizeof(f1));
        P.r(&f1, sizeof(f1));
    }

    setVisible(TRUE);
    setEnabled(TRUE);
}
