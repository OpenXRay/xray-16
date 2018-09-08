#include "StdAfx.h"
#include "Torch.h"
#include "Entity.h"
#include "Actor.h"
#include "xrEngine/LightAnimLibrary.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrServer_Objects_ALife_Items.h"
#include "ai_sounds.h"

#include "Level.h"
#include "Include/xrRender/Kinematics.h"
#include "xrEngine/CameraBase.h"
#include "xrEngine/xr_collide_form.h"
#include "Inventory.h"
#include "game_base_space.h"

#include "UIGameCustom.h"
#include "ActorEffector.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"

static const float TORCH_INERTION_CLAMP = PI_DIV_6;
static const float TORCH_INERTION_SPEED_MAX = 7.5f;
static const float TORCH_INERTION_SPEED_MIN = 0.5f;
static Fvector TORCH_OFFSET = {-0.2f, +0.1f, -0.3f};
static const Fvector OMNI_OFFSET = {-0.2f, +0.1f, -0.1f};
static const float OPTIMIZATION_DISTANCE = 100.f;

static bool stalker_use_dynamic_lights = false;

CTorch::CTorch(void)
{
    light_render = GEnv.Render->light_create();
    light_render->set_type(IRender_Light::SPOT);
    light_render->set_shadow(true);
    light_omni = GEnv.Render->light_create();
    light_omni->set_type(IRender_Light::POINT);
    light_omni->set_shadow(false);

    m_switched_on = false;
    glow_render = GEnv.Render->glow_create();
    lanim = 0;
    fBrightness = 1.f;

    m_prev_hp.set(0, 0);
    m_delta_h = 0;
    m_night_vision = NULL;

    // Disabling shift by x and z axes for 1st render,
    // because we don't have dynamic lighting in it.
    if (GEnv.CurrentRenderer == 1)
    {
        TORCH_OFFSET.x = 0;
        TORCH_OFFSET.z = 0;
    }
}

CTorch::~CTorch()
{
    light_render.destroy();
    light_omni.destroy();
    glow_render.destroy();
    xr_delete(m_night_vision);
}

inline bool CTorch::can_use_dynamic_lights()
{
    if (!H_Parent())
        return (true);

    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(H_Parent());
    if (!owner)
        return (true);

    return (owner->can_use_dynamic_lights());
}

void CTorch::Load(LPCSTR section)
{
    inherited::Load(section);
    light_trace_bone = pSettings->r_string(section, "light_trace_bone");

    m_bNightVisionEnabled = !!pSettings->r_bool(section, "night_vision");
}

void CTorch::SwitchNightVision()
{
    if (OnClient())
        return;
    SwitchNightVision(!m_bNightVisionOn);
}

void CTorch::SwitchNightVision(bool vision_on, bool use_sounds)
{
    if (!m_bNightVisionEnabled)
        return;

    m_bNightVisionOn = vision_on;

    CActor* pA = smart_cast<CActor*>(H_Parent());
    if (!pA)
    {
        return;
    }
    if (!m_night_vision)
        m_night_vision = new CNightVisionEffector(cNameSect());

    LPCSTR disabled_names = pSettings->r_string(cNameSect(), "disabled_maps");
    LPCSTR curr_map = *Level().name();
    u32 cnt = _GetItemCount(disabled_names);
    bool b_allow = true;
    string512 tmp;
    for (u32 i = 0; i < cnt; ++i)
    {
        _GetItem(disabled_names, i, tmp);
        if (0 == xr_stricmp(tmp, curr_map))
        {
            b_allow = false;
            break;
        }
    }

    CHelmet* pHelmet = smart_cast<CHelmet*>(pA->inventory().ItemFromSlot(HELMET_SLOT));
    CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>(pA->inventory().ItemFromSlot(OUTFIT_SLOT));

    if (pHelmet && pHelmet->m_NightVisionSect.size() && !b_allow)
    {
        m_night_vision->OnDisabled(pA, use_sounds);
        return;
    }
    else if (pOutfit && pOutfit->m_NightVisionSect.size() && !b_allow)
    {
        m_night_vision->OnDisabled(pA, use_sounds);
        return;
    }

    bool bIsActiveNow = m_night_vision->IsActive();

    if (m_bNightVisionOn)
    {
        if (!bIsActiveNow)
        {
            if (pHelmet && pHelmet->m_NightVisionSect.size())
            {
                m_night_vision->Start(pHelmet->m_NightVisionSect, pA, use_sounds);
                return;
            }
            else if (pOutfit && pOutfit->m_NightVisionSect.size())
            {
                m_night_vision->Start(pOutfit->m_NightVisionSect, pA, use_sounds);
                return;
            }
            m_bNightVisionOn = false; // in case if there is no nightvision in helmet and outfit
        }
    }
    else
    {
        if (bIsActiveNow)
        {
            m_night_vision->Stop(100000.0f, use_sounds);
        }
    }
}

