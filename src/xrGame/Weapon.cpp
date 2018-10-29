#include "StdAfx.h"
#include "Weapon.h"
#include "ParticlesObject.h"
#include "entity_alive.h"
#include "inventory_item_impl.h"
#include "Inventory.h"
#include "xrServer_Objects_ALife_Items.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "Level.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "Include/xrRender/Kinematics.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrPhysics/MathUtils.h"
#include "Common/object_broker.h"
#include "player_hud.h"
#include "GamePersistent.h"
#include "EffectorFall.h"
#include "debug_renderer.h"
#include "static_cast_checked.hpp"
#include "clsid_game.h"
#include "WeaponBinocularsVision.h"
#include "xrUICore/Windows/UIWindow.h"
#include "ui/UIXmlInit.h"
#include "Torch.h"
#include "xrNetServer/NET_Messages.h"
#include "xrCore/xr_token.h"

#define WEAPON_REMOVE_TIME 60000
#define ROTATION_TIME 0.25f

BOOL b_toggle_weapon_aim = FALSE;
extern CUIXml* pWpnScopeXml;

CWeapon::CWeapon()
{
    SetState(eHidden);
    SetNextState(eHidden);
    m_sub_state = eSubstateReloadBegin;
    m_bTriStateReload = false;
    SetDefaults();

    m_Offset.identity();
    m_StrapOffset.identity();

    m_iAmmoCurrentTotal = 0;
    m_BriefInfo_CalcFrame = 0;

    iAmmoElapsed = -1;
    iMagazineSize = -1;
    m_ammoType = 0;

    eHandDependence = hdNone;

    m_zoom_params.m_fCurrentZoomFactor = g_fov;
    m_zoom_params.m_fZoomRotationFactor = 0.f;
    m_zoom_params.m_pVision = nullptr;
    m_zoom_params.m_pNight_vision = nullptr;

    m_pCurrentAmmo = nullptr;

    m_pFlameParticles2 = nullptr;
    m_sFlameParticles2 = nullptr;

    m_fCurrentCartirdgeDisp = 1.f;

    m_strap_bone0 = nullptr;
    m_strap_bone1 = nullptr;
    m_StrapOffset.identity();
    m_strapped_mode = false;
    m_can_be_strapped = false;
    m_ef_main_weapon_type = u32(-1);
    m_ef_weapon_type = u32(-1);
    m_UIScope = nullptr;
    m_set_next_ammoType_on_reload = undefined_ammo_type;
    m_crosshair_inertion = 0.f;
    m_activation_speed_is_overriden = false;
    m_cur_scope = 0;
    m_bRememberActorNVisnStatus = false;
}

CWeapon::~CWeapon()
{
    xr_delete(m_UIScope);
    delete_data(m_scopes);
}

void CWeapon::Hit(SHit* pHDS) { inherited::Hit(pHDS); }
void CWeapon::UpdateXForm()
{
    if (Device.dwFrame == dwXF_Frame)
        return;

    dwXF_Frame = Device.dwFrame;

    if (!H_Parent())
        return;

    // Get access to entity and its visual
    CEntityAlive* E = smart_cast<CEntityAlive*>(H_Parent());

    if (!E)
    {
        if (!IsGameTypeSingle())
            UpdatePosition(H_Parent()->XFORM());

        return;
    }

    const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
    if (parent && parent->use_simplified_visual())
        return;

    if (parent->attached(this))
        return;

    IKinematics* V = smart_cast<IKinematics*>(E->Visual());
    VERIFY(V);

    // Get matrices
    int boneL = -1, boneR = -1, boneR2 = -1;

    // this ugly case is possible in case of a CustomMonster, not a Stalker, nor an Actor
    E->g_WeaponBones(boneL, boneR, boneR2);

    if (boneR == -1)
        return;

    if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
        boneL = boneR2;

    V->CalculateBones();
    Fmatrix& mL = V->LL_GetTransform(u16(boneL));
    Fmatrix& mR = V->LL_GetTransform(u16(boneR));
    // Calculate
    Fmatrix mRes;
    Fvector R, D, N;
    D.sub(mL.c, mR.c);

    if (fis_zero(D.magnitude()))
    {
        mRes.set(E->XFORM());
        mRes.c.set(mR.c);
    }
    else
    {
        D.normalize();
        R.crossproduct(mR.j, D);

        N.crossproduct(D, R);
        N.normalize();

        mRes.set(R, N, D, mR.c);
        mRes.mulA_43(E->XFORM());
    }

    UpdatePosition(mRes);
}

void CWeapon::UpdateFireDependencies_internal()
{
    if (Device.dwFrame != dwFP_Frame)
    {
        dwFP_Frame = Device.dwFrame;

        UpdateXForm();

        if (GetHUDmode())
        {
            HudItemData()->setup_firedeps(m_current_firedeps);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
        }
        else
        {
            // 3rd person or no parent
            Fmatrix& parent = XFORM();
            Fvector& fp = vLoadedFirePoint;
            Fvector& fp2 = vLoadedFirePoint2;
            Fvector& sp = vLoadedShellPoint;

            parent.transform_tiny(m_current_firedeps.vLastFP, fp);
            parent.transform_tiny(m_current_firedeps.vLastFP2, fp2);
            parent.transform_tiny(m_current_firedeps.vLastSP, sp);

            m_current_firedeps.vLastFD.set(0.f, 0.f, 1.f);
            parent.transform_dir(m_current_firedeps.vLastFD);

            m_current_firedeps.m_FireParticlesXForm.set(parent);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));
        }
    }
}

void CWeapon::ForceUpdateFireParticles()
{
    if (!GetHUDmode())
    { // update particlesXFORM real bullet direction

        if (!H_Parent())
            return;

        Fvector p, d;
        smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

        Fmatrix _pxf;
        _pxf.k = d;
        _pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
        _pxf.j.crossproduct(_pxf.k, _pxf.i);
        _pxf.c = XFORM().c;

        m_current_firedeps.m_FireParticlesXForm.set(_pxf);
    }
}

