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

constexpr pcstr TORCH_DEFINITION = "torch_definition";
static const float TORCH_INERTION_CLAMP = PI_DIV_6;
static const float TORCH_INERTION_SPEED_MAX = 7.5f;
static const float TORCH_INERTION_SPEED_MIN = 0.5f;
static Fvector TORCH_OFFSET = {-0.2f, +0.1f, -0.3f};
static const Fvector OMNI_OFFSET = {-0.2f, +0.1f, -0.1f};
static const float OPTIMIZATION_DISTANCE = 100.f;

// Tuning knobs for the Dead Air torch2 (headlamp boost) — see CTorch::UpdateCL. Bump these to
// make the boosted beam brighter/longer-reaching; kept as named constants so it's a one-line
// change instead of hunting through the update loop.
// NOTE: these were escalated to 60/6 by an earlier session while chasing what looked like an
// impossibly dim headlamp. That dimness was really the spot-cookie bug (the beam multiplied out to
// exactly zero, so no gain could ever make it visible -- see net_Spawn's set_texture comment). With
// that fixed, a 60x gain would be blinding, so these are back to 1.0 = "use Dead Air's own values":
// color (0.6 base + ~1.0 from script) = ~1.6, range 24. Raise only for taste.
static const float TORCH2_BOOST_GAIN = 1.0f;
static const float TORCH2_RANGE_GAIN = 1.0f;

CTorch::CTorch()
    : fBrightness(1.f), lanim(nullptr), guid_bone(BI_NONE),
      m_delta_h(0), m_switched_on(false),
      light_render(GEnv.Render->light_create()),
      light_omni(GEnv.Render->light_create()),
      light_torch2(GEnv.Render->light_create()),
      glow_render(GEnv.Render->glow_create()),
      m_bNightVisionEnabled(false), m_bNightVisionOn(false), m_night_vision(nullptr)
{
    m_prev_hp.set(0, 0);

    light_render->set_type(IRender_Light::SPOT);
    // Shadow-casting is disabled here as a deliberate workaround, not stylistic: this engine's shadow-map
    // path only works correctly for lights whose position is set once (static lamps). A light attached to a
    // moving bone re-renders its shadow map every frame, and something in that path silently breaks the
    // accum_spot stencil-marking technique for it -- confirmed via a RenderDoc capture of the light
    // accumulation buffer (r2_RT_accum): with shadows on, this light contributed exactly (0,0,0,0) to a
    // 500x500px region of static wall geometry it was visibly aimed at, while a static lamp correctly lit
    // its surroundings in the same buffer. With shadows off, the same light's contribution to that buffer
    // jumped to being the dominant light source on screen. See the DAR3 project memory
    // ("dar3-dynamic-lights-dont-light-environment") for the full investigation.
    light_render->set_shadow(false);
    light_omni->set_type(IRender_Light::POINT);
    light_omni->set_shadow(false);
    // Same shadow caveat as light_render above: this beam is bone-attached and moves every frame.
    light_torch2->set_type(IRender_Light::SPOT);
    light_torch2->set_shadow(false);

    // Disabling shift by x and z axes for 1st render,
    // because we don't have dynamic lighting in it.
    if (GEnv.Render->GenerationIsR1())
    {
        TORCH_OFFSET.x = 0;
        TORCH_OFFSET.z = 0;
    }
}

