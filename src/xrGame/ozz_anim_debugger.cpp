#include "StdAfx.h"
#include "ozz_anim_debugger.h"
#include "player_hud.h"
#include "ik/HudIKController.h"

#include "xrEngine/device.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/xr_object_list.h"

#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/KinematicsAnimated.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>

extern int g_debug_utils;

COzzAnimDebugger::COzzAnimDebugger()
{
    ImGui::SetCurrentContext(Device.GetImGuiContext());
}

IKinematicsAnimated* COzzAnimDebugger::SelectedKin() const
{
    const auto& vec = m_selectedIsHud ? m_hudTargets : m_worldTargets;
    if (m_selectedIdx < 0 || m_selectedIdx >= static_cast<int>(vec.size()))
        return nullptr;
    return vec[m_selectedIdx].kin;
}

void COzzAnimDebugger::RefreshTargets()
{
    m_hudTargets.clear();
    m_worldTargets.clear();

    if (g_player_hud)
    {
        if (IKinematicsAnimated* hands = g_player_hud->get_hands_model())
            m_hudTargets.push_back({"[hands]", hands});

        for (u16 idx = 0; idx < 2; ++idx)
        {
            auto* item = g_player_hud->attached_item(idx);
            if (!item || !item->m_model)
                continue;
            IKinematicsAnimated* ka = smart_cast<IKinematicsAnimated*>(item->m_model);
            if (ka)
            {
                string128 buf;
                xr_sprintf(buf, sizeof(buf), "[weapon %u] %s", idx, item->m_sect_name.c_str());
                m_hudTargets.push_back({buf, ka});
            }
        }
    }

    if (g_pGameLevel)
    {
        const u32 count = g_pGameLevel->Objects.o_count();
        for (u32 i = 0; i < count; ++i)
        {
            IGameObject* obj = g_pGameLevel->Objects.o_get_by_iterator(i);
            if (!obj || !obj->Visual())
                continue;
            IKinematicsAnimated* kin = obj->Visual()->dcast_PKinematicsAnimated();
            if (!kin)
                continue;
            m_worldTargets.push_back({obj->cName().c_str(), kin});
        }

        std::sort(m_worldTargets.begin(), m_worldTargets.end(),
            [](const TargetEntry& a, const TargetEntry& b) {
                return xr_strcmp(a.label.c_str(), b.label.c_str()) < 0;
            });
    }

    const auto& vec = m_selectedIsHud ? m_hudTargets : m_worldTargets;
    if (m_selectedIdx >= static_cast<int>(vec.size()))
    {
        m_selectedIdx = vec.empty() ? -1 : 0;
        RefreshMotionsForSelected();
    }

    IKinematicsAnimated* const selected = SelectedKin();
    if (selected != m_lastSelectedKin)
    {
        m_lastSelectedKin = selected;
        RefreshMotionsForSelected();
        m_ikMessage.clear();
    }
}

void COzzAnimDebugger::RefreshMotionsForSelected()
{
    m_motions.clear();
    IKinematicsAnimated* kin = SelectedKin();
    if (!kin)
        return;

    kin->EnumerateCycleNames(m_motions);

    std::sort(m_motions.begin(), m_motions.end(),
        [](const shared_str& a, const shared_str& b) {
            return xr_strcmp(a.c_str(), b.c_str()) < 0;
        });
}

void COzzAnimDebugger::PlayOnSelected(const shared_str& motion_name)
{
    IKinematicsAnimated* kin = SelectedKin();
    if (!kin)
        return;

    MotionID id = kin->ID_Cycle_Safe(motion_name.c_str());
    if (!id.valid())
        return;

    kin->PlayCycle(id, m_mix ? TRUE : FALSE);
}

void COzzAnimDebugger::PlayOnAll(const shared_str& motion_name)
{
    auto play = [&](const xr_vector<TargetEntry>& vec)
    {
        u32 queued = 0;
        for (const auto& te : vec)
        {
            if (!te.kin)
                continue;
            MotionID id = te.kin->ID_Cycle_Safe(motion_name.c_str());
            if (!id.valid())
                continue;
            te.kin->PlayCycle(id, m_mix ? TRUE : FALSE);
            ++queued;
        }
        return queued;
    };
    u32 total = play(m_hudTargets) + play(m_worldTargets);
    Msg("* [OzzDbg] play_on_all '%s': queued=%u", motion_name.c_str(), total);
}