void CWeapon::Load(LPCSTR section)
{
    inherited::Load(section);
    CShootingObject::Load(section);

    if (pSettings->line_exist(section, "flame_particles_2"))
        m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

    // load ammo classes
    m_ammoTypes.clear();
    LPCSTR S = pSettings->r_string(section, "ammo_class");
    if (S && S[0])
    {
        string128 _ammoItem;
        int count = _GetItemCount(S);
        for (int it = 0; it < count; ++it)
        {
            _GetItem(S, it, _ammoItem);
            m_ammoTypes.push_back(_ammoItem);
        }
    }

    iAmmoElapsed = pSettings->r_s32(section, "ammo_elapsed");
    iMagazineSize = pSettings->r_s32(section, "ammo_mag_size");

    ////////////////////////////////////////////////////
    // дисперсия стрельбы

    //подбрасывание камеры во время отдачи
    u8 rm = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return", 1);
    cam_recoil.ReturnMode = (rm == 1);

    rm = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return_stop", 0);
    cam_recoil.StopReturn = (rm == 1);

    float temp_f = 0.0f;
    temp_f = pSettings->r_float(section, "cam_relax_speed");
    cam_recoil.RelaxSpeed = _abs(deg2rad(temp_f));
    VERIFY(!fis_zero(cam_recoil.RelaxSpeed));
    if (fis_zero(cam_recoil.RelaxSpeed))
    {
        cam_recoil.RelaxSpeed = EPS_L;
    }

    cam_recoil.RelaxSpeed_AI = cam_recoil.RelaxSpeed;
    if (pSettings->line_exist(section, "cam_relax_speed_ai"))
    {
        temp_f = pSettings->r_float(section, "cam_relax_speed_ai");
        cam_recoil.RelaxSpeed_AI = _abs(deg2rad(temp_f));
        VERIFY(!fis_zero(cam_recoil.RelaxSpeed_AI));
        if (fis_zero(cam_recoil.RelaxSpeed_AI))
        {
            cam_recoil.RelaxSpeed_AI = EPS_L;
        }
    }
    temp_f = pSettings->r_float(section, "cam_max_angle");
    cam_recoil.MaxAngleVert = _abs(deg2rad(temp_f));
    VERIFY(!fis_zero(cam_recoil.MaxAngleVert));
    if (fis_zero(cam_recoil.MaxAngleVert))
    {
        cam_recoil.MaxAngleVert = EPS;
    }

    temp_f = pSettings->r_float(section, "cam_max_angle_horz");
    cam_recoil.MaxAngleHorz = _abs(deg2rad(temp_f));
    VERIFY(!fis_zero(cam_recoil.MaxAngleHorz));
    if (fis_zero(cam_recoil.MaxAngleHorz))
    {
        cam_recoil.MaxAngleHorz = EPS;
    }

    temp_f = pSettings->r_float(section, "cam_step_angle_horz");
    cam_recoil.StepAngleHorz = deg2rad(temp_f);

    cam_recoil.DispersionFrac = _abs(READ_IF_EXISTS(pSettings, r_float, section, "cam_dispersion_frac", 0.7f));

    //подбрасывание камеры во время отдачи в режиме zoom ==> ironsight or scope
    // zoom_cam_recoil.Clone( cam_recoil ); ==== нельзя !!!!!!!!!!
    zoom_cam_recoil.RelaxSpeed = cam_recoil.RelaxSpeed;
    zoom_cam_recoil.RelaxSpeed_AI = cam_recoil.RelaxSpeed_AI;
    zoom_cam_recoil.DispersionFrac = cam_recoil.DispersionFrac;
    zoom_cam_recoil.MaxAngleVert = cam_recoil.MaxAngleVert;
    zoom_cam_recoil.MaxAngleHorz = cam_recoil.MaxAngleHorz;
    zoom_cam_recoil.StepAngleHorz = cam_recoil.StepAngleHorz;

    zoom_cam_recoil.ReturnMode = cam_recoil.ReturnMode;
    zoom_cam_recoil.StopReturn = cam_recoil.StopReturn;

    if (pSettings->line_exist(section, "zoom_cam_relax_speed"))
    {
        zoom_cam_recoil.RelaxSpeed = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed")));
        VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed));
        if (fis_zero(zoom_cam_recoil.RelaxSpeed))
        {
            zoom_cam_recoil.RelaxSpeed = EPS_L;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_relax_speed_ai"))
    {
        zoom_cam_recoil.RelaxSpeed_AI = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed_ai")));
        VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed_AI));
        if (fis_zero(zoom_cam_recoil.RelaxSpeed_AI))
        {
            zoom_cam_recoil.RelaxSpeed_AI = EPS_L;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_max_angle"))
    {
        zoom_cam_recoil.MaxAngleVert = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle")));
        VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleVert));
        if (fis_zero(zoom_cam_recoil.MaxAngleVert))
        {
            zoom_cam_recoil.MaxAngleVert = EPS;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_max_angle_horz"))
    {
        zoom_cam_recoil.MaxAngleHorz = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle_horz")));
        VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleHorz));
        if (fis_zero(zoom_cam_recoil.MaxAngleHorz))
        {
            zoom_cam_recoil.MaxAngleHorz = EPS;
        }
    }
    if (pSettings->line_exist(section, "zoom_cam_step_angle_horz"))
    {
        zoom_cam_recoil.StepAngleHorz = deg2rad(pSettings->r_float(section, "zoom_cam_step_angle_horz"));
    }
    if (pSettings->line_exist(section, "zoom_cam_dispersion_frac"))
    {
        zoom_cam_recoil.DispersionFrac = _abs(pSettings->r_float(section, "zoom_cam_dispersion_frac"));
    }

    m_pdm.m_fPDM_disp_base = pSettings->r_float(section, "PDM_disp_base");
    m_pdm.m_fPDM_disp_vel_factor = pSettings->r_float(section, "PDM_disp_vel_factor");
    m_pdm.m_fPDM_disp_accel_factor = pSettings->r_float(section, "PDM_disp_accel_factor");
    m_pdm.m_fPDM_disp_crouch = pSettings->r_float(section, "PDM_disp_crouch");
    m_pdm.m_fPDM_disp_crouch_no_acc = pSettings->r_float(section, "PDM_disp_crouch_no_acc");
    m_crosshair_inertion = READ_IF_EXISTS(pSettings, r_float, section, "crosshair_inertion", 5.91f);
    m_first_bullet_controller.load(section);
    fireDispersionConditionFactor = pSettings->r_float(section, "fire_dispersion_condition_factor");

    // modified by Peacemaker [17.10.08]
    //	misfireProbability			  = pSettings->r_float(section,"misfire_probability");
    //	misfireConditionK			  = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k",	1.0f);
    misfireStartCondition = pSettings->r_float(section, "misfire_start_condition");
    misfireEndCondition = READ_IF_EXISTS(pSettings, r_float, section, "misfire_end_condition", 0.f);
    misfireStartProbability = READ_IF_EXISTS(pSettings, r_float, section, "misfire_start_prob", 0.f);
    misfireEndProbability = pSettings->r_float(section, "misfire_end_prob");
    conditionDecreasePerShot = pSettings->r_float(section, "condition_shot_dec");
    conditionDecreasePerQueueShot =
        READ_IF_EXISTS(pSettings, r_float, section, "condition_queue_shot_dec", conditionDecreasePerShot);

    vLoadedFirePoint = pSettings->r_fvector3(section, "fire_point");

    if (pSettings->line_exist(section, "fire_point2"))
        vLoadedFirePoint2 = pSettings->r_fvector3(section, "fire_point2");
    else
        vLoadedFirePoint2 = vLoadedFirePoint;

    // hands
    eHandDependence = EHandDependence(pSettings->r_s32(section, "hand_dependence"));
    m_bIsSingleHanded = true;
    if (pSettings->line_exist(section, "single_handed"))
        m_bIsSingleHanded = !!pSettings->r_bool(section, "single_handed");
    //
    m_fMinRadius = pSettings->r_float(section, "min_radius");
    m_fMaxRadius = pSettings->r_float(section, "max_radius");

    // информация о возможных апгрейдах и их визуализации в инвентаре
    m_eScopeStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "scope_status");
    m_eSilencerStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "silencer_status");
    m_eGrenadeLauncherStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "grenade_launcher_status");

    m_zoom_params.m_bZoomEnabled = !!pSettings->r_bool(section, "zoom_enabled");
    m_zoom_params.m_fZoomRotateTime = pSettings->r_float(section, "zoom_rotate_time");

    if (m_eScopeStatus == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(section, "scopes_sect"))
        {
            LPCSTR str = pSettings->r_string(section, "scopes_sect");
            for (int i = 0, count = _GetItemCount(str); i < count; ++i)
            {
                string128 scope_section;
                _GetItem(str, i, scope_section);
                m_scopes.push_back(scope_section);
            }
        }
        else
        {
            m_scopes.push_back(section);
        }
    }
    else if (m_eScopeStatus == ALife::eAddonPermanent)
    {
        shared_str scope_tex_name = pSettings->r_string(cNameSect(), "scope_texture");
        m_zoom_params.m_fScopeZoomFactor = pSettings->r_float(cNameSect(), "scope_zoom_factor");
        if (!GEnv.isDedicatedServer)
        {
            m_UIScope = new CUIWindow();
            if (!pWpnScopeXml)
            {
                pWpnScopeXml = new CUIXml();
                pWpnScopeXml->Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "scopes.xml");
            }
            CUIXmlInit::InitWindow(*pWpnScopeXml, scope_tex_name.c_str(), 0, m_UIScope);
        }
    }

    if (m_eSilencerStatus == ALife::eAddonAttachable)
    {
        m_sSilencerName = pSettings->r_string(section, "silencer_name");
        m_iSilencerX = pSettings->r_s32(section, "silencer_x");
        m_iSilencerY = pSettings->r_s32(section, "silencer_y");
    }

    if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
    {
        m_sGrenadeLauncherName = pSettings->r_string(section, "grenade_launcher_name");
        m_iGrenadeLauncherX = pSettings->r_s32(section, "grenade_launcher_x");
        m_iGrenadeLauncherY = pSettings->r_s32(section, "grenade_launcher_y");
    }

    InitAddons();
    if (pSettings->line_exist(section, "weapon_remove_time"))
        m_dwWeaponRemoveTime = pSettings->r_u32(section, "weapon_remove_time");
    else
        m_dwWeaponRemoveTime = WEAPON_REMOVE_TIME;

    if (pSettings->line_exist(section, "auto_spawn_ammo"))
        m_bAutoSpawnAmmo = pSettings->r_bool(section, "auto_spawn_ammo");
    else
        m_bAutoSpawnAmmo = TRUE;

    m_zoom_params.m_bHideCrosshairInZoom = true;

    if (pSettings->line_exist(hud_sect, "zoom_hide_crosshair"))
        m_zoom_params.m_bHideCrosshairInZoom = !!pSettings->r_bool(hud_sect, "zoom_hide_crosshair");

    Fvector def_dof;
    def_dof.set(-1, -1, -1);
    m_zoom_params.m_ZoomDof = READ_IF_EXISTS(pSettings, r_fvector3, section, "zoom_dof",Fvector().set(-1, -1, -1));
    m_zoom_params.m_bZoomDofEnabled = !def_dof.similar(m_zoom_params.m_ZoomDof);

    m_zoom_params.m_ReloadDof = READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_dof",Fvector4().set(-1, -1, -1, -1));
    m_zoom_params.m_ReloadEmptyDof = READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_empty_dof", Fvector4().set(-1, -1, -1, -1));

    m_bHasTracers = !!READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
    m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

    string256 temp;
    for (int i = egdNovice; i < egdCount; ++i)
    {
        strconcat(sizeof(temp), temp, "hit_probability_", get_token_name(difficulty_type_token, i));
        m_hit_probability[i] = READ_IF_EXISTS(pSettings, r_float, section, temp, 1.f);
    }

    m_zoom_params.m_bUseDynamicZoom = READ_IF_EXISTS(pSettings, r_bool, section, "scope_dynamic_zoom", false);
    m_zoom_params.m_sUseZoomPostprocess = nullptr;
    m_zoom_params.m_sUseBinocularVision = nullptr;

    // Added by Axel, to enable optional condition use on any item
    m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", true));
}

