#pragma once

#include "inventory_item_object.h"
#include "HudSound.h"

class CLAItem;
class CNightVisionEffector;

class CTorch : public CInventoryItemObject
{
private:
    typedef CInventoryItemObject inherited;

protected:
    float fBrightness;
    CLAItem* lanim;

    u16 guid_bone;
    shared_str light_trace_bone;

    float m_delta_h;
    Fvector2 m_prev_hp;
    bool m_switched_on;
    ref_light light_render;
    ref_light light_omni;
    // Dedicated headlamp (torch2) beam. Dead Air multiplexes every handheld light source onto the single
    // light_render above, which means an equipped glowstick/lighter/flashlight replaces the headlamp beam
    // instead of coexisting with it. This second spot light gives torch2 its own independent beam so the
    // headlamp works regardless of what is in the player's hand. Actor-only: it is never activated unless
    // a script has driven the torch2 API, which only xr_actor.script does, and only for the player.
    ref_light light_torch2;
    ref_glow glow_render;
    Fvector m_focus;

private:
    inline bool can_use_dynamic_lights();

public:
    CTorch();
    virtual ~CTorch();

    virtual void Load(LPCSTR section);
    virtual bool net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P); // export to server
    virtual void net_Import(NET_Packet& P); // import from server

    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);

    virtual void UpdateCL();

    void Switch();
    void Switch(bool light_on);
    bool torch_active() const;

    // Dead Air dynamic-torch script controls: torch1 (main beam) + torch2 (secondary/boost,
    // driven separately so the two no longer clobber each other's stored values). Applied to the
    // real light_render/glow_render each frame in UpdateCL once a script has actually driven them
    // ("customized"); unscripted torches keep the stock config-driven look. torch2 only boosts
    // light_render's color/range — it never touches the beam's cone (width) or light_omni, so
    // enabling it can't turn the focused spot into an omnidirectional flood.
    float m_torch_cr = 1.f, m_torch_cg = 1.f, m_torch_cb = 1.f, m_torch_ca = 1.f;
    float m_torch_oy = 0.f, m_torch_oz = 0.f;
    float m_torch_radius = 0.f, m_torch_range = 0.f, m_torch_inertion = 0.f;
    bool m_torch_spot = true;
    bool m_torch_customized = false;
    shared_str m_torch_anim, m_torch_texture;

    float m_torch2_cr = 1.f, m_torch2_cg = 1.f, m_torch2_cb = 1.f;
    float m_torch2_ox = 0.f, m_torch2_oy = 0.f;
    float m_torch2_radius = 0.f, m_torch2_range = 0.f;
    bool m_torch2_enabled = false;
    bool m_torch2_customized = false;

    // Baked-in main-beam look captured at spawn. torch2's boost is added on top of these (never
    // replaces them), so a partially-drained battery can only ever brighten the idle beam, not
    // dim it below its resting look.
    float m_torch_base_range = 0.f;
    float m_torch_base_cr = 1.f, m_torch_base_cg = 1.f, m_torch_base_cb = 1.f;
    // Baked cone half-angle (radians) from the visual's `spot_angle`. Dead Air's per-mode handler passes
    // torch_set_radius(0) for the default headlamp meaning "no override"; without remembering the baked
    // value we'd silently keep the previous mode's cone (e.g. a glowstick's 60deg), so switching back from
    // a glowstick left the headlamp beam too wide.
    float m_torch_base_cone = 0.f;

    void torch_set_color_r(float v) { m_torch_cr = v; m_torch_customized = true; }
    void torch_set_color_g(float v) { m_torch_cg = v; m_torch_customized = true; }
    void torch_set_color_b(float v) { m_torch_cb = v; m_torch_customized = true; }
    void torch_set_color_a(float v) { m_torch_ca = v; m_torch_customized = true; }
    void torch2_set_color_r(float v) { m_torch2_cr = v; m_torch2_customized = true; }
    void torch2_set_color_g(float v) { m_torch2_cg = v; m_torch2_customized = true; }
    void torch2_set_color_b(float v) { m_torch2_cb = v; m_torch2_customized = true; }
    void torch_set_offset_y(float v) { m_torch_oy = v; m_torch_customized = true; }
    void torch_set_offset_z(float v) { m_torch_oz = v; m_torch_customized = true; }
    void torch2_set_offset_x(float v) { m_torch2_ox = v; m_torch2_customized = true; }
    void torch2_set_offset_y(float v) { m_torch2_oy = v; m_torch2_customized = true; }
    void torch_set_radius(float v) { m_torch_radius = v; m_torch_customized = true; }
    void torch_set_range(float v) { m_torch_range = v; m_torch_customized = true; }
    void torch2_set_radius(float v) { m_torch2_radius = v; m_torch2_customized = true; }
    void torch2_set_range(float v) { m_torch2_range = v; m_torch2_customized = true; }
    void torch_set_inertion(float v) { m_torch_inertion = v; }
    void torch_set_animation(pcstr v) { m_torch_anim = v; }
    void torch_set_texture(pcstr v) { m_torch_texture = v; m_torch_customized = true; }
    void torch_switch_spot(bool v) { m_torch_spot = v; m_torch_customized = true; }
    void enable_torch(bool v) { Switch(v); }
    void enable_torch2(bool v) { m_torch2_enabled = v; }
    bool torch_enabled() const { return torch_active(); }

    // True once Dead Air's Lua torch API has taken this torch over. Dead Air multiplexes EVERY
    // handheld light source (headlamp, hand flashlight, glowstick, lighter) onto this single object
    // and owns its on/off state from script, so the engine must stop applying its own stock
    // semantics to it -- see CActor::SwitchTorch and the "emitting" gate in CTorch::UpdateCL.
    bool script_driven() const { return m_torch_customized || m_torch2_customized; }

    virtual bool can_be_attached() const;

    // CAttachableItem
    virtual void enable(bool value);