CTorch::~CTorch()
{
    light_render.destroy();
    light_omni.destroy();
    light_torch2.destroy();
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

    if (pSettings->line_exist(section, "snd_turn_on"))
        m_sounds.LoadSound(section, "snd_turn_on", "sndTurnOn", false, SOUND_TYPE_ITEM_USING);
    if (pSettings->line_exist(section, "snd_turn_off"))
        m_sounds.LoadSound(section, "snd_turn_off", "sndTurnOff", false, SOUND_TYPE_ITEM_USING);
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

    const bool was_on = m_bNightVisionOn;
    m_bNightVisionOn = vision_on;

    CActor* pA = smart_cast<CActor*>(H_Parent());
    if (!pA)
        return; // NPC torches: no post-process to drive

    // [PPDBG-NV] one line per actual toggle of the player's night vision.
    Msg("[PPDBG-NV] SwitchNightVision(on=%d snd=%d) f=%u wasOn=%d", vision_on ? 1 : 0, use_sounds ? 1 : 0,
        Device.dwFrame, was_on ? 1 : 0);
    if (!m_night_vision)
        m_night_vision = xr_new<CNightVisionEffector>(cNameSect());

    LPCSTR disabled_names = pSettings->r_string(cNameSect(), "disabled_maps");
    pcstr curr_map = Level().name().c_str();
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

    Msg("[PPDBG-NV]   map='%s' allowed=%d helmet='%s' outfit='%s' activeNow=%d", curr_map, b_allow ? 1 : 0,
        pHelmet ? pHelmet->m_NightVisionSect.c_str() : "<no helmet>",
        pOutfit ? pOutfit->m_NightVisionSect.c_str() : "<no outfit>", bIsActiveNow ? 1 : 0);

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
            Msg("[PPDBG-NV]   bail: neither helmet nor outfit has nightvision_sect");
            m_bNightVisionOn = false; // in case if there is no nightvision in helmet and outfit
        }
    }
    else
    {
        if (bIsActiveNow)
        {
            Msg("[PPDBG-NV]   -> Stop()");
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
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (pActor)
    {
        if (light_on && !m_switched_on)
        {
            if (m_sounds.FindSoundItem("SndTurnOn", false))
                m_sounds.PlaySound("SndTurnOn", pActor->Position(), NULL, !!pActor->HUDview());
        }
        else if (!light_on && m_switched_on)
        {
            if (m_sounds.FindSoundItem("SndTurnOff", false))
                m_sounds.PlaySound("SndTurnOff", pActor->Position(), NULL, !!pActor->HUDview());
        }
    }

    if (pActor && m_switched_on != light_on)
        Msg("[PPDBG-TORCHSWITCH] actor torch %d -> %d (cust1=%d cust2=%d en2=%d enabled=%d) frame=%u",
            m_switched_on, light_on, m_torch_customized, m_torch2_customized, m_torch2_enabled, enabled(),
            Device.dwFrame);

    m_switched_on = light_on;
    if (can_use_dynamic_lights())
    {
        light_render->set_active(light_on);

        // CActor *pA = smart_cast<CActor *>(H_Parent());
        // if(!pA)
        light_omni->set_active(light_on);
    }
    // The headlamp beam's activation is owned by UpdateCL (it follows torch2's enable flag, not the
    // object's master switch), so only ever turn it OFF from here -- switching it on here would flash
    // the beam for a frame even with the headlamp disabled.
    if (!light_on)
        light_torch2->set_active(false);
    glow_render->set_active(light_on);

    if (light_trace_bone.c_str())
    {
        IKinematics* pVisual = smart_cast<IKinematics*>(Visual());
        VERIFY(pVisual);
        u16 bi = pVisual->LL_BoneID(light_trace_bone);

        pVisual->LL_SetBoneVisible(bi, light_on, TRUE);
        pVisual->CalculateBones(TRUE);
    }
}
bool CTorch::torch_active() const { return (m_switched_on); }
bool CTorch::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeItemTorch* torch = smart_cast<CSE_ALifeItemTorch*>(e);
    R_ASSERT(torch);
    cNameVisual_set(torch->get_visual());

    R_ASSERT(!GetCForm());
    R_ASSERT(smart_cast<IKinematics*>(Visual()));
    CForm = xr_new<CCF_Skeleton>(this);

    if (!inherited::net_Spawn(DC))
        return (FALSE);

    bool b_r2 = GEnv.Render->GenerationIsR2OrHigher();

    IKinematics* K = smart_cast<IKinematics*>(Visual());
    CInifile* pUserData = K->LL_UserData();
    R_ASSERT3(pUserData, "Empty Torch user data!", torch->get_visual());
    // "empty" is the documented sentinel for "no color animator" (see the torch_definition comment in
    // light_night.ltx). It must NOT be looked up as a real animator: this game's lanims library contains an
    // actual item named "empty" whose output is black, and CTorch::UpdateCL applies the animator's color to
    // light_render every frame (for the default, non-torch1-customized headlamp), which drove the spot beam's
    // color to (0,0,0) -- the beam then lit nothing, and only the omni point light (~1.3m) remained visible,
    // i.e. "the headlamp only lights a few steps." Honor the sentinel so the beam keeps its config color.
    pcstr color_anim_name = pUserData->r_string(TORCH_DEFINITION, "color_animator");
    lanim = (color_anim_name && color_anim_name[0] && 0 != xr_stricmp(color_anim_name, "empty"))
        ? LALib.FindItem(color_anim_name)
        : nullptr;
    Msg("[PPDBG-TORCHSPAWN] color_animator='%s' -> lanim=%s visual='%s'", color_anim_name,
        lanim ? "NON-NULL(animated)" : "null(static)", torch->get_visual());
    guid_bone = K->LL_BoneID(pUserData->r_string(TORCH_DEFINITION, "guide_bone"));
    VERIFY(guid_bone != BI_NONE);

    Fcolor clr = pUserData->r_fcolor(TORCH_DEFINITION, (b_r2) ? "color_r2" : "color");
    fBrightness = clr.intensity();
    float range = pUserData->r_float(TORCH_DEFINITION, (b_r2) ? "range_r2" : "range");
    light_render->set_color(clr);
    light_render->set_range(range);

    if (b_r2)
    {
        bool useVolumetric = pUserData->read_if_exists<bool>(TORCH_DEFINITION, "volumetric_enabled", false);
        light_render->set_volumetric(useVolumetric);
        if (useVolumetric)
        {
            float volQuality = pUserData->read_if_exists<float>(TORCH_DEFINITION, "volumetric_quality", 1.f);
            clamp(volQuality, 0.f, 1.f);
            light_render->set_volumetric_quality(volQuality);

            float volIntensity = pUserData->read_if_exists<float>(TORCH_DEFINITION, "volumetric_intensity", 1.f);
            clamp(volIntensity, 0.f, 10.f);
            light_render->set_volumetric_intensity(volIntensity);

            float volDistance = pUserData->read_if_exists<float>(TORCH_DEFINITION, "volumetric_distance", 1.f);
            clamp(volDistance, 0.f, 1.f);
            light_render->set_volumetric_distance(volDistance);
        }
    }

    Fcolor clr_o = pUserData->r_fcolor(TORCH_DEFINITION, (b_r2) ? "omni_color_r2" : "omni_color");
    float range_o = pUserData->r_float(TORCH_DEFINITION, (b_r2) ? "omni_range_r2" : "omni_range");
    light_omni->set_color(clr_o);
    light_omni->set_range(range_o);

    m_torch_base_cone = deg2rad(pUserData->r_float(TORCH_DEFINITION, "spot_angle"));
    light_render->set_cone(m_torch_base_cone);
    // NOTE: the configured spot cookie (`spot_texture`, e.g. "internal\internal_light_torch_r2") is
    // deliberately NOT applied. This engine's accum_spot pixel shader ends with
    //     output = cookie.rgb * (diffuse * attenuation) * Ldynamic_color.rgb
    // (verified by disassembling the compiled accum_spot_unshadowed shader), i.e. it shapes the beam using
    // the cookie's RGB. Dead Air's torch cookie carries its shape in ALPHA with RGB left black, so with it
    // bound every torch beam multiplied out to exactly zero -- the light was accumulated every frame but
    // contributed no illumination at any range or brightness, which is why raising color/range never helped.
    // The player's headlamp and every NPC torch were dark; only the small omni point light below was visible
    // ("the headlamp only lights a few steps"). Passing no texture makes accum_spot fall back to the engine's
    // default spot shader, which produces a correct cone. The beam's width still comes from `spot_angle`.
    // See project memory "dar3-dynamic-lights-dont-light-environment".
    light_render->set_texture(nullptr);

    // Headlamp beam: same cookie-less spot as light_render (see the note above), seeded with the visual's
    // baked look. Its live colour/range come from the torch2 script API every frame in UpdateCL; the cone
    // deliberately keeps the baked spot_angle rather than the script's torch2_set_radius(90), because 90deg
    // reads as a flood rather than a headlamp -- change the set_cone call in UpdateCL if you want it wider.
    light_torch2->set_color(clr);
    light_torch2->set_range(range);
    light_torch2->set_cone(m_torch_base_cone);
    light_torch2->set_texture(nullptr);

    glow_render->set_texture(pUserData->r_string(TORCH_DEFINITION, "glow_texture"));
    glow_render->set_color(clr);
    glow_render->set_radius(pUserData->r_float(TORCH_DEFINITION, "glow_radius"));

    // Cache the baked-in main-beam look so torch2's boost can be added on top of it (and
    // reverted cleanly when disabled). The cone (beam width) is never touched by torch2 — it
    // always keeps this visual-defined spot shape, so boosting never turns the beam into a
    // flood light.
    m_torch_base_range = range;
    m_torch_base_cr = clr.r;
    m_torch_base_cg = clr.g;
    m_torch_base_cb = clr.b;

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

    // Dead Air keeps the actor's shared handheld light PERMANENTLY switched on: the headlamp, the hand
    // flashlight, every glowstick colour and the lighter all multiplex onto this one object (selected by
    // itms_manager.TorchType), and the mod expresses "nothing is lit right now" through torch1's range
    // being 0, not by switching the object off. Its own itms_manager.actor_on_update re-asserts
    // `enable_torch(true)` every frame for exactly that reason. Re-assert it engine-side too, so that a
    // stock engine-side toggle can never leave the object hard-off -- when it did, UpdateCL early-returned
    // here and took the glowstick/lighter down with the headlamp, which is why a glowstick only lit
    // anything while the headlamp happened to be switched on.
    // NOTE: deliberately NOT gated on CAttachableItem::enabled(). For the actor's own torch that flag stays
    // false for the whole game (CActor::can_attach explicitly skips the enabled() test that
    // CAttachmentOwner::can_attach applies, so the torch is attached and lit while never being "enabled"),
    // and gating on it made this re-assert dead code.
    if (m_torch_customized && !m_switched_on && smart_cast<CActor*>(H_Parent()))
    {
        Msg("[PPDBG-TORCHFORCE] re-asserting actor torch ON (cust1=%d cust2=%d en2=%d enabled=%d) frame=%u",
            m_torch_customized, m_torch2_customized, m_torch2_enabled, enabled(), Device.dwFrame);
        // Setting the flag first suppresses Switch()'s turn-on sound: this is the engine re-asserting a
        // state the mod considers permanent, not the player flicking a switch, so it must stay silent.
        m_switched_on = true;
        Switch(true);
    }

    // [PPDBG] actor-torch state dump. Deliberately ABOVE the m_switched_on guard: the failing case for the
    // glowstick was precisely m_switched_on == false, which the old placement could never observe.
    if (smart_cast<CActor*>(H_Parent()))
    {
        static u32 s_ppdbg_state_lf = 0;
        if (Device.dwFrame - s_ppdbg_state_lf > 30)
        {
            s_ppdbg_state_lf = Device.dwFrame;
            Msg("[PPDBG-TORCHSTATE] on=%d en=%d cust1=%d spot=%d c1(%.2f,%.2f,%.2f) rng1=%.1f | cust2=%d en2=%d "
                "2c(%.2f,%.2f,%.2f) rng2=%.1f | base_c(%.2f,%.2f,%.2f) base_rng=%.1f lanim=%d lr_active=%d "
                "t2_active=%d",
                m_switched_on, enabled(), m_torch_customized, m_torch_spot, m_torch_cr, m_torch_cg, m_torch_cb,
                m_torch_range, m_torch2_customized, m_torch2_enabled, m_torch2_cr, m_torch2_cg, m_torch2_cb,
                m_torch2_range, m_torch_base_cr, m_torch_base_cg, m_torch_base_cb, m_torch_base_range,
                lanim ? 1 : 0, light_render->get_active(), light_torch2->get_active());
        }
    }

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
                // The headlamp rides the same head transform as the main beam; it only differs in whether
                // it is active and in the colour/range torch2 supplies.
                light_torch2->set_position(offset);

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
                light_torch2->set_rotation(dir, right);

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

    // Dead Air torch1 (main beam) overrides — only diverge from stock config-driven behavior
    // once a script has actually called a torch_set_* setter.
    if (m_torch_customized)
    {
        if (can_use_dynamic_lights())
        {
            light_render->set_type(m_torch_spot ? IRender_Light::SPOT : IRender_Light::POINT);

            // Dead Air's default headlamp (TorchType 0, the `else` branch in xr_actor.script:UpdateTorch)
            // deliberately zeroes torch1 -- `torch_set_color_r/g/b(0)` + `torch_set_range(0)` -- because in
            // Dead Air the actual headlamp beam is torch2 (the flashlight-key toggle, enable_torch2). But a
            // zero-color spot is an invisible light: with torch2 not enabled, the beam vanished and only the
            // ~1.3m omni point light remained ("headlamp only lights a few steps"). TorchTypes that don't set
            // a color (handheld flashlight 1/2) also inherit that stale zero and go black too. So: whenever
            // torch1's color is all-zero, substitute the baked base-beam color so the light is actually
            // visible. (The headlamp itself no longer rides this light at all -- it has its own below.)
            const bool torch1_black = (m_torch_cr <= 0.f) && (m_torch_cg <= 0.f) && (m_torch_cb <= 0.f);
            if (torch1_black)
                light_render->set_color(Fcolor(m_torch_base_cr, m_torch_base_cg, m_torch_base_cb, 1.f));
            else
                light_render->set_color(Fcolor(m_torch_cr, m_torch_cg, m_torch_cb, m_torch_ca));

            // A range of 0 is NOT "no override" -- in Dead Air it is how a mode says "emit nothing"
            // (TorchType 0, the default headlamp, pairs torch_set_range(0) with the real beam living on
            // torch2). It is honoured by the "emitting" gate further down, which deactivates the render
            // lights instead; substituting the baked base range here, as this used to, made the headlamp
            // beam permanently on and was only masked by the object also being hard-switched off.
            if (m_torch_range > 0.f)
                light_render->set_range(m_torch_range);
            // A radius of 0 means "this mode doesn't override the cone" -- restore the baked spot_angle
            // rather than inheriting the previous mode's cone, so each light source keeps its own beam
            // width (e.g. returning to the headlamp after a 60deg glowstick gives back its 35deg beam).
            if (m_torch_radius > 0.f)
                light_render->set_cone(deg2rad(m_torch_radius));
            else
                light_render->set_cone(m_torch_base_cone);
            // Script-supplied cookies are skipped for the same reason as the config one in net_Spawn: this
            // engine's accum_spot shades the beam with the cookie's RGB, and Dead Air's torch cookies are
            // alpha-shaped with black RGB, which zeroes the whole beam. Keep the engine default spot shader.
        }
    }

    // Dead Air torch2 = the headlamp beam (the flashlight key, enable_torch2). It gets its OWN light here
    // rather than being folded into light_render, so the headlamp is independent of whatever handheld source
    // TorchType has selected: the mod multiplexes headlamp/flashlight/glowstick/lighter onto light_render, so
    // any boost applied there either clobbers the handheld mode or has to be suppressed by it. With a separate
    // beam, a glowstick in hand and the headlamp on simply both light the world.
    //
    // Colour is ADDED on top of the baked-in beam colour rather than replacing it: Dead Air scales torch2's
    // colour by battery charge (clamped down to 20%), so a drained battery can yield a colour dimmer than the
    // baked beam; adding guarantees "on" is never dimmer than the visual's intended look while a low battery
    // still gives a proportionally weaker beam. Range likewise takes the larger of the two.
    const bool torch2_on = m_torch2_customized && m_torch2_enabled;
    if (can_use_dynamic_lights())
    {
        if (torch2_on)
        {
            light_torch2->set_color(Fcolor((m_torch_base_cr + m_torch2_cr) * TORCH2_BOOST_GAIN,
                (m_torch_base_cg + m_torch2_cg) * TORCH2_BOOST_GAIN,
                (m_torch_base_cb + m_torch2_cb) * TORCH2_BOOST_GAIN, 1.f));
            light_torch2->set_range(_max(m_torch2_range, m_torch_base_range) * TORCH2_RANGE_GAIN);
            // Keep the visual's baked cone. The script asks for torch2_set_radius(90), which reads as a
            // flood rather than a headlamp; swap in deg2rad(m_torch2_radius) here if you want it wider.
            light_torch2->set_cone(m_torch_base_cone);
        }
        light_torch2->set_active(torch2_on);
    }

    // Which handheld mode is currently emitting. Dead Air never switches the shared light object off --
    // it says "off" by leaving torch1 with range 0 -- so the object stays on (see the force-on at the top of
    // this function) and the actual render lights are gated here instead. This is what lets a glowstick
    // (TorchType 3-10: its own colour and range 8) light the world on its own. The omni fill light and the
    // glow sprite belong to "some handheld light is out" in general, so they follow either source.
    if (m_torch_customized)
    {
        const bool torch1_emitting = (m_torch_range > 0.f);
        if (can_use_dynamic_lights())
        {
            light_render->set_active(torch1_emitting);
            light_omni->set_active(torch1_emitting || torch2_on);
        }
        glow_render->set_active(torch1_emitting || torch2_on);
    }

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
        if (!m_torch_customized)
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

    u8 F = 0;
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

    u8 F = P.r_u8();
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
    m_sounds.LoadSound(section.c_str(), "snd_night_vision_idle", "NightVisionIdleSnd", true, SOUND_TYPE_ITEM_USING);
    m_sounds.LoadSound(
        section.c_str(), "snd_night_vision_broken", "NightVisionBrokenSnd", false, SOUND_TYPE_ITEM_USING);
}

void CNightVisionEffector::Start(const shared_str& sect, CActor* pA, bool play_sound)
{
    m_pActor = pA;
    AddEffector(m_pActor, effNightvision, sect);
    // [PPDBG-NV] did the pp effector actually get created and registered?
    Msg("[PPDBG-NV]   Start(sect='%s') pp_eff_name=%s -> effector%s registered", sect.c_str(),
        pSettings->line_exist(sect, "pp_eff_name") ? pSettings->r_string(sect, "pp_eff_name") : "<MISSING>",
        m_pActor->Cameras().GetPPEffector((EEffectorPPType)effNightvision) ? "" : " NOT");
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

        m_sounds.StopSound("NightVisionOnSnd");
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