void CWeapon::LoadFireParams(LPCSTR section)
{
    cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "cam_dispersion"));
    cam_recoil.DispersionInc = 0.0f;

    if (pSettings->line_exist(section, "cam_dispersion_inc"))
    {
        cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "cam_dispersion_inc"));
    }

    zoom_cam_recoil.Dispersion = cam_recoil.Dispersion;
    zoom_cam_recoil.DispersionInc = cam_recoil.DispersionInc;

    if (pSettings->line_exist(section, "zoom_cam_dispersion"))
    {
        zoom_cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion"));
    }
    if (pSettings->line_exist(section, "zoom_cam_dispersion_inc"))
    {
        zoom_cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion_inc"));
    }

    CShootingObject::LoadFireParams(section);
};

BOOL CWeapon::net_Spawn(CSE_Abstract* DC)
{
    m_fRTZoomFactor = m_zoom_params.m_fScopeZoomFactor;
    BOOL bResult = inherited::net_Spawn(DC);
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeItemWeapon* E = smart_cast<CSE_ALifeItemWeapon*>(e);

    // iAmmoCurrent					= E->a_current;
    iAmmoElapsed = E->a_elapsed;
    m_flagsAddOnState = E->m_addon_flags.get();
    m_ammoType = E->ammo_type;
    SetState(E->wpn_state);
    SetNextState(E->wpn_state);

    m_DefaultCartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
    if (iAmmoElapsed)
    {
        m_fCurrentCartirdgeDisp = m_DefaultCartridge.param_s.kDisp;
        for (int i = 0; i < iAmmoElapsed; ++i)
            m_magazine.push_back(m_DefaultCartridge);
    }

    UpdateAddonsVisibility();
    InitAddons();

    m_dwWeaponIndependencyTime = 0;

    VERIFY((u32)iAmmoElapsed == m_magazine.size());
    m_bAmmoWasSpawned = false;

    return bResult;
}

void CWeapon::net_Destroy()
{
    inherited::net_Destroy();

    //удалить объекты партиклов
    StopFlameParticles();
    StopFlameParticles2();
    StopLight();
    Light_Destroy();

    while (m_magazine.size())
        m_magazine.pop_back();
}

BOOL CWeapon::IsUpdating()
{
    bool bIsActiveItem = m_pInventory && m_pInventory->ActiveItem() == this;
    return bIsActiveItem || bWorking; // || IsPending() || getVisible();
}

void CWeapon::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);

    P.w_float_q8(GetCondition(), 0.0f, 1.0f);

    u8 need_upd = IsUpdating() ? 1 : 0;
    P.w_u8(need_upd);
    P.w_u16(u16(iAmmoElapsed));
    P.w_u8(m_flagsAddOnState);
    P.w_u8(m_ammoType);
    P.w_u8((u8)GetState());
    P.w_u8((u8)IsZoomed());
}

void CWeapon::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);

    float _cond;
    P.r_float_q8(_cond, 0.0f, 1.0f);
    SetCondition(_cond);

    u8 flags = 0;
    P.r_u8(flags);

    u16 ammo_elapsed = 0;
    P.r_u16(ammo_elapsed);

    u8 NewAddonState;
    P.r_u8(NewAddonState);

    m_flagsAddOnState = NewAddonState;
    UpdateAddonsVisibility();

    u8 ammoType, wstate;
    P.r_u8(ammoType);
    P.r_u8(wstate);

    u8 Zoom;
    P.r_u8(Zoom);

    if (H_Parent() && H_Parent()->Remote())
    {
        if (Zoom)
            OnZoomIn();
        else
            OnZoomOut();
    };
    switch (wstate)
    {
    case eFire:
    case eFire2:
    case eSwitch:
    case eReload: {
    }
    break;
    default:
    {
        if (ammoType >= m_ammoTypes.size())
            Msg("!! Weapon [%d], State - [%d]", ID(), wstate);
        else
        {
            m_ammoType = ammoType;
            SetAmmoElapsed((ammo_elapsed));
        }
    }
    break;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeapon::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    save_data(iAmmoElapsed, output_packet);
    save_data(m_cur_scope, output_packet);
    save_data(m_flagsAddOnState, output_packet);
    save_data(m_ammoType, output_packet);
    save_data(m_zoom_params.m_bIsZoomModeNow, output_packet);
    save_data(m_bRememberActorNVisnStatus, output_packet);
}