public:
    void SwitchNightVision();
    void SwitchNightVision(bool light_on, bool use_sounds = true);

    bool GetNightVisionStatus() { return m_bNightVisionOn; }
    CNightVisionEffector* GetNightVision() { return m_night_vision; }

    // True once a script has toggled night vision itself (Lua enable_night_vision). Dead Air handles
    // the kNIGHT_VISION key in itms_manager.on_key_press, so the engine's own binding must not toggle
    // as well -- see CActor::SwitchNightVision.
    void set_night_vision_script_driven() { m_night_vision_script_driven = true; }
    bool night_vision_script_driven() const { return m_night_vision_script_driven; }

protected:
    bool m_bNightVisionEnabled;
    bool m_bNightVisionOn;
    bool m_night_vision_script_driven = false;

    CNightVisionEffector* m_night_vision;
    HUD_SOUND_COLLECTION m_sounds;

    enum EStats
    {
        eTorchActive = (1 << 0),
        eNightVisionActive = (1 << 1),
        eAttached = (1 << 2)
    };

public:
    virtual bool use_parent_ai_locations() const { return (!H_Parent()); }
    virtual void create_physic_shell();
    virtual void activate_physic_shell();
    virtual void setup_physic_shell();

    virtual void afterDetach();

private:
    DECLARE_SCRIPT_REGISTER_FUNCTION(CGameObject);
};

class CNightVisionEffector
{
    CActor* m_pActor;
    HUD_SOUND_COLLECTION m_sounds;

public:
    enum EPlaySounds
    {
        eStartSound = 0,
        eStopSound,
        eIdleSound,
        eBrokeSound
    };
    CNightVisionEffector(const shared_str& sect);
    void Start(const shared_str& sect, CActor* pA, bool play_sound = true);
    void Stop(const float factor, bool play_sound = true);
    bool IsActive();
    void OnDisabled(CActor* pA, bool play_sound = true);
    void PlaySounds(EPlaySounds which);
};