void CTorch::Switch()
{
    if (OnClient())
        return;
    bool bActive = !m_switched_on;
    Switch(bActive);
}

void CTorch::Switch(bool light_on)
{
    m_switched_on = light_on;
    if (can_use_dynamic_lights())
    {
        light_render->set_active(light_on);

        // CActor *pA = smart_cast<CActor *>(H_Parent());
        // if(!pA)
        light_omni->set_active(light_on);
    }
    glow_render->set_active(light_on);

    if (*light_trace_bone)
    {
        IKinematics* pVisual = smart_cast<IKinematics*>(Visual());
        VERIFY(pVisual);
        u16 bi = pVisual->LL_BoneID(light_trace_bone);

        pVisual->LL_SetBoneVisible(bi, light_on, TRUE);
        pVisual->CalculateBones(TRUE);
    }
}
bool CTorch::torch_active() const { return (m_switched_on); }
BOOL CTorch::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeItemTorch* torch = smart_cast<CSE_ALifeItemTorch*>(e);
    R_ASSERT(torch);
    cNameVisual_set(torch->get_visual());

    R_ASSERT(!GetCForm());
    R_ASSERT(smart_cast<IKinematics*>(Visual()));
    CForm = new CCF_Skeleton(this);

    if (!inherited::net_Spawn(DC))
        return (FALSE);

    bool b_r2 = !!psDeviceFlags.test(rsR2);
    b_r2 |= !!psDeviceFlags.test(rsR3);
    b_r2 |= !!psDeviceFlags.test(rsR4);

    IKinematics* K = smart_cast<IKinematics*>(Visual());
    CInifile* pUserData = K->LL_UserData();
    R_ASSERT3(pUserData, "Empty Torch user data!", torch->get_visual());
    lanim = LALib.FindItem(pUserData->r_string("torch_definition", "color_animator"));
    guid_bone = K->LL_BoneID(pUserData->r_string("torch_definition", "guide_bone"));
    VERIFY(guid_bone != BI_NONE);

    Fcolor clr = pUserData->r_fcolor("torch_definition", (b_r2) ? "color_r2" : "color");
    fBrightness = clr.intensity();
    float range = pUserData->r_float("torch_definition", (b_r2) ? "range_r2" : "range");
    light_render->set_color(clr);
    light_render->set_range(range);

    Fcolor clr_o = pUserData->r_fcolor("torch_definition", (b_r2) ? "omni_color_r2" : "omni_color");
    float range_o = pUserData->r_float("torch_definition", (b_r2) ? "omni_range_r2" : "omni_range");
    light_omni->set_color(clr_o);
    light_omni->set_range(range_o);

    light_render->set_cone(deg2rad(pUserData->r_float("torch_definition", "spot_angle")));
    light_render->set_texture(pUserData->r_string("torch_definition", "spot_texture"));

    glow_render->set_texture(pUserData->r_string("torch_definition", "glow_texture"));
    glow_render->set_color(clr);
    glow_render->set_radius(pUserData->r_float("torch_definition", "glow_radius"));

    //включить/выключить фонарик
    Switch(torch->m_active);
    VERIFY(!torch->m_active || (torch->ID_Parent != 0xffff));

    if (torch->ID_Parent == 0)
        SwitchNightVision(torch->m_nightvision_active, false);
    // else
    //	SwitchNightVision	(false, false);

    m_delta_h = PI_DIV_2 - atan((range * 0.5f) / _abs(TORCH_OFFSET.x));

    return (TRUE);
}

void CTorch::net_Destroy()
{
    Switch(false);
    SwitchNightVision(false);

    inherited::net_Destroy();
}