void CWeapon::load(IReader& input_packet)
{
    inherited::load(input_packet);
    load_data(iAmmoElapsed, input_packet);
    load_data(m_cur_scope, input_packet);
    load_data(m_flagsAddOnState, input_packet);
    UpdateAddonsVisibility();
    load_data(m_ammoType, input_packet);
    load_data(m_zoom_params.m_bIsZoomModeNow, input_packet);

    if (m_zoom_params.m_bIsZoomModeNow)
        OnZoomIn();
    else
        OnZoomOut();

    load_data(m_bRememberActorNVisnStatus, input_packet);
}

void CWeapon::OnEvent(NET_Packet& P, u16 type)
{
    switch (type)
    {
    case GE_ADDON_CHANGE:
    {
        P.r_u8(m_flagsAddOnState);
        InitAddons();
        UpdateAddonsVisibility();
    }
    break;

    case GE_WPN_STATE_CHANGE:
    {
        u8 state;
        P.r_u8(state);
        P.r_u8(m_sub_state);
        //			u8 NewAmmoType =
        P.r_u8();
        u8 AmmoElapsed = P.r_u8();
        u8 NextAmmo = P.r_u8();
        if (NextAmmo == undefined_ammo_type)
            m_set_next_ammoType_on_reload = undefined_ammo_type;
        else
            m_set_next_ammoType_on_reload = NextAmmo;

        if (OnClient())
            SetAmmoElapsed(int(AmmoElapsed));
        OnStateSwitch(u32(state), GetState());
    }
    break;
    default: { inherited::OnEvent(P, type);
    }
    break;
    }
};

void CWeapon::shedule_Update(u32 dT)
{
    // Queue shrink
    //	u32	dwTimeCL		= Level().timeServer()-NET_Latency;
    //	while ((NET.size()>2) && (NET[1].dwTimeStamp<dwTimeCL)) NET.pop_front();

    // Inherited
    inherited::shedule_Update(dT);
}

void CWeapon::OnH_B_Independent(bool just_before_destroy)
{
    RemoveShotEffector();

    inherited::OnH_B_Independent(just_before_destroy);

    FireEnd();
    SetPending(FALSE);
    SwitchState(eHidden);

    m_strapped_mode = false;
    m_zoom_params.m_bIsZoomModeNow = false;
    UpdateXForm();
}

void CWeapon::OnH_A_Independent()
{
    m_dwWeaponIndependencyTime = Level().timeServer();
    inherited::OnH_A_Independent();
    Light_Destroy();
    UpdateAddonsVisibility();
};

void CWeapon::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    UpdateAddonsVisibility();
};

void CWeapon::OnActiveItem()
{
    //. from Activate
    UpdateAddonsVisibility();
    m_BriefInfo_CalcFrame = 0;

    //. Show
    SwitchState(eShowing);
    //-

    inherited::OnActiveItem();
    //если мы занружаемся и оружие было в руках
    //.	SetState					(eIdle);
    //.	SetNextState				(eIdle);
}

void CWeapon::OnHiddenItem()
{
    m_BriefInfo_CalcFrame = 0;

    if (IsGameTypeSingle())
        SwitchState(eHiding);
    else
        SwitchState(eHidden);

    OnZoomOut();
    inherited::OnHiddenItem();

    m_set_next_ammoType_on_reload = undefined_ammo_type;
}

void CWeapon::SendHiddenItem()
{
    if (!CHudItem::object().getDestroy() && m_pInventory)
    {
        // !!! Just single entry for given state !!!
        NET_Packet P;
        CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
        P.w_u8(u8(eHiding));
        P.w_u8(u8(m_sub_state));
        P.w_u8(m_ammoType);
        P.w_u8(u8(iAmmoElapsed & 0xff));
        P.w_u8(m_set_next_ammoType_on_reload);
        CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
        SetPending(TRUE);
    }
}

void CWeapon::OnH_B_Chield()
{
    m_dwWeaponIndependencyTime = 0;
    inherited::OnH_B_Chield();

    OnZoomOut();
    m_set_next_ammoType_on_reload = undefined_ammo_type;
}

extern u32 hud_adj_mode;
bool CWeapon::AllowBore() { return true; }
void CWeapon::UpdateCL()
{
    inherited::UpdateCL();
    UpdateHUDAddonsVisibility();
    //подсветка от выстрела
    UpdateLight();

    //нарисовать партиклы
    UpdateFlameParticles();
    UpdateFlameParticles2();

    if (!IsGameTypeSingle())
        make_Interpolation();

    if ((GetNextState() == GetState()) && IsGameTypeSingle() && H_Parent() == Level().CurrentEntity())
    {
        CActor* pActor = smart_cast<CActor*>(H_Parent());
        if (pActor && !pActor->AnyMove() && this == pActor->inventory().ActiveItem())
        {
            if (hud_adj_mode == 0 && GetState() == eIdle && (Device.dwTimeGlobal - m_dw_curr_substate_time > 20000) &&
                !IsZoomed() && g_player_hud->attached_item(1) == nullptr)
            {
                if (AllowBore())
                    SwitchState(eBore);

                ResetSubStateTime();
            }
        }
    }

    if (m_zoom_params.m_pNight_vision && !need_renderable())
    {
        if (!m_zoom_params.m_pNight_vision->IsActive())
        {
            CActor* pA = smart_cast<CActor*>(H_Parent());
            R_ASSERT(pA);
            CTorch* pTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
            if (pTorch && pTorch->GetNightVisionStatus())
            {
                m_bRememberActorNVisnStatus = pTorch->GetNightVisionStatus();
                pTorch->SwitchNightVision(false, false);
            }
            m_zoom_params.m_pNight_vision->Start(m_zoom_params.m_sUseZoomPostprocess, pA, false);
        }
    }
    else if (m_bRememberActorNVisnStatus)
    {
        m_bRememberActorNVisnStatus = false;
        EnableActorNVisnAfterZoom();
    }

    if (m_zoom_params.m_pVision)
        m_zoom_params.m_pVision->Update();
}
void CWeapon::EnableActorNVisnAfterZoom()
{
    CActor* pA = smart_cast<CActor*>(H_Parent());
    if (IsGameTypeSingle() && !pA)
        pA = g_actor;

    if (pA)
    {
        CTorch* pTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
        if (pTorch)
        {
            pTorch->SwitchNightVision(true, false);
            pTorch->GetNightVision()->PlaySounds(CNightVisionEffector::eIdleSound);
        }
    }
}

bool CWeapon::need_renderable() { return !(IsZoomed() && ZoomTexture() && !IsRotatingToZoom()); }
void CWeapon::renderable_Render()
{
    UpdateXForm();

    //нарисовать подсветку

    RenderLight();

    //если мы в режиме снайперки, то сам HUD рисовать не надо
    if (IsZoomed() && !IsRotatingToZoom() && ZoomTexture())
        RenderHud(FALSE);
    else
        RenderHud(TRUE);

    inherited::renderable_Render();
}

void CWeapon::signal_HideComplete()
{
    if (H_Parent())
        setVisible(FALSE);
    SetPending(FALSE);
}

