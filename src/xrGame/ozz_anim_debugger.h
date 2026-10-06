#pragma once

#include "xrEngine/editor_base.h"

class CHudIKController;
class CHudWeaponCollision;
class IKinematics;

class COzzAnimDebugger final : public xray::editor::ide_tool
{
public:
    COzzAnimDebugger();
    void on_tool_frame() override;

private:
    pcstr tool_name() const override { return "Ozz Anim Debugger"; }

    struct TargetEntry
    {
        xr_string label;
        IKinematicsAnimated* kin = nullptr;
    };

    void RefreshTargets();
    void RefreshMotionsForSelected();
    void PlayOnSelected(const shared_str& motion_name);
    void PlayOnAll(const shared_str& motion_name);

    IKinematicsAnimated* SelectedKin() const;

    void DrawHudIKPanel();
    enum class HudTargetKind : u32
    {
        Unknown,
        Hands,
        MonolithicItem,
        SeparateItem
    };

    static HudTargetKind ClassifyHudTarget(IKinematicsAnimated* kin, int& slot);
    bool SelectHudTarget(IKinematicsAnimated* kin);
    void DrawHudIKTargetHeader(IKinematicsAnimated* kin);
    bool DrawHudIKGunUnavailable(IKinematicsAnimated* kin);
    bool DrawHudIKNoController(IKinematicsAnimated* kin);
    static pcstr EquippedWeaponSection(int& slot);
    void DrawHudWeaponCollision(HudTargetKind kind, int slot);
    void DrawHudWeaponCollisionSettings(CHudWeaponCollision& collision);
    void DrawHudWeaponCollisionState(const CHudWeaponCollision& collision);
    static bool IsFiniteVector(const Fvector& v);
    void DrawHudIKPlot(const CHudIKController& ctrl);
    void DrawHudIKDiagnostics(const CHudIKController& ctrl);
    void DrawHudIKArmEditor(CHudIKController& ctrl, u16 arm);
    bool DrawBonePicker(pcstr label, IKinematics* kin, u16& bone);
    void DrawHudIKGunEditor(CHudIKController& ctrl, bool externalLayout);
    void DrawHudIKGunDiagnostics(const CHudIKController& ctrl);
    void CopyHudIKGunSettings(const CHudIKController& ctrl) const;
    xr_string BuildHudIKGunText(const CHudIKController& ctrl) const;
    static pcstr HudIKBoneName(IKinematics* skeleton, u16 bone);
    static bool IsBoneAncestorOrSelf(IKinematics* skeleton, u16 bone, u16 ancestor);
    static bool GunBoneConflictsWithArms(const CHudIKController& ctrl, u16 gunBone);
    void CopyHudIKArmSettings(const CHudIKController& ctrl, u16 arm) const;

    xr_vector<TargetEntry> m_hudTargets;
    xr_vector<TargetEntry> m_worldTargets;

    xr_vector<shared_str>  m_motions;

    bool m_selectedIsHud = false;
    int  m_selectedIdx = -1;
    u32  m_targets_refreshed_frame = 0;
    bool m_mix = true;
    bool m_play_on_all = false;
    char m_target_filter[128] = {};
    char m_motion_filter[128] = {};

    int  m_ikArm = 0;
    int  m_ikPlane = 1;
    bool m_ikFit = true;
    float m_ikScale = 300.f;
    float m_ikCenter[2] = {};
    bool m_ikShowAnimated = true;
    bool m_ikShowResolved = true;
    bool m_ikShowDesired = true;
    char m_ikBoneFilter[64] = {};
    xr_string m_ikMessage;
    xr_string m_ikGunMessage;
    IKinematicsAnimated* m_lastSelectedKin = nullptr;
};
