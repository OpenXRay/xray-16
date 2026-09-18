#pragma once

#include "xrEngine/editor_base.h"

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
};