void CWeapon::SetDefaults()
{
    SetPending(FALSE);

    m_flags.set(FUsingCondition, TRUE);
    bMisfire = false;
    m_flagsAddOnState = 0;
    m_zoom_params.m_bIsZoomModeNow = false;
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
    Position().set(trans.c);
    XFORM().mul(trans, m_strapped_mode ? m_StrapOffset : m_Offset);
    VERIFY(!fis_zero(DET(renderable.xform)));
}

bool CWeapon::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;

    switch (cmd)
    {
    case kWPN_FIRE:
    {
        //если оружие чем-то занято, то ничего не делать
        {
            if (IsPending())
                return false;

            if (flags & CMD_START)
                FireStart();
            else
                FireEnd();
        };
    }
        return true;
    case kWPN_NEXT: { return SwitchAmmoType(flags);
    }

    case kWPN_ZOOM:
        if (IsZoomEnabled())
        {
            if (b_toggle_weapon_aim)
            {
                if (flags & CMD_START)
                {
                    if (!IsZoomed())
                    {
                        if (!IsPending())
                        {
                            if (GetState() != eIdle)
                                SwitchState(eIdle);
                            OnZoomIn();
                        }
                    }
                    else
                        OnZoomOut();
                }
            }
            else
            {
                if (flags & CMD_START)
                {
                    if (!IsZoomed() && !IsPending())
                    {
                        if (GetState() != eIdle)
                            SwitchState(eIdle);
                        OnZoomIn();
                    }
                }
                else if (IsZoomed())
                    OnZoomOut();
            }
            return true;
        }
        else
            return false;

    case kWPN_ZOOM_INC:
    case kWPN_ZOOM_DEC:
        if (IsZoomEnabled() && IsZoomed() && (flags&CMD_START) )
        {
            if (cmd == kWPN_ZOOM_INC)
                ZoomInc();
            else
                ZoomDec();
            return true;
        }
        else
            return false;
    }
    return false;
}

bool CWeapon::SwitchAmmoType(u32 flags)
{
    if (IsPending() || OnClient())
        return false;

    if (!(flags & CMD_START))
        return false;

    u8 l_newType = m_ammoType;
    bool b1, b2;
    do
    {
        l_newType = u8((u32(l_newType + 1)) % m_ammoTypes.size());
        b1 = (l_newType != m_ammoType);
        b2 = unlimited_ammo() ? false : (!m_pInventory->GetAny(m_ammoTypes[l_newType].c_str()));
    } while (b1 && b2);

    if (l_newType != m_ammoType)
    {
        m_set_next_ammoType_on_reload = l_newType;
        if (OnServer())
        {
            Reload();
        }
    }
    return true;
}

void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, u32 ParentID)
{
    if (!m_ammoTypes.size())
        return;
    if (OnClient())
        return;
    m_bAmmoWasSpawned = true;

    int l_type = 0;
    l_type %= m_ammoTypes.size();

    if (!ammoSect)
        ammoSect = m_ammoTypes[l_type].c_str();

    ++l_type;
    l_type %= m_ammoTypes.size();

    CSE_Abstract* D = F_entity_Create(ammoSect);

    {
        CSE_ALifeItemAmmo* l_pA = smart_cast<CSE_ALifeItemAmmo*>(D);
        R_ASSERT(l_pA);
        l_pA->m_boxSize = (u16)pSettings->r_s32(ammoSect, "box_size");
        D->s_name = ammoSect;
        D->set_name_replace("");
        //.		D->s_gameid					= u8(GameID());
        D->s_RP = 0xff;
        D->ID = 0xffff;
        if (ParentID == 0xffffffff)
            D->ID_Parent = (u16)H_Parent()->ID();
        else
            D->ID_Parent = (u16)ParentID;

        D->ID_Phantom = 0xffff;
        D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
        D->RespawnTime = 0;
        l_pA->m_tNodeID = GEnv.isDedicatedServer ? u32(-1) : ai_location().level_vertex_id();

        if (boxCurr == 0xffffffff)
            boxCurr = l_pA->m_boxSize;

        while (boxCurr)
        {
            l_pA->a_elapsed = (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
            NET_Packet P;
            D->Spawn_Write(P, TRUE);
            Level().Send(P, net_flags(TRUE));

            if (boxCurr > l_pA->m_boxSize)
                boxCurr -= l_pA->m_boxSize;
            else
                boxCurr = 0;
        }
    }
    F_entity_Destroy(D);
}

int CWeapon::GetSuitableAmmoTotal(bool use_item_to_spawn) const
{
    int ae_count = iAmmoElapsed;
    if (!m_pInventory)
    {
        return ae_count;
    }

    //чтоб не делать лишних пересчетов
    if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
    {
        return ae_count + m_iAmmoCurrentTotal;
    }
    m_BriefInfo_CalcFrame = Device.dwFrame;

    m_iAmmoCurrentTotal = 0;
    for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
    {
        m_iAmmoCurrentTotal += GetAmmoCount_forType(m_ammoTypes[i]);

        if (!use_item_to_spawn)
        {
            continue;
        }
        if (!inventory_owner().item_to_spawn())
        {
            continue;
        }
        m_iAmmoCurrentTotal += inventory_owner().ammo_in_box_to_spawn();
    }
    return ae_count + m_iAmmoCurrentTotal;
}

int CWeapon::GetAmmoCount(u8 ammo_type) const
{
    VERIFY(m_pInventory);
    R_ASSERT(ammo_type < m_ammoTypes.size());

    return GetAmmoCount_forType(m_ammoTypes[ammo_type]);
}

int CWeapon::GetAmmoCount_forType(shared_str const& ammo_type) const
{
    int res = 0;

    TIItemContainer::iterator itb = m_pInventory->m_belt.begin();
    TIItemContainer::iterator ite = m_pInventory->m_belt.end();
    for (; itb != ite; ++itb)
    {
        CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(*itb);
        if (pAmmo && (pAmmo->cNameSect() == ammo_type))
        {
            res += pAmmo->m_boxCurr;
        }
    }

    itb = m_pInventory->m_ruck.begin();
    ite = m_pInventory->m_ruck.end();
    for (; itb != ite; ++itb)
    {
        CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(*itb);
        if (pAmmo && (pAmmo->cNameSect() == ammo_type))
        {
            res += pAmmo->m_boxCurr;
        }
    }
    return res;
}

float CWeapon::GetConditionMisfireProbability() const
{
    // modified by Peacemaker [17.10.08]
    //	if(GetCondition() > 0.95f)
    //		return 0.0f;
    if (GetCondition() > misfireStartCondition)
        return 0.0f;
    if (GetCondition() < misfireEndCondition)
        return misfireEndProbability;
    //	float mis = misfireProbability+powf(1.f-GetCondition(), 3.f)*misfireConditionK;
    float mis = misfireStartProbability +
        ((misfireStartCondition - GetCondition()) * // condition goes from 1.f to 0.f
            (misfireEndProbability - misfireStartProbability) / // probability goes from 0.f to 1.f
            ((misfireStartCondition == misfireEndCondition) ? // !!!say "No" to devision by zero
                    misfireStartCondition :
                    (misfireStartCondition - misfireEndCondition)));
    clamp(mis, 0.0f, 0.99f);
    return mis;
}

BOOL CWeapon::CheckForMisfire()
{
    if (OnClient())
        return FALSE;

    float rnd = ::Random.randF(0.f, 1.f);
    float mp = GetConditionMisfireProbability();
    if (rnd < mp)
    {
        FireEnd();

        bMisfire = true;
        SwitchState(eMisfire);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL CWeapon::IsMisfire() const { return bMisfire; }
void CWeapon::Reload() { OnZoomOut(); }
bool CWeapon::IsGrenadeLauncherAttached() const
{
    return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
               0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) ||
        ALife::eAddonPermanent == m_eGrenadeLauncherStatus;
}

bool CWeapon::IsScopeAttached() const
{
    return (ALife::eAddonAttachable == m_eScopeStatus &&
               0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope)) ||
        ALife::eAddonPermanent == m_eScopeStatus;
}

bool CWeapon::IsSilencerAttached() const
{
    return (ALife::eAddonAttachable == m_eSilencerStatus &&
               0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer)) ||
        ALife::eAddonPermanent == m_eSilencerStatus;
}

