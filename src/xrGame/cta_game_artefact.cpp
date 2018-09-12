////////////////////////////////////////////////////////////////////////////////
//	Module		:	cta_game_artefact.cpp
//	Created		:	19.12.2007
//	Modified	:	19.12.2007
//	Autor		:	Alexander Maniluk
//	Description	:	Artefact object for Capture The Artefact game mode
////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "cta_game_artefact.h"
#include "cta_game_artefact_activation.h"
#include "game_cl_capture_the_artefact.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xr_level_controller.h"

CtaGameArtefact::CtaGameArtefact()
{
    // game object must present...
    m_game = smart_cast<game_cl_CaptureTheArtefact*>(&Game());
    m_artefact_rpoint = NULL;
    m_my_team = etSpectatorsTeam;
}

CtaGameArtefact::~CtaGameArtefact() {}
bool CtaGameArtefact::IsMyTeamArtefact()
{
    if (!m_game)
        return true;

    R_ASSERT(H_Parent());
    game_PlayerState* ps = m_game->GetPlayerByGameID(H_Parent()->ID());
    R_ASSERT(ps != NULL);
    if (ps->team == etGreenTeam)
    {
        if (m_game->GetGreenArtefactID() == this->ID())
        {
            return true;
        }
    }
    else if (ps->team == etBlueTeam)
    {
        if (m_game->GetBlueArtefactID() == this->ID())
        {
            return true;
        }
    }
    return false;
}
bool CtaGameArtefact::Action(s32 cmd, u32 flags)
{
    if (m_game && (cmd == kWPN_FIRE) && (flags & CMD_START))
    {
        if (!m_game->CanActivateArtefact() || !IsMyTeamArtefact())
            return true;
    }

    return inherited::Action((u16)cmd, flags);
}

void CtaGameArtefact::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    /*// just temporary (before we get huds for artefact activation)
    if (S == eActivating)
    {
        // here will be animation
        OnAnimationEnd(eActivating);
        return;
    } else
    {
        //inherited::OnStateSwitch(S, oldState);
    }*/
}

void CtaGameArtefact::OnAnimationEnd(u32 state)
{
    if (!H_Parent())
    {
#ifndef MASTER_GOLD
        Msg("! ERROR: enemy artefact activation, H_Parent is NULL.");
#endif // #ifndef MASTER_GOLD
        return;
    }
    inherited::OnAnimationEnd(state);
}

void CtaGameArtefact::UpdateCLChild()
{
    inherited::UpdateCLChild();
    if (H_Parent())
        XFORM().set(H_Parent()->XFORM());

    if (!m_artefact_rpoint)
        InitializeArtefactRPoint();

    if (!m_artefact_rpoint)
    {
#ifdef DEBUG
        Msg("--- Waiting for sync packet, for artefact rpoint.");
#endif // #ifdef DEBUG
        return;
    }

    VERIFY(m_artefact_rpoint);

    if (m_game && m_artefact_rpoint->similar(XFORM().c, m_game->GetBaseRadius()) && PPhysicsShell())
    {
        MoveTo(*m_artefact_rpoint);
        deactivate_physics_shell();
    }
}

void CtaGameArtefact::InitializeArtefactRPoint()
{
    if (!m_game)
        return;

    if (ID() == m_game->GetGreenArtefactID())
    {
        m_artefact_rpoint = &m_game->GetGreenArtefactRPoint();
        m_my_team = etGreenTeam;
    }
    else if (ID() == m_game->GetBlueArtefactID())
    {
        m_artefact_rpoint = &m_game->GetBlueArtefactRPoint();
        m_my_team = etBlueTeam;
    }
}

void CtaGameArtefact::CreateArtefactActivation()
{
    if (OnServer())
    {
        NET_Packet P;
        CGameObject::u_EventGen(P, GE_OWNERSHIP_REJECT, H_Parent()->ID());
        P.w_u16(ID());
        P.w_u8(0);
        P.w_vec3(*m_artefact_rpoint);
        CGameObject::u_EventSend(P);
        MoveTo(*m_artefact_rpoint); // for server net_Import
    }
    // deactivate_physics_shell();
}

/*BOOL CtaGameArtefact::net_Relevant()
{
    return inherited::net_Relevant();
}*/

bool CtaGameArtefact::CanTake() const
{
    if (!inherited::CanTake())
        return false;

    return true;
};

