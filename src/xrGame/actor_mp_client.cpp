#include "StdAfx.h"
#include "actor_mp_client.h"
#include "ActorCondition.h"
#include "xrEngine/CameraBase.h"
#include "xrEngine/CameraManager.h"

#include "game_cl_base.h"
#include "ui/UIActorMenu.h"
#include "ui/UIDragDropReferenceList.h"
#include "UIGameCustom.h"
#include "eatable_item.h"

// if we are not current control entity we use this value
const float CActorMP::cam_inert_value = 0.7f;

CActorMP::CActorMP()
{
    // m_i_am_dead				= false;
}

void CActorMP::OnEvent(NET_Packet& P, u16 type)
{
    if (type == GEG_PLAYER_USE_BOOSTER)
    {
        use_booster(P);
        return;
    }
    inherited::OnEvent(P, type);
}

void CActorMP::Die(IGameObject* killer)
{
    // m_i_am_dead				= true;
    // conditions().health()	= 0.f;
    conditions().SetHealth(0.f);
    inherited::Die(killer);
}

void CActorMP::cam_Set(EActorCameras style)
{
#ifndef DEBUG
    if (style != eacFirstEye)
        return;
#endif
    CCameraBase* old_cam = cam_Active();
    cam_active = style;
    old_cam->OnDeactivate();
    cam_Active()->OnActivate(old_cam);
}

void CActorMP::use_booster(NET_Packet& packet)
{
    if (OnServer())
        return;

    u16 tmp_booster_id;
    packet.r_u16(tmp_booster_id);
    IGameObject* tmp_booster = Level().Objects.net_Find(tmp_booster_id);
    VERIFY2(tmp_booster, "using unknown or deleted booster");
    if (!tmp_booster)
    {
        Msg("! ERROR: trying to use unkown booster object, ID = %d", tmp_booster_id);
        return;
    }

    CEatableItem* tmp_eatable = smart_cast<CEatableItem*>(tmp_booster);
    VERIFY2(tmp_eatable, "using not eatable object");
    if (!tmp_eatable)
    {
        Msg("! ERROR: trying to use not eatable object, ID = %d", tmp_booster_id);
        return;
    }
    tmp_eatable->UseBy(this);
}

void CActorMP::On_SetEntity()
{
    prev_cam_inert_value = psCamInert;
    if (this != Level().CurrentControlEntity())
    {
        psCamInert = cam_inert_value;
    }
    inherited::On_SetEntity();
}

void CActorMP::On_LostEntity() { psCamInert = prev_cam_inert_value; }