bool CWeapon::GrenadeLauncherAttachable() { return (ALife::eAddonAttachable == m_eGrenadeLauncherStatus); }
bool CWeapon::ScopeAttachable() { return (ALife::eAddonAttachable == m_eScopeStatus); }
bool CWeapon::SilencerAttachable() { return (ALife::eAddonAttachable == m_eSilencerStatus); }
shared_str wpn_scope = "wpn_scope";
shared_str wpn_silencer = "wpn_silencer";
shared_str wpn_grenade_launcher = "wpn_launcher";

void CWeapon::UpdateHUDAddonsVisibility()
{ // actor only
    if (!GetHUDmode())
        return;

    //.	return;

    if (ScopeAttachable())
    {
        HudItemData()->set_bone_visible(wpn_scope, IsScopeAttached());
    }

    if (m_eScopeStatus == ALife::eAddonDisabled)
    {
        HudItemData()->set_bone_visible(wpn_scope, FALSE, TRUE);
    }
    else if (m_eScopeStatus == ALife::eAddonPermanent)
        HudItemData()->set_bone_visible(wpn_scope, TRUE, TRUE);

    if (SilencerAttachable())
    {
        HudItemData()->set_bone_visible(wpn_silencer, IsSilencerAttached());
    }
    if (m_eSilencerStatus == ALife::eAddonDisabled)
    {
        HudItemData()->set_bone_visible(wpn_silencer, FALSE, TRUE);
    }
    else if (m_eSilencerStatus == ALife::eAddonPermanent)
        HudItemData()->set_bone_visible(wpn_silencer, TRUE, TRUE);

    if (GrenadeLauncherAttachable())
    {
        HudItemData()->set_bone_visible(wpn_grenade_launcher, IsGrenadeLauncherAttached());
    }
    if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled)
    {
        HudItemData()->set_bone_visible(wpn_grenade_launcher, FALSE, TRUE);
    }
    else if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
        HudItemData()->set_bone_visible(wpn_grenade_launcher, TRUE, TRUE);
}

void CWeapon::UpdateAddonsVisibility()
{
    IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual());
    R_ASSERT(pWeaponVisual);

    u16 bone_id;
    UpdateHUDAddonsVisibility();

    pWeaponVisual->CalculateBones_Invalidate();

    bone_id = pWeaponVisual->LL_BoneID(wpn_scope);
    if (ScopeAttachable())
    {
        if (IsScopeAttached())
        {
            if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
                pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
        }
        else
        {
            if (pWeaponVisual->LL_GetBoneVisible(bone_id))
                pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
        }
    }
    if (m_eScopeStatus == ALife::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
    {
        pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
        //		Log("scope", pWeaponVisual->LL_GetBoneVisible		(bone_id));
    }
    bone_id = pWeaponVisual->LL_BoneID(wpn_silencer);
    if (SilencerAttachable())
    {
        if (IsSilencerAttached())
        {
            if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
                pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
        }
        else
        {
            if (pWeaponVisual->LL_GetBoneVisible(bone_id))
                pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
        }
    }
    if (m_eSilencerStatus == ALife::eAddonDisabled && bone_id != BI_NONE && pWeaponVisual->LL_GetBoneVisible(bone_id))
    {
        pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
        //		Log("silencer", pWeaponVisual->LL_GetBoneVisible	(bone_id));
    }

    bone_id = pWeaponVisual->LL_BoneID(wpn_grenade_launcher);
    if (GrenadeLauncherAttachable())
    {
        if (IsGrenadeLauncherAttached())
        {
            if (!pWeaponVisual->LL_GetBoneVisible(bone_id))
                pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
        }
        else
        {
            if (pWeaponVisual->LL_GetBoneVisible(bone_id))
                pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
        }
    }
    if (m_eGrenadeLauncherStatus == ALife::eAddonDisabled && bone_id != BI_NONE &&
        pWeaponVisual->LL_GetBoneVisible(bone_id))
    {
        pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
        //		Log("gl", pWeaponVisual->LL_GetBoneVisible			(bone_id));
    }

    pWeaponVisual->CalculateBones_Invalidate();
    pWeaponVisual->CalculateBones(TRUE);
}

void CWeapon::InitAddons() {}
float CWeapon::CurrentZoomFactor()
{
    return IsScopeAttached() ? m_zoom_params.m_fScopeZoomFactor : m_zoom_params.m_fIronSightZoomFactor;
};
void GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor);
void CWeapon::OnZoomIn()
{
    m_zoom_params.m_bIsZoomModeNow = true;
    if (m_zoom_params.m_bUseDynamicZoom)
        SetZoomFactor(m_fRTZoomFactor);
    else
        m_zoom_params.m_fCurrentZoomFactor = CurrentZoomFactor();

    // Отключаем инерцию (Заменено GetInertionFactor())
    // EnableHudInertion(FALSE);

    if (m_zoom_params.m_bZoomDofEnabled && !IsScopeAttached())
        GamePersistent().SetEffectorDOF(m_zoom_params.m_ZoomDof);

    if (GetHUDmode())
        GamePersistent().SetPickableEffectorDOF(true);

    if (m_zoom_params.m_sUseBinocularVision.size() && IsScopeAttached() && nullptr == m_zoom_params.m_pVision)
        m_zoom_params.m_pVision = new CBinocularsVision(m_zoom_params.m_sUseBinocularVision /*"wpn_binoc"*/);

    if (m_zoom_params.m_sUseZoomPostprocess.size() && IsScopeAttached())
    {
        CActor* pA = smart_cast<CActor*>(H_Parent());
        if (pA)
        {
            if (nullptr == m_zoom_params.m_pNight_vision)
            {
                m_zoom_params.m_pNight_vision =
                    new CNightVisionEffector(m_zoom_params.m_sUseZoomPostprocess /*"device_torch"*/);
            }
        }
    }
}

void CWeapon::OnZoomOut()
{
    m_zoom_params.m_bIsZoomModeNow = false;
    m_fRTZoomFactor = GetZoomFactor(); // store current
    m_zoom_params.m_fCurrentZoomFactor = g_fov;

    // Включаем инерцию (также заменено  GetInertionFactor())
    // EnableHudInertion	(TRUE);

    GamePersistent().RestoreEffectorDOF();

    if (GetHUDmode())
        GamePersistent().SetPickableEffectorDOF(false);

    ResetSubStateTime();

    xr_delete(m_zoom_params.m_pVision);
    if (m_zoom_params.m_pNight_vision)
    {
        m_zoom_params.m_pNight_vision->Stop(100000.0f, false);
        xr_delete(m_zoom_params.m_pNight_vision);
    }
}

CUIWindow* CWeapon::ZoomTexture()
{
    if (UseScopeTexture())
        return m_UIScope;
    else
        return nullptr;
}

