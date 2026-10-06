#pragma once

#include "xrEngine/editor_base.h"

class CHudIKController;
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
    void DrawHudIKPlot(const CHudIKController& ctrl);
    void DrawHudIKDiagnostics(const CHudIKController& ctrl);
    void DrawHudIKArmEditor(CHudIKController& ctrl, u16 arm);
    bool DrawBonePicker(pcstr label, IKinematics* kin, u16& bone);
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
    IKinematicsAnimated* m_lastSelectedKin = nullptr;
};