void CTorch::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    m_focus.set(Position());
}

void CTorch::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    Switch(false);
    SwitchNightVision(false);

    m_sounds.StopAllSounds();
}

void CTorch::UpdateCL()
{
    inherited::UpdateCL();

    if (!m_switched_on)
        return;

    CBoneInstance& BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);
    Fmatrix M;

    if (H_Parent())
    {
        CActor* actor = smart_cast<CActor*>(H_Parent());
        if (actor)
            smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones_Invalidate();

        if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition) < _sqr(OPTIMIZATION_DISTANCE) ||
            GameID() != eGameIDSingle)
        {
            // near camera
            smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones();
            M.mul_43(XFORM(), BI.mTransform);
        }
        else
        {
            // approximately the same
            M = H_Parent()->XFORM();
            H_Parent()->Center(M.c);
            M.c.y += H_Parent()->Radius() * 2.f / 3.f;
        }

        if (actor)
        {
            m_prev_hp.x = angle_inertion_var(m_prev_hp.x, -actor->cam_FirstEye()->yaw, TORCH_INERTION_SPEED_MIN,
                TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
            m_prev_hp.y = angle_inertion_var(m_prev_hp.y, -actor->cam_FirstEye()->pitch, TORCH_INERTION_SPEED_MIN,
                TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);

            Fvector dir, right, up;
            dir.setHP(m_prev_hp.x + m_delta_h, m_prev_hp.y);
            Fvector::generate_orthonormal_basis_normalized(dir, up, right);

            if (true)
            {
                Fvector offset = M.c;
                offset.mad(M.i, TORCH_OFFSET.x);
                offset.mad(M.j, TORCH_OFFSET.y);
                offset.mad(M.k, TORCH_OFFSET.z);
                light_render->set_position(offset);

                if (true /*false*/)
                {
                    offset = M.c;
                    offset.mad(M.i, OMNI_OFFSET.x);
                    offset.mad(M.j, OMNI_OFFSET.y);
                    offset.mad(M.k, OMNI_OFFSET.z);
                    light_omni->set_position(offset);
                }
            } // if (true)
            glow_render->set_position(M.c);

            if (true)
            {
                light_render->set_rotation(dir, right);

                if (true /*false*/)
                {
                    light_omni->set_rotation(dir, right);
                }
            } // if (true)
            glow_render->set_direction(dir);

        } // if(actor)
        else
        {
            if (can_use_dynamic_lights())
            {
                light_render->set_position(M.c);
                light_render->set_rotation(M.k, M.i);

                Fvector offset = M.c;
                offset.mad(M.i, OMNI_OFFSET.x);
                offset.mad(M.j, OMNI_OFFSET.y);
                offset.mad(M.k, OMNI_OFFSET.z);
                light_omni->set_position(M.c);
                light_omni->set_rotation(M.k, M.i);
            } // if (can_use_dynamic_lights())

            glow_render->set_position(M.c);
            glow_render->set_direction(M.k);
        }
    } // if(HParent())
    else
    {
        if (getVisible() && m_pPhysicsShell)
        {
            M.mul(XFORM(), BI.mTransform);

            m_switched_on = false;
            light_render->set_active(false);
            light_omni->set_active(false);
            glow_render->set_active(false);
        } // if (getVisible() && m_pPhysicsShell)
    }

    if (!m_switched_on)
        return;

    // calc color animator
    if (!lanim)
        return;

    int frame;
    // возвращает в формате BGR
    u32 clr = lanim->CalculateBGR(Device.fTimeGlobal, frame);

    Fcolor fclr;
    fclr.set((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);
    fclr.mul_rgb(fBrightness / 255.f);
    if (can_use_dynamic_lights())
    {
        light_render->set_color(fclr);
        light_omni->set_color(fclr);
    }
    glow_render->set_color(fclr);
}

void CTorch::create_physic_shell() { CPhysicsShellHolder::create_physic_shell(); }
void CTorch::activate_physic_shell() { CPhysicsShellHolder::activate_physic_shell(); }
void CTorch::setup_physic_shell() { CPhysicsShellHolder::setup_physic_shell(); }
void CTorch::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);
    //	P.w_u8						(m_switched_on ? 1 : 0);

    BYTE F = 0;
    F |= (m_switched_on ? eTorchActive : 0);
    F |= (m_bNightVisionOn ? eNightVisionActive : 0);
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    if (pA)
    {
        if (pA->attached(this))
            F |= eAttached;
    }
    P.w_u8(F);
    //	Msg("CTorch::net_export - NV[%d]", m_bNightVisionOn);
}