void CWeapon::SwitchState(u32 S)
{
    if (OnClient())
        return;

#ifndef MASTER_GOLD
    if (bDebug)
    {
        Msg("---Server is going to send GE_WPN_STATE_CHANGE to [%d], weapon_section[%s], parent[%s]", S,
            cNameSect().c_str(), H_Parent() ? H_Parent()->cName().c_str() : "NULL Parent");
    }
#endif // #ifndef MASTER_GOLD

    SetNextState(S);
    if (CHudItem::object().Local() && !CHudItem::object().getDestroy() && m_pInventory && OnServer())
    {
        // !!! Just single entry for given state !!!
        NET_Packet P;
        CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
        P.w_u8(u8(S));
        P.w_u8(u8(m_sub_state));
        P.w_u8(m_ammoType);
        P.w_u8(u8(iAmmoElapsed & 0xff));
        P.w_u8(m_set_next_ammoType_on_reload);
        CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
    }
}

void CWeapon::OnMagazineEmpty() { VERIFY((u32)iAmmoElapsed == m_magazine.size()); }
void CWeapon::reinit()
{
    CShootingObject::reinit();
    CHudItemObject::reinit();
}

void CWeapon::reload(LPCSTR section)
{
    CShootingObject::reload(section);
    CHudItemObject::reload(section);

    m_can_be_strapped = true;
    m_strapped_mode = false;

    if (pSettings->line_exist(section, "strap_bone0"))
        m_strap_bone0 = pSettings->r_string(section, "strap_bone0");
    else
        m_can_be_strapped = false;

    if (pSettings->line_exist(section, "strap_bone1"))
        m_strap_bone1 = pSettings->r_string(section, "strap_bone1");
    else
        m_can_be_strapped = false;

    if (m_eScopeStatus == ALife::eAddonAttachable)
    {
        m_addon_holder_range_modifier =
            READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_range_modifier", m_holder_range_modifier);
        m_addon_holder_fov_modifier =
            READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_fov_modifier", m_holder_fov_modifier);
    }
    else
    {
        m_addon_holder_range_modifier = m_holder_range_modifier;
        m_addon_holder_fov_modifier = m_holder_fov_modifier;
    }

    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "position");
        ypr = pSettings->r_fvector3(section, "orientation");
        ypr.mul(PI / 180.f);

        m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
        m_Offset.translate_over(pos);
    }

    m_StrapOffset = m_Offset;
    if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation"))
    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "strap_position");
        ypr = pSettings->r_fvector3(section, "strap_orientation");
        ypr.mul(PI / 180.f);

        m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
        m_StrapOffset.translate_over(pos);
    }
    else
        m_can_be_strapped = false;

    m_ef_main_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_main_weapon_type", u32(-1));
    m_ef_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_weapon_type", u32(-1));
}

void CWeapon::create_physic_shell() { CPhysicsShellHolder::create_physic_shell(); }
bool CWeapon::ActivationSpeedOverriden(Fvector& dest, bool clear_override)
{
    if (m_activation_speed_is_overriden)
    {
        if (clear_override)
        {
            m_activation_speed_is_overriden = false;
        }

        dest = m_overriden_activation_speed;
        return true;
    }

    return false;
}

void CWeapon::SetActivationSpeedOverride(Fvector const& speed)
{
    m_overriden_activation_speed = speed;
    m_activation_speed_is_overriden = true;
}

void CWeapon::activate_physic_shell()
{
    UpdateXForm();
    CPhysicsShellHolder::activate_physic_shell();
}

void CWeapon::setup_physic_shell() { CPhysicsShellHolder::setup_physic_shell(); }
int g_iWeaponRemove = 1;

bool CWeapon::NeedToDestroyObject() const
{
    if (GameID() == eGameIDSingle)
        return false;
    if (Remote())
        return false;
    if (H_Parent())
        return false;
    if (g_iWeaponRemove == -1)
        return false;
    if (g_iWeaponRemove == 0)
        return true;
    if (TimePassedAfterIndependant() > m_dwWeaponRemoveTime)
        return true;

    return false;
}

ALife::_TIME_ID CWeapon::TimePassedAfterIndependant() const
{
    if (!H_Parent() && m_dwWeaponIndependencyTime != 0)
        return Level().timeServer() - m_dwWeaponIndependencyTime;
    else
        return 0;
}

bool CWeapon::can_kill() const
{
    if (GetSuitableAmmoTotal(true) || m_ammoTypes.empty())
        return (true);

    return (false);
}

CInventoryItem* CWeapon::can_kill(CInventory* inventory) const
{
    if (GetAmmoElapsed() || m_ammoTypes.empty())
        return (const_cast<CWeapon*>(this));

    TIItemContainer::iterator I = inventory->m_all.begin();
    TIItemContainer::iterator E = inventory->m_all.end();
    for (; I != E; ++I)
    {
        CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(*I);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator i =
            std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return (inventory_item);
    }

    return (nullptr);
}

const CInventoryItem* CWeapon::can_kill(const xr_vector<const CGameObject*>& items) const
{
    if (m_ammoTypes.empty())
        return (this);

    xr_vector<const CGameObject*>::const_iterator I = items.begin();
    xr_vector<const CGameObject*>::const_iterator E = items.end();
    for (; I != E; ++I)
    {
        const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator i =
            std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return (inventory_item);
    }

    return (nullptr);
}

bool CWeapon::ready_to_kill() const
{
    return (
        !IsMisfire() && ((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) && GetAmmoElapsed());
}

void CWeapon::UpdateHudAdditonal(Fmatrix& trans)
{
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (!pActor)
        return;

    if ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) ||
        (!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f))
    {
        u8 idx = GetCurrentHudOffsetIdx();
        //		if(idx==0)					return;

        attachable_hud_item* hi = HudItemData();
        R_ASSERT(hi);
        Fvector curr_offs, curr_rot;
        curr_offs = hi->m_measures.m_hands_offset[0][idx]; // pos,aim
        curr_rot = hi->m_measures.m_hands_offset[1][idx]; // rot,aim
        curr_offs.mul(m_zoom_params.m_fZoomRotationFactor);
        curr_rot.mul(m_zoom_params.m_fZoomRotationFactor);

        Fmatrix hud_rotation;
        hud_rotation.identity();
        hud_rotation.rotateX(curr_rot.x);

        Fmatrix hud_rotation_y;
        hud_rotation_y.identity();
        hud_rotation_y.rotateY(curr_rot.y);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation_y.identity();
        hud_rotation_y.rotateZ(curr_rot.z);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation.translate_over(curr_offs);
        trans.mulB_43(hud_rotation);

        if (pActor->IsZoomAimingMode())
            m_zoom_params.m_fZoomRotationFactor += Device.fTimeDelta / m_zoom_params.m_fZoomRotateTime;
        else
            m_zoom_params.m_fZoomRotationFactor -= Device.fTimeDelta / m_zoom_params.m_fZoomRotateTime;

        clamp(m_zoom_params.m_fZoomRotationFactor, 0.f, 1.f);
    }
}

void CWeapon::SetAmmoElapsed(int ammo_count)
{
    iAmmoElapsed = ammo_count;

    u32 uAmmo = u32(iAmmoElapsed);

    if (uAmmo != m_magazine.size())
    {
        if (uAmmo > m_magazine.size())
        {
            CCartridge l_cartridge;
            l_cartridge.Load(m_ammoTypes[m_ammoType].c_str(), m_ammoType);
            while (uAmmo > m_magazine.size())
                m_magazine.push_back(l_cartridge);
        }
        else
        {
            while (uAmmo < m_magazine.size())
                m_magazine.pop_back();
        };
    };
}