/*void CtaGameArtefact::Interpolate()
{
    inherited::Interpolate();
}*/

void CtaGameArtefact::PH_A_CrPr()
{
    /*if (m_just_after_spawn)
    {
        VERIFY(object().Visual());
        IKinematics *K = object().Visual()->dcast_PKinematics();
        VERIFY( K );
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);

        object().PPhysicsShell()->GetGlobalTransformDynamic(&object().XFORM());
        object().spatial_move();
        m_just_after_spawn = false;

        VERIFY(!OnServer());
        if (object().PPhysicsShell())
        {
            object().PPhysicsShell()->get_ElementByStoreOrder(0)->Fix();
            object().PPhysicsShell()->SetIgnoreStatic	();
        }
        //object().PPhysicsShell()->SetIgnoreDynamic	();
        //PPhysicsShell()->DisableCollision();
    }*/
}

/*
void CtaGameArtefact::net_Export(NET_Packet& P)
{
    if (H_Parent() || IsGameTypeSingle())
    {
        P.w_u8				(0);
        return;
    }
    CPHSynchronize* pSyncObj		= NULL;
    SPHNetState						State;
    pSyncObj = PHGetSyncItem		(0);

    if (pSyncObj)
        pSyncObj->get_State					(State);
    else
        State.position.set					(Position());


    CSE_ALifeInventoryItem::mask_num_items		num_items;
    num_items.mask			= 0;
    num_items.num_items		= 1;// always to synchronize . (object().PHGetSyncItemsNumber() == 1)

    if (State.enabled)									num_items.mask |=
CSE_ALifeInventoryItem::inventory_item_state_enabled;
    if (fis_zero(State.angular_vel.square_magnitude()))	num_items.mask |=
CSE_ALifeInventoryItem::inventory_item_angular_null;
    if (fis_zero(State.linear_vel.square_magnitude()))	num_items.mask |=
CSE_ALifeInventoryItem::inventory_item_linear_null;

    P.w_u8					(num_items.common);

    P.w_vec3				(State.position);

    float					magnitude = _sqrt(State.quaternion.magnitude());
    if (fis_zero(magnitude)) {
        magnitude			= 1;
        State.quaternion.x	= 0.f;
        State.quaternion.y	= 0.f;
        State.quaternion.z	= 1.f;
        State.quaternion.w	= 0.f;
    }
    else {
        float				invert_magnitude = 1.f/magnitude;

        State.quaternion.x	*= invert_magnitude;
        State.quaternion.y	*= invert_magnitude;
        State.quaternion.z	*= invert_magnitude;
        State.quaternion.w	*= invert_magnitude;

        clamp				(State.quaternion.x,0.f,1.f);
        clamp				(State.quaternion.y,0.f,1.f);
        clamp				(State.quaternion.z,0.f,1.f);
        clamp				(State.quaternion.w,0.f,1.f);
    }

    P.w_float_q8			(State.quaternion.x,0.f,1.f);
    P.w_float_q8			(State.quaternion.y,0.f,1.f);
    P.w_float_q8			(State.quaternion.z,0.f,1.f);
    P.w_float_q8			(State.quaternion.w,0.f,1.f);

    if (!(num_items.mask & CSE_ALifeInventoryItem::inventory_item_angular_null)) {
        clamp				(State.angular_vel.x,0.f,10.f*PI_MUL_2);
        clamp				(State.angular_vel.y,0.f,10.f*PI_MUL_2);
        clamp				(State.angular_vel.z,0.f,10.f*PI_MUL_2);

        P.w_float_q8		(State.angular_vel.x,0.f,10.f*PI_MUL_2);
        P.w_float_q8		(State.angular_vel.y,0.f,10.f*PI_MUL_2);
        P.w_float_q8		(State.angular_vel.z,0.f,10.f*PI_MUL_2);
    }

    if (!(num_items.mask & CSE_ALifeInventoryItem::inventory_item_linear_null)) {
        clamp				(State.linear_vel.x,-32.f,32.f);
        clamp				(State.linear_vel.y,-32.f,32.f);
        clamp				(State.linear_vel.z,-32.f,32.f);

        P.w_float_q8		(State.linear_vel.x,-32.f,32.f);
        P.w_float_q8		(State.linear_vel.y,-32.f,32.f);
        P.w_float_q8		(State.linear_vel.z,-32.f,32.f);
    }
    P.w_u8(1);		//always enabled...
};*/