void CTorch::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);

    BYTE F = P.r_u8();
    bool new_m_switched_on = !!(F & eTorchActive);
    bool new_m_bNightVisionOn = !!(F & eNightVisionActive);

    if (new_m_switched_on != m_switched_on)
        Switch(new_m_switched_on);
    if (new_m_bNightVisionOn != m_bNightVisionOn)
    {
        //		Msg("CTorch::net_Import - NV[%d]", new_m_bNightVisionOn);

        const CActor* pA = smart_cast<const CActor*>(H_Parent());
        if (pA)
        {
            SwitchNightVision(new_m_bNightVisionOn);
        }
    }
}
bool CTorch::can_be_attached() const
{
    const CActor* pA = smart_cast<const CActor*>(H_Parent());
    if (pA)
        return pA->inventory().InSlot(this);
    else
        return true;
}

void CTorch::afterDetach()
{
    inherited::afterDetach();
    Switch(false);
}
void CTorch::renderable_Render() { inherited::renderable_Render(); }
void CTorch::enable(bool value)
{
    inherited::enable(value);

    if (!enabled() && m_switched_on)
        Switch(false);
}

CNightVisionEffector::CNightVisionEffector(const shared_str& section) : m_pActor(NULL)
{
    m_sounds.LoadSound(section.c_str(), "snd_night_vision_on", "NightVisionOnSnd", false, SOUND_TYPE_ITEM_USING);
    m_sounds.LoadSound(section.c_str(), "snd_night_vision_off", "NightVisionOffSnd", false, SOUND_TYPE_ITEM_USING);
    m_sounds.LoadSound(section.c_str(), "snd_night_vision_idle", "NightVisionIdleSnd", false, SOUND_TYPE_ITEM_USING);
    m_sounds.LoadSound(
        section.c_str(), "snd_night_vision_broken", "NightVisionBrokenSnd", false, SOUND_TYPE_ITEM_USING);
}

void CNightVisionEffector::Start(const shared_str& sect, CActor* pA, bool play_sound)
{
    m_pActor = pA;
    AddEffector(m_pActor, effNightvision, sect);
    if (play_sound)
    {
        PlaySounds(eStartSound);
        PlaySounds(eIdleSound);
    }
}

void CNightVisionEffector::Stop(const float factor, bool play_sound)
{
    if (!m_pActor)
        return;
    CEffectorPP* pp = m_pActor->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
    if (pp)
    {
        pp->Stop(factor);
        if (play_sound)
            PlaySounds(eStopSound);

        m_sounds.StopSound("NightVisionIdleSnd");
    }
}

bool CNightVisionEffector::IsActive()
{
    if (!m_pActor)
        return false;
    CEffectorPP* pp = m_pActor->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
    return (pp != NULL);
}

void CNightVisionEffector::OnDisabled(CActor* pA, bool play_sound)
{
    m_pActor = pA;
    if (play_sound)
        PlaySounds(eBrokeSound);
}

void CNightVisionEffector::PlaySounds(EPlaySounds which)
{
    if (!m_pActor)
        return;

    bool bPlaySoundFirstPerson = !!m_pActor->HUDview();
    switch (which)
    {
    case eStartSound: { m_sounds.PlaySound("NightVisionOnSnd", m_pActor->Position(), NULL, bPlaySoundFirstPerson);
    }
    break;
    case eStopSound: { m_sounds.PlaySound("NightVisionOffSnd", m_pActor->Position(), NULL, bPlaySoundFirstPerson);
    }
    break;
    case eIdleSound:
    {
        m_sounds.PlaySound("NightVisionIdleSnd", m_pActor->Position(), NULL, bPlaySoundFirstPerson, true);
    }
    break;
    case eBrokeSound: { m_sounds.PlaySound("NightVisionBrokenSnd", m_pActor->Position(), NULL, bPlaySoundFirstPerson);
    }
    break;
    default: NODEFAULT;
    }
}