u32 CWeapon::ef_main_weapon_type() const
{
    VERIFY(m_ef_main_weapon_type != u32(-1));
    return (m_ef_main_weapon_type);
}

u32 CWeapon::ef_weapon_type() const
{
    VERIFY(m_ef_weapon_type != u32(-1));
    return (m_ef_weapon_type);
}

bool CWeapon::IsNecessaryItem(const shared_str& item_sect)
{
    return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end());
}

void CWeapon::modify_holder_params(float& range, float& fov) const
{
    if (!IsScopeAttached())
    {
        inherited::modify_holder_params(range, fov);
        return;
    }
    range *= m_addon_holder_range_modifier;
    fov *= m_addon_holder_fov_modifier;
}

bool CWeapon::render_item_ui_query()
{
    bool b_is_active_item = (m_pInventory->ActiveItem() == this);
    bool res = b_is_active_item && IsZoomed() && ZoomHideCrosshair() && ZoomTexture() && !IsRotatingToZoom();
    return res;
}

void CWeapon::render_item_ui()
{
    if (m_zoom_params.m_pVision)
        m_zoom_params.m_pVision->Draw();

    ZoomTexture()->Update();
    ZoomTexture()->Draw();
}

bool CWeapon::unlimited_ammo()
{
    if (IsGameTypeSingle())
    {
        if (m_pInventory)
        {
            return inventory_owner().unlimited_ammo() && m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited);
        }
        else
            return false;
    }

    return ((GameID() == eGameIDDeathmatch) && m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited));
};

float CWeapon::GetMagazineWeight(const decltype(CWeapon::m_magazine)& mag) const
{
    float res = 0;
    const char* last_type = nullptr;
    float last_ammo_weight = 0;
    for (auto& c : mag)
    {
        // Usually ammos in mag have same type, use this fact to improve performance
        if (last_type != c.m_ammoSect.c_str())
        {
            last_type = c.m_ammoSect.c_str();
            last_ammo_weight = c.Weight();
        }
        res += last_ammo_weight;
    }
    return res;
}

float CWeapon::Weight() const
{
    float res = CInventoryItemObject::Weight();
    if (IsGrenadeLauncherAttached() && GetGrenadeLauncherName().size())
    {
        res += pSettings->r_float(GetGrenadeLauncherName(), "inv_weight");
    }
    if (IsScopeAttached() && m_scopes.size())
    {
        res += pSettings->r_float(GetScopeName(), "inv_weight");
    }
    if (IsSilencerAttached() && GetSilencerName().size())
    {
        res += pSettings->r_float(GetSilencerName(), "inv_weight");
    }
    res += GetMagazineWeight(m_magazine);

    return res;
}

bool CWeapon::show_crosshair() { return !IsPending() && (!IsZoomed() || !ZoomHideCrosshair()); }
bool CWeapon::show_indicators() { return !(IsZoomed() && ZoomTexture()); }
float CWeapon::GetConditionToShow() const
{
    return (GetCondition()); // powf(GetCondition(),4.0f));
}

BOOL CWeapon::ParentMayHaveAimBullet()
{
    IGameObject* O = H_Parent();
    CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
    return EA->cast_actor() != nullptr;
}

BOOL CWeapon::ParentIsActor()
{
    IGameObject* O = H_Parent();
    if (!O)
        return FALSE;

    CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
    if (!EA)
        return FALSE;

    return EA->cast_actor() != nullptr;
}

extern u32 hud_adj_mode;

void CWeapon::debug_draw_firedeps()
{
#ifdef DEBUG
    if (hud_adj_mode == 5 || hud_adj_mode == 6 || hud_adj_mode == 7)
    {
        CDebugRenderer& render = Level().debug_renderer();

        if (hud_adj_mode == 5)
            render.draw_aabb(get_LastFP(), 0.005f, 0.005f, 0.005f, color_xrgb(255, 0, 0));

        if (hud_adj_mode == 6)
            render.draw_aabb(get_LastFP2(), 0.005f, 0.005f, 0.005f, color_xrgb(0, 0, 255));

        if (hud_adj_mode == 7)
            render.draw_aabb(get_LastSP(), 0.005f, 0.005f, 0.005f, color_xrgb(0, 255, 0));
    }
#endif // DEBUG
}

const float& CWeapon::hit_probability() const
{
    VERIFY((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster));
    return (m_hit_probability[egdNovice]);
}

void CWeapon::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);
    m_BriefInfo_CalcFrame = 0;

    if (S == eReload)
    {
        CActor* current_actor = smart_cast<CActor*>(H_Parent());
        if (current_actor && H_Parent() == Level().CurrentEntity())
            if (iAmmoElapsed == 0)
                if (!fsimilar(m_zoom_params.m_ReloadEmptyDof.w, -1.0f))
                    current_actor->Cameras().AddCamEffector(new CEffectorDOF(m_zoom_params.m_ReloadEmptyDof));
            else
                if (!fsimilar(m_zoom_params.m_ReloadDof.w, -1.0f))
                    current_actor->Cameras().AddCamEffector(new CEffectorDOF(m_zoom_params.m_ReloadDof));
    }
}

void CWeapon::OnAnimationEnd(u32 state) { inherited::OnAnimationEnd(state); }
u8 CWeapon::GetCurrentHudOffsetIdx()
{
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (!pActor)
        return 0;

    bool b_aiming = ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) ||
        (!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f));

    if (!b_aiming)
        return 0;
    else
        return 1;
}

void CWeapon::render_hud_mode() { RenderLight(); }
bool CWeapon::MovingAnimAllowedNow() { return !IsZoomed(); }
bool CWeapon::IsHudModeNow() { return (HudItemData() != nullptr); }
void CWeapon::ZoomInc()
{
    if (!IsScopeAttached())
        return;
    if (!m_zoom_params.m_bUseDynamicZoom)
        return;
    float delta, min_zoom_factor;
    GetZoomData(m_zoom_params.m_fScopeZoomFactor, delta, min_zoom_factor);

    float f = GetZoomFactor() - delta;
    clamp(f, m_zoom_params.m_fScopeZoomFactor, min_zoom_factor);
    SetZoomFactor(f);
}

void CWeapon::ZoomDec()
{
    if (!IsScopeAttached())
        return;
    if (!m_zoom_params.m_bUseDynamicZoom)
        return;
    float delta, min_zoom_factor;
    GetZoomData(m_zoom_params.m_fScopeZoomFactor, delta, min_zoom_factor);

    float f = GetZoomFactor() + delta;
    clamp(f, m_zoom_params.m_fScopeZoomFactor, min_zoom_factor);
    SetZoomFactor(f);
}
u32 CWeapon::Cost() const
{
    u32 res = CInventoryItem::Cost();
    if (IsGrenadeLauncherAttached() && GetGrenadeLauncherName().size())
    {
        res += pSettings->r_u32(GetGrenadeLauncherName(), "cost");
    }
    if (IsScopeAttached() && m_scopes.size())
    {
        res += pSettings->r_u32(GetScopeName(), "cost");
    }
    if (IsSilencerAttached() && GetSilencerName().size())
    {
        res += pSettings->r_u32(GetSilencerName(), "cost");
    }

    if (iAmmoElapsed)
    {
        float w = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "cost");
        float bs = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "box_size");

        res += iFloor(w * (iAmmoElapsed / bs));
    }
    return res;
}