void COzzAnimDebugger::on_tool_frame()
{
#ifdef MASTER_GOLD
    if (!g_debug_utils)
        return;
#endif
    if (!get_open_state())
        return;

    if (Device.dwFrame != m_targets_refreshed_frame)
    {
        RefreshTargets();
        m_targets_refreshed_frame = Device.dwFrame;
    }

    if (!ImGui::Begin(tool_name(), &get_open_state(), get_default_window_flags()))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Mix on play", &m_mix);
    ImGui::SameLine();
    ImGui::Checkbox("Broadcast (play on all)", &m_play_on_all);

    ImGui::Separator();

    const float child_height = ImGui::GetContentRegionAvail().y - 10.0f;
    const float col_width = (ImGui::GetContentRegionAvail().x - 10.0f) * 0.5f;

    if (ImGui::BeginChild("targets", ImVec2(col_width, child_height), ImGuiChildFlags_Borders))
    {
        ImGui::Text("Targets: HUD=%u World=%u",
            static_cast<u32>(m_hudTargets.size()),
            static_cast<u32>(m_worldTargets.size()));
        ImGui::InputTextWithHint("##target_filter", "filter targets", m_target_filter, sizeof(m_target_filter));

        auto drawList = [&](const xr_vector<TargetEntry>& vec, bool isHud)
        {
            for (int i = 0; i < static_cast<int>(vec.size()); ++i)
            {
                const auto& te = vec[i];
                if (m_target_filter[0] && !strstr(te.label.c_str(), m_target_filter))
                    continue;
                const bool is_sel = (m_selectedIsHud == isHud && i == m_selectedIdx);
                if (ImGui::Selectable(te.label.c_str(), is_sel))
                {
                    m_selectedIsHud = isHud;
                    m_selectedIdx = i;
                    RefreshMotionsForSelected();
                }
            }
        };

        if (!m_hudTargets.empty() && ImGui::TreeNodeEx("HUD", ImGuiTreeNodeFlags_DefaultOpen))
        {
            drawList(m_hudTargets, true);
            ImGui::TreePop();
        }
        if (!m_worldTargets.empty() && ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen))
        {
            drawList(m_worldTargets, false);
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("motions", ImVec2(col_width, child_height), ImGuiChildFlags_Borders))
    {
        if (ImGui::BeginTabBar("##ozz_right_tabs"))
        {
            if (ImGui::BeginTabItem("Motions"))
            {
                ImGui::Text("Motions: %u", static_cast<u32>(m_motions.size()));
                ImGui::InputTextWithHint("##motion_filter", "filter motions", m_motion_filter, sizeof(m_motion_filter));

                for (const auto& name : m_motions)
                {
                    if (m_motion_filter[0] && !strstr(name.c_str(), m_motion_filter))
                        continue;
                    if (ImGui::Button(name.c_str()))
                    {
                        if (m_play_on_all)
                            PlayOnAll(name);
                        else
                            PlayOnSelected(name);
                    }
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("HUD IK"))
            {
                DrawHudIKPanel();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

void COzzAnimDebugger::DrawHudIKPanel()
{
    IKinematicsAnimated* kin = SelectedKin();
    if (!m_selectedIsHud || !kin || !g_player_hud)
    {
        ImGui::TextWrapped("Select a HUD skeleton ([hands] or a monolithic weapon) in the target list to author arm IK.");
        return;
    }

    CHudIKController* ctrl = g_player_hud->hud_ik(kin);
    if (!ctrl || !ctrl->Skeleton())
    {
        ImGui::TextWrapped("The selected HUD skeleton has no arm IK controller. Only the shared hands skeleton and monolithic item skeletons are supported.");
        return;
    }

    ctrl->RequestSnapshot();

    ImGui::TextWrapped("Independent per-arm wrist targets for authoring. Targets are same-skeleton Animated, Model or Bone-relative grip primitives, snapshotted before either arm is corrected. Cross-model weapon sockets, automatic two-hand weapon fitting, VR hand input and collision are not part of this panel.");

    ImGui::SeparatorText("Model-space diagnostic plot");
    DrawHudIKPlot(*ctrl);

    ImGui::SeparatorText("Solver diagnostics");
    DrawHudIKDiagnostics(*ctrl);

    ImGui::SeparatorText("Arm authoring");
    ImGui::RadioButton("Left arm", &m_ikArm, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Right arm", &m_ikArm, 1);
    DrawHudIKArmEditor(*ctrl, static_cast<u16>(m_ikArm ? 1 : 0));

    if (!m_ikMessage.empty())
        ImGui::TextWrapped("%s", m_ikMessage.c_str());
}

bool COzzAnimDebugger::DrawBonePicker(pcstr label, IKinematics* kin, u16& bone)
{
    const u16 count = kin->LL_BoneCount();
    pcstr preview = "<invalid>";
    if (bone == BI_NONE)
        preview = "<none>";
    else if (bone < count)
    {
        pcstr name = kin->LL_BoneName_dbg(bone);
        preview = name ? name : "<unnamed>";
    }

    bool changed = false;
    if (ImGui::BeginCombo(label, preview))
    {
        if (ImGui::IsWindowAppearing())
        {
            m_ikBoneFilter[0] = 0;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputTextWithHint("##bone_filter", "filter bones", m_ikBoneFilter, sizeof(m_ikBoneFilter));

        if (ImGui::Selectable("<none>", bone == BI_NONE) && bone != BI_NONE)
        {
            bone = BI_NONE;
            changed = true;
        }

        for (u16 i = 0; i < count; ++i)
        {
            pcstr name = kin->LL_BoneName_dbg(i);
            if (!name)
                continue;
            if (m_ikBoneFilter[0] && !strstr(name, m_ikBoneFilter))
                continue;
            ImGui::PushID(static_cast<int>(i));
            const bool is_sel = (bone == i);
            if (ImGui::Selectable(name, is_sel) && !is_sel)
            {
                bone = i;
                changed = true;
            }
            if (is_sel)
                ImGui::SetItemDefaultFocus();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    return changed;
}

void COzzAnimDebugger::DrawHudIKArmEditor(CHudIKController& ctrl, u16 arm)
{
    IKinematics* skeleton = ctrl.Skeleton();
    if (!skeleton)
        return;

    const pcstr armName = arm ? "right" : "left";
    const pcstr spaceLabels[3] = {
        "Animated (offset from animated wrist)",
        "Model (skeleton model space)",
        "Bone (offset from current target bone)",
    };

    CHudIKController::ArmSettings settings = ctrl.GetArm(arm);
    bool changed = false;

    ImGui::PushID(static_cast<int>(arm));

    changed |= ImGui::Checkbox("Enabled", &settings.enabled);
    ImGui::SameLine();
    ImGui::TextDisabled("(disabled arms leave final bones untouched)");

    changed |= DrawBonePicker("Upper arm bone", skeleton, settings.bones[0]);
    changed |= DrawBonePicker("Forearm bone", skeleton, settings.bones[1]);
    changed |= DrawBonePicker("Hand (wrist) bone", skeleton, settings.bones[2]);

    int space = static_cast<int>(settings.space);
    if (ImGui::Combo("Target space", &space, spaceLabels, 3))
    {
        settings.space = static_cast<CHudIKController::TargetSpace>(space);
        changed = true;
    }
    if (settings.space == CHudIKController::TargetSpace::Bone)
    {
        changed |= DrawBonePicker("Target bone", skeleton, settings.targetBone);
        if (settings.targetBone == BI_NONE)
            ImGui::TextColored(ImVec4(1.f, 0.75f, 0.3f, 1.f), "Pick a target bone for a bone-relative target.");
    }
    ImGui::TextDisabled("Changing the space keeps the numbers; use capture to preserve the current animated wrist.");

    changed |= ImGui::DragFloat3("Wrist translation (m)", &settings.position.x, 0.001f, 0.f, 0.f, "%.4f");
    changed |= ImGui::DragFloat3("Wrist rotation H/P/B (deg)", &settings.rotation.x, 0.25f, 0.f, 0.f, "%.2f");
    changed |= ImGui::DragFloat3("Elbow offset (model m)", &settings.elbowOffset.x, 0.001f, 0.f, 0.f, "%.4f");
    changed |= ImGui::SliderFloat("Weight", &settings.weight, 0.f, 1.f, "%.2f");

    if (changed)
        ctrl.SetArm(arm, settings);

    if (ImGui::Button("Reset arm"))
    {
        ctrl.ResetArm(arm);
        m_ikMessage = "Reset arm to controller defaults.";
    }
    ImGui::SameLine();

    const bool boneMissing = settings.space == CHudIKController::TargetSpace::Bone && settings.targetBone == BI_NONE;
    ImGui::BeginDisabled(boneMissing);
    char captureLabel[96];
    xr_sprintf(captureLabel, sizeof(captureLabel), "Capture animated wrist (%s space)",
        settings.space == CHudIKController::TargetSpace::Animated ? "Animated" :
        settings.space == CHudIKController::TargetSpace::Model ? "Model" : "Bone");
    if (ImGui::Button(captureLabel))
    {
        const bool ok = ctrl.CaptureTarget(arm, settings.space, settings.targetBone);
        string128 msg;
        xr_sprintf(msg, sizeof(msg), ok ? "Captured %s animated wrist into the target." :
            "Capture failed for %s arm: no fresh animated wrist or target bone available.", armName);
        m_ikMessage = msg;
    }
    ImGui::EndDisabled();

    if (ImGui::Button("Copy arm settings"))
    {
        CopyHudIKArmSettings(ctrl, arm);
        m_ikMessage = "Copied selected arm settings to the clipboard.";
    }

    ImGui::PopID();
}

void COzzAnimDebugger::CopyHudIKArmSettings(const CHudIKController& ctrl, u16 arm) const
{
    IKinematics* skeleton = ctrl.Skeleton();
    const CHudIKController::ArmSettings& settings = ctrl.GetArm(arm);

    auto boneName = [&](u16 id) -> pcstr
    {
        if (id == BI_NONE)
            return "";
        if (!skeleton || id >= skeleton->LL_BoneCount())
            return "<invalid>";
        pcstr name = skeleton->LL_BoneName_dbg(id);
        return name ? name : "<unnamed>";
    };

    pcstr spaceName = "animated";
    if (settings.space == CHudIKController::TargetSpace::Model)
        spaceName = "model";
    else if (settings.space == CHudIKController::TargetSpace::Bone)
        spaceName = "bone";

    const auto& vec = m_selectedIsHud ? m_hudTargets : m_worldTargets;
    pcstr skeletonLabel = "";
    if (m_selectedIdx >= 0 && m_selectedIdx < static_cast<int>(vec.size()))
        skeletonLabel = vec[m_selectedIdx].label.c_str();

    string512 line;
    xr_string text;
    xr_sprintf(line, sizeof(line), "hud_ik reference/session settings (not auto-loaded) arm=%s skeleton=%s\n", arm ? "right" : "left", skeletonLabel);
    text += line;
    xr_sprintf(line, sizeof(line), "enabled=%d\n", settings.enabled ? 1 : 0);
    text += line;
    xr_sprintf(line, sizeof(line), "upperarm_bone=%s\nforearm_bone=%s\nhand_bone=%s\n",
        boneName(settings.bones[0]), boneName(settings.bones[1]), boneName(settings.bones[2]));
    text += line;
    xr_sprintf(line, sizeof(line), "target_space=%s\ntarget_bone=%s\n", spaceName, boneName(settings.targetBone));
    text += line;
    xr_sprintf(line, sizeof(line), "offset_position=%.5f, %.5f, %.5f\n",
        settings.position.x, settings.position.y, settings.position.z);
    text += line;
    xr_sprintf(line, sizeof(line), "offset_hpb_deg=%.4f, %.4f, %.4f\n",
        settings.rotation.x, settings.rotation.y, settings.rotation.z);
    text += line;
    xr_sprintf(line, sizeof(line), "elbow_offset_model=%.5f, %.5f, %.5f\n",
        settings.elbowOffset.x, settings.elbowOffset.y, settings.elbowOffset.z);
    text += line;
    xr_sprintf(line, sizeof(line), "weight=%.4f\n", settings.weight);
    text += line;

    ImGui::SetClipboardText(text.c_str());
}

void COzzAnimDebugger::DrawHudIKDiagnostics(const CHudIKController& ctrl)
{
    if (!ImGui::BeginTable("##hud_ik_diag", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        return;

    ImGui::TableSetupColumn("Metric");
    ImGui::TableSetupColumn("Left");
    ImGui::TableSetupColumn("Right");
    ImGui::TableHeadersRow();

    const u32 deviceFrame = Device.dwFrame;

    auto beginRow = [](pcstr label)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
    };

    beginRow("Status");
    for (u16 a = 0; a < 2; ++a)
    {
        const CHudIKController::ArmState& s = ctrl.GetState(a);
        ImGui::TableSetColumnIndex(a + 1);
        ImVec4 color(0.6f, 0.6f, 0.6f, 1.f);
        if (ctrl.GetArm(a).enabled)
            color = s.solved ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.7f, 0.3f, 1.f);
        ImGui::TextColored(color, "%s", s.status ? s.status : "?");
    }

    beginRow("Enabled / valid / solved");
    for (u16 a = 0; a < 2; ++a)
    {
        const CHudIKController::ArmState& s = ctrl.GetState(a);
        ImGui::TableSetColumnIndex(a + 1);
        ImGui::Text("%s / %s / %s", ctrl.GetArm(a).enabled ? "yes" : "no", s.valid ? "yes" : "no", s.solved ? "yes" : "no");
    }

    beginRow("Fresh-frame stamp");
    for (u16 a = 0; a < 2; ++a)
    {
        const CHudIKController::ArmState& s = ctrl.GetState(a);
        ImGui::TableSetColumnIndex(a + 1);
        const u32 age = deviceFrame >= s.frame ? deviceFrame - s.frame : 0;
        if (ctrl.GetArm(a).enabled && age > 2)
            ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "%u (stale, age %u)", s.frame, age);
        else
            ImGui::Text("%u (age %u)", s.frame, age);
    }

    beginRow("Endpoint error (m)");
    for (u16 a = 0; a < 2; ++a)
    {
        ImGui::TableSetColumnIndex(a + 1);
        ImGui::Text("%.4f", ctrl.GetState(a).error);
    }

    beginRow("Desired wrist (input)");
    for (u16 a = 0; a < 2; ++a)
    {
        const Fvector& p = ctrl.GetState(a).target.c;
        ImGui::TableSetColumnIndex(a + 1);
        ImGui::Text("%.3f %.3f %.3f", p.x, p.y, p.z);
    }

    beginRow("Resolved / preview wrist");
    for (u16 a = 0; a < 2; ++a)
    {
        const Fvector& p = ctrl.GetState(a).resolved[2].c;
        ImGui::TableSetColumnIndex(a + 1);
        ImGui::Text("%.3f %.3f %.3f", p.x, p.y, p.z);
    }

    beginRow("Animated wrist (input)");
    for (u16 a = 0; a < 2; ++a)
    {
        const Fvector& p = ctrl.GetState(a).animated[2].c;
        ImGui::TableSetColumnIndex(a + 1);
        ImGui::Text("%.3f %.3f %.3f", p.x, p.y, p.z);
    }

    beginRow("Elbow hint (input)");
    for (u16 a = 0; a < 2; ++a)
    {
        const Fvector& p = ctrl.GetState(a).elbow;
        ImGui::TableSetColumnIndex(a + 1);
        ImGui::Text("%.3f %.3f %.3f", p.x, p.y, p.z);
    }

    ImGui::EndTable();
}

void COzzAnimDebugger::DrawHudIKPlot(const CHudIKController& ctrl)
{
    const float kRange = 50.f;
    auto sanitizeView = [&]()
    {
        if (!std::isfinite(m_ikScale) || m_ikScale <= 0.f)
            m_ikScale = 300.f;
        m_ikScale = std::clamp(m_ikScale, 20.f, 4000.f);
        for (float& c : m_ikCenter)
            c = std::isfinite(c) ? std::clamp(c, -kRange, kRange) : 0.f;
    };
    sanitizeView();
    const pcstr planeNames[3] = {"XY", "XZ", "YZ"};
    const pcstr axisNames[3] = {"X", "Y", "Z"};
    for (int i = 0; i < 3; ++i)
    {
        if (i)
            ImGui::SameLine();
        ImGui::RadioButton(planeNames[i], &m_ikPlane, i);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Fit", &m_ikFit);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::SliderFloat("Zoom", &m_ikScale, 20.f, 4000.f, "%.0f px/m",
            ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp))
        m_ikFit = false;
    sanitizeView();
    ImGui::Checkbox("Animated input", &m_ikShowAnimated);
    ImGui::SameLine();
    ImGui::Checkbox("Resolved output", &m_ikShowResolved);
    ImGui::SameLine();
    ImGui::Checkbox("Desired inputs", &m_ikShowDesired);

    const int ax = m_ikPlane == 2 ? 1 : 0;
    const int ay = m_ikPlane == 0 ? 1 : 2;
    auto comp = [](const Fvector& v, int i) { return i == 0 ? v.x : (i == 1 ? v.y : v.z); };

    const ImVec2 size(std::clamp(ImGui::GetContentRegionAvail().x, 120.f, 4096.f), 280.f);
    const ImVec2 p0 = ImGui::GetCursorScreenPos();
    const ImVec2 p1(p0.x + size.x, p0.y + size.y);
    ImGui::InvisibleButton("##hud_ik_plot", size, ImGuiButtonFlags_MouseButtonLeft);
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    ImGuiIO& io = ImGui::GetIO();

    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        m_ikFit = true;
    else
    {
        if (hovered && std::isfinite(io.MouseWheel) && io.MouseWheel != 0.f)
        {
            m_ikScale = m_ikScale * powf(1.1f, std::clamp(io.MouseWheel, -8.f, 8.f));
            m_ikFit = false;
        }
        if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.f) && std::isfinite(io.MouseDelta.x) &&
            std::isfinite(io.MouseDelta.y))
        {
            m_ikCenter[0] -= io.MouseDelta.x / m_ikScale;
            m_ikCenter[1] += io.MouseDelta.y / m_ikScale;
            m_ikFit = false;
        }
    }
    sanitizeView();

    bool any = false;
    float mn[2] = {};
    float mx[2] = {};
    auto include = [&](const Fvector& p)
    {
        const float h = comp(p, ax);
        const float v = comp(p, ay);
        if (!any)
        {
            mn[0] = mx[0] = h;
            mn[1] = mx[1] = v;
            any = true;
            return;
        }
        mn[0] = std::min(mn[0], h);
        mx[0] = std::max(mx[0], h);
        mn[1] = std::min(mn[1], v);
        mx[1] = std::max(mx[1], v);
    };

    auto okPoint = [&](const Fvector& p)
    {
        const float h = comp(p, ax);
        const float v = comp(p, ay);
        return std::isfinite(h) && std::isfinite(v) && std::fabs(h) <= kRange && std::fabs(v) <= kRange;
    };
    auto okChain = [&](const Fmatrix* m) { return okPoint(m[0].c) && okPoint(m[1].c) && okPoint(m[2].c); };

    u32 omitted = 0;
    for (u16 a = 0; a < 2; ++a)
    {
        const CHudIKController::ArmState& s = ctrl.GetState(a);
        if (!s.valid)
            continue;
        if (okChain(s.animated))
        {
            for (int j = 0; j < 3; ++j)
                include(s.animated[j].c);
        }
        else
            ++omitted;
        if (ctrl.GetArm(a).enabled)
        {
            if (okChain(s.resolved))
            {
                for (int j = 0; j < 3; ++j)
                    include(s.resolved[j].c);
            }
            else
                ++omitted;
        }
        if (okPoint(s.target.c))
            include(s.target.c);
        else
            ++omitted;
        if (okPoint(s.elbow))
            include(s.elbow);
        else
            ++omitted;
    }

    if (m_ikFit && any)
    {
        const float extent = std::max(std::max(mx[0] - mn[0], mx[1] - mn[1]), 0.05f);
        m_ikScale = std::clamp(std::min(size.x, size.y) / (extent * 1.3f), 20.f, 4000.f);
        m_ikCenter[0] = (mn[0] + mx[0]) * 0.5f;
        m_ikCenter[1] = (mn[1] + mx[1]) * 0.5f;
    }

    const ImVec2 mid(p0.x + size.x * 0.5f, p0.y + size.y * 0.5f);
    auto projectUV = [&](float h, float v)
    {
        return ImVec2(mid.x + (h - m_ikCenter[0]) * m_ikScale, mid.y - (v - m_ikCenter[1]) * m_ikScale);
    };
    auto project = [&](const Fvector& p) { return projectUV(comp(p, ax), comp(p, ay)); };

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(p0, p1, true);
    dl->AddRectFilled(p0, p1, IM_COL32(18, 20, 24, 255));

    const float steps[] = {0.005f, 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.f, 2.f, 5.f};
    float step = steps[IM_ARRAYSIZE(steps) - 1];
    for (float candidate : steps)
    {
        if (candidate * m_ikScale >= 28.f)
        {
            step = candidate;
            break;
        }
    }

    const float halfW = size.x * 0.5f / m_ikScale;
    const float halfH = size.y * 0.5f / m_ikScale;
    const int h0 = static_cast<int>(floorf((m_ikCenter[0] - halfW) / step));
    const int h1 = static_cast<int>(ceilf((m_ikCenter[0] + halfW) / step));
    const int v0 = static_cast<int>(floorf((m_ikCenter[1] - halfH) / step));
    const int v1 = static_cast<int>(ceilf((m_ikCenter[1] + halfH) / step));
    if (h1 - h0 <= 512 && v1 - v0 <= 512)
    {
        for (int i = h0; i <= h1; ++i)
        {
            const ImVec2 a = projectUV(i * step, m_ikCenter[1] - halfH);
            const ImVec2 b = projectUV(i * step, m_ikCenter[1] + halfH);
            dl->AddLine(a, b, i == 0 ? IM_COL32(110, 110, 130, 255) : IM_COL32(45, 48, 56, 255));
        }
        for (int i = v0; i <= v1; ++i)
        {
            const ImVec2 a = projectUV(m_ikCenter[0] - halfW, i * step);
            const ImVec2 b = projectUV(m_ikCenter[0] + halfW, i * step);
            dl->AddLine(a, b, i == 0 ? IM_COL32(110, 110, 130, 255) : IM_COL32(45, 48, 56, 255));
        }
    }

    char caption[128];
    xr_sprintf(caption, sizeof(caption), "%s plane  horizontal=%s vertical=%s  grid %g cm  model space",
        planeNames[m_ikPlane], axisNames[ax], axisNames[ay], step * 100.f);
    dl->AddText(ImVec2(p0.x + 6.f, p0.y + 4.f), IM_COL32(170, 175, 190, 255), caption);

    if (!any)
        dl->AddText(ImVec2(p0.x + 6.f, p0.y + 24.f), IM_COL32(255, 180, 90, 255),
            "No valid arm state yet (waiting for the HUD skeleton callback)");

    if (omitted)
    {
        char omittedText[96];
        xr_sprintf(omittedText, sizeof(omittedText), "%u element(s) non-finite or beyond +/-50 m omitted", omitted);
        dl->AddText(ImVec2(p0.x + 6.f, p0.y + 44.f), IM_COL32(255, 120, 90, 255), omittedText);
    }

    for (u16 a = 0; a < 2; ++a)
    {
        const CHudIKController::ArmState& s = ctrl.GetState(a);
        if (!s.valid)
            continue;

        const bool enabled = ctrl.GetArm(a).enabled;
        const pcstr tag = a ? "R" : "L";
        const ImU32 base = a ? IM_COL32(255, 170, 70, 255) : IM_COL32(80, 200, 255, 255);
        const ImU32 dim = a ? IM_COL32(255, 170, 70, 110) : IM_COL32(80, 200, 255, 110);
        char label[16];

        if (m_ikShowAnimated && okChain(s.animated))
        {
            ImVec2 pts[3];
            for (int j = 0; j < 3; ++j)
                pts[j] = project(s.animated[j].c);
            dl->AddLine(pts[0], pts[1], dim, 1.5f);
            dl->AddLine(pts[1], pts[2], dim, 1.5f);
            for (int j = 0; j < 3; ++j)
                dl->AddCircle(pts[j], 3.5f, dim, 12, 1.5f);
            xr_sprintf(label, sizeof(label), "%s anim", tag);
            dl->AddText(ImVec2(pts[2].x + 6.f, pts[2].y + 2.f), dim, label);
        }

        const bool resolvedOk = enabled && okChain(s.resolved);
        if (resolvedOk && m_ikShowResolved)
        {
            ImVec2 pts[3];
            for (int j = 0; j < 3; ++j)
                pts[j] = project(s.resolved[j].c);
            dl->AddLine(pts[0], pts[1], base, 3.5f);
            dl->AddLine(pts[1], pts[2], base, 3.5f);
            for (int j = 0; j < 3; ++j)
                dl->AddCircleFilled(pts[j], j == 2 ? 5.5f : 4.5f, base);
            xr_sprintf(label, sizeof(label), "%s out", tag);
            dl->AddText(ImVec2(pts[2].x + 6.f, pts[2].y - 14.f), base, label);
        }

        if (m_ikShowDesired && okPoint(s.target.c))
        {
            const ImVec2 tp = project(s.target.c);
            if (resolvedOk)
                dl->AddLine(project(s.resolved[2].c), tp, IM_COL32(255, 60, 60, 200), 1.5f);

            const float axisLength = 0.04f;
            const Fvector* axes[3] = {&s.target.i, &s.target.j, &s.target.k};
            const ImU32 axisColors[3] = {IM_COL32(255, 80, 80, 255), IM_COL32(90, 255, 90, 255), IM_COL32(100, 140, 255, 255)};
            for (int j = 0; j < 3; ++j)
            {
                Fvector e;
                e.set(s.target.c.x + axes[j]->x * axisLength, s.target.c.y + axes[j]->y * axisLength,
                    s.target.c.z + axes[j]->z * axisLength);
                if (okPoint(e))
                    dl->AddLine(tp, project(e), axisColors[j], 2.f);
            }
            dl->AddQuad(ImVec2(tp.x, tp.y - 7.f), ImVec2(tp.x + 7.f, tp.y), ImVec2(tp.x, tp.y + 7.f),
                ImVec2(tp.x - 7.f, tp.y), base, 2.f);
            xr_sprintf(label, sizeof(label), "%s target", tag);
            dl->AddText(ImVec2(tp.x + 9.f, tp.y + 4.f), base, label);
        }

        if (m_ikShowDesired && okPoint(s.elbow))
        {
            const ImVec2 hint = project(s.elbow);
            const ImU32 hintColor = IM_COL32(255, 235, 80, 255);
            if (okPoint(s.resolved[1].c))
                dl->AddLine(project(s.resolved[1].c), hint, IM_COL32(255, 235, 80, 140), 1.f);
            dl->AddRect(ImVec2(hint.x - 5.f, hint.y - 5.f), ImVec2(hint.x + 5.f, hint.y + 5.f), hintColor, 0.f, 0, 2.f);
            xr_sprintf(label, sizeof(label), "%s elbow", tag);
            dl->AddText(ImVec2(hint.x + 8.f, hint.y - 14.f), hintColor, label);
        }
    }

    dl->PopClipRect();
    dl->AddRect(p0, p1, IM_COL32(90, 95, 110, 255));

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.f, 1.f), "Solver output: thick solid chain, filled joints (left cyan, right orange)");
    ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.8f, 1.f), "Animated input: thin faded chain, hollow joints");
    ImGui::TextColored(ImVec4(1.f, 0.92f, 0.4f, 1.f), "Desired inputs: diamond with RGB axes = wrist target, yellow square = elbow hint, red line = endpoint error");
    ImGui::TextDisabled("Orthographic model-space diagnostic of the selected skeleton, not a viewport gizmo or world projection. Drag pans, wheel zooms, double-click fits. Resolved output only draws for enabled arms.");
}
