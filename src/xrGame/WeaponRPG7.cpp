#include "StdAfx.h"
#include "WeaponRPG7.h"
#include "xrServer_Objects_ALife_Items.h"
#include "ExplosiveRocket.h"
#include "Entity.h"
#include "Level.h"
#include "player_hud.h"
#include "HUDManager.h"

CWeaponRPG7::CWeaponRPG7() {}
CWeaponRPG7::~CWeaponRPG7() {}
void CWeaponRPG7::Load(LPCSTR section)
{
    inherited::Load(section);
    CRocketLauncher::Load(section);

    m_zoom_params.m_fScopeZoomFactor = pSettings->r_float(section, "max_zoom_factor");

    m_sRocketSection = pSettings->r_string(section, "rocket_class");
}

bool CWeaponRPG7::AllowBore() { return inherited::AllowBore() && 0 != iAmmoElapsed; }
void CWeaponRPG7::FireTrace(const Fvector& P, const Fvector& D)
{
    inherited::FireTrace(P, D);
    UpdateMissileVisibility();
}

void CWeaponRPG7::on_a_hud_attach()
{
    inherited::on_a_hud_attach();
    UpdateMissileVisibility();
}

void CWeaponRPG7::UpdateMissileVisibility()
{
    bool vis_hud, vis_weap;
    vis_hud = (!!iAmmoElapsed || GetState() == eReload);
    vis_weap = !!iAmmoElapsed;

    if (GetHUDmode())
    {
        HudItemData()->set_bone_visible("grenade", vis_hud, TRUE);
    }

    IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual());
    VERIFY(pWeaponVisual);
    pWeaponVisual->LL_SetBoneVisible(pWeaponVisual->LL_BoneID("grenade"), vis_weap, TRUE);
}

BOOL CWeaponRPG7::net_Spawn(CSE_Abstract* DC)
{
    BOOL l_res = inherited::net_Spawn(DC);

    UpdateMissileVisibility();
    if (iAmmoElapsed && !getCurrentRocket())
        CRocketLauncher::SpawnRocket(m_sRocketSection, this);

    return l_res;
}

void CWeaponRPG7::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    UpdateMissileVisibility();
}

void CWeaponRPG7::UnloadMagazine(bool spawn_ammo)
{
    inherited::UnloadMagazine(spawn_ammo);
    UpdateMissileVisibility();
}

void CWeaponRPG7::ReloadMagazine()
{
    inherited::ReloadMagazine();

    if (iAmmoElapsed && !getRocketCount())
        CRocketLauncher::SpawnRocket(m_sRocketSection.c_str(), this);
}

void CWeaponRPG7::SwitchState(u32 S) { inherited::SwitchState(S); }
void CWeaponRPG7::FireStart() { inherited::FireStart(); }
#include "Inventory.h"
#include "InventoryOwner.h"
void CWeaponRPG7::switch2_Fire()
{
    m_iShotNum = 0;
    m_bFireSingleShot = true;
    bWorking = false;

    if (GetState() == eFire && getRocketCount())
    {
        Fvector p1, d1, p;
        Fvector p2, d2, d;
        p1.set(get_LastFP());
        d1.set(get_LastFD());
        p = p1;
        d = d1;
        CEntity* E = smart_cast<CEntity*>(H_Parent());
        if (E)
        {
            E->g_fireParams(this, p2, d2);
            p = p2;
            d = d2;

            if (IsHudModeNow())
            {
                Fvector p0;
                float dist = HUD().GetCurrentRayQuery().range;
                p0.mul(d2, dist);
                p0.add(p1);
                p = p1;
                d.sub(p0, p1);
                d.normalize_safe();
            }
        }

        Fmatrix launch_matrix;
        launch_matrix.identity();
        launch_matrix.k.set(d);
        Fvector::generate_orthonormal_basis(launch_matrix.k, launch_matrix.j, launch_matrix.i);
        launch_matrix.c.set(p);

        d.normalize();
        d.mul(m_fLaunchSpeed);

        CRocketLauncher::LaunchRocket(launch_matrix, d, zero_vel);

        CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
        VERIFY(pGrenade);
        pGrenade->SetInitiator(H_Parent()->ID());

        if (OnServer())
        {
            NET_Packet P;
            u_EventGen(P, GE_LAUNCH_ROCKET, ID());
            P.w_u16(u16(getCurrentRocket()->ID()));
            u_EventSend(P);
        }
    }
}

void CWeaponRPG7::PlayAnimReload()
{
    VERIFY(GetState() == eReload);
    PlayHUDMotion("anm_reload", FALSE, this, GetState());
}

void CWeaponRPG7::OnEvent(NET_Packet& P, u16 type)
{
    inherited::OnEvent(P, type);
    u16 id;
    switch (type)
    {
    case GE_OWNERSHIP_TAKE:
    {
        P.r_u16(id);
        CRocketLauncher::AttachRocket(id, this);
    }
    break;
    case GE_OWNERSHIP_REJECT:
    case GE_LAUNCH_ROCKET:
    {
        bool bLaunch = (type == GE_LAUNCH_ROCKET);
        P.r_u16(id);
        CRocketLauncher::DetachRocket(id, bLaunch);
        if (bLaunch)
            UpdateMissileVisibility();
    }
    break;
    }
}

void CWeaponRPG7::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);
    UpdateMissileVisibility();
}
