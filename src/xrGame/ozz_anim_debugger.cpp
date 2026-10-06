#include "StdAfx.h"
#include "ozz_anim_debugger.h"
#include "player_hud.h"
#include "ik/HudIKController.h"
#include "ik/HudWeaponCollision.h"

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
        m_ikGunMessage.clear();
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
                    m_lastSelectedKin = SelectedKin();
                    m_ikMessage.clear();
                    m_ikGunMessage.clear();
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

COzzAnimDebugger::HudTargetKind COzzAnimDebugger::ClassifyHudTarget(IKinematicsAnimated* kin, int& slot)
{
    slot = -1;
    if (!g_player_hud || !kin)
        return HudTargetKind::Unknown;
    if (g_player_hud->get_hands_model() == kin)
        return HudTargetKind::Hands;
    for (u16 idx = 0; idx < 2; ++idx)
    {
        attachable_hud_item* item = g_player_hud->attached_item(idx);
        if (!item || !item->m_model)
            continue;
        if (smart_cast<IKinematicsAnimated*>(item->m_model) != kin)
            continue;
        slot = static_cast<int>(idx);
        return item->m_monolithic ? HudTargetKind::MonolithicItem : HudTargetKind::SeparateItem;
    }
    return HudTargetKind::Unknown;
}

bool COzzAnimDebugger::SelectHudTarget(IKinematicsAnimated* kin)
{
    if (!kin)
        return false;
    for (int i = 0; i < static_cast<int>(m_hudTargets.size()); ++i)
    {
        if (m_hudTargets[i].kin != kin)
            continue;
        m_selectedIsHud = true;
        m_selectedIdx = i;
        m_lastSelectedKin = kin;
        RefreshMotionsForSelected();
        m_ikMessage.clear();
        m_ikGunMessage.clear();
        return true;
    }
    return false;
}

void COzzAnimDebugger::DrawHudIKTargetHeader(IKinematicsAnimated* kin)
{
    pcstr label = "<unknown>";
    if (m_selectedIdx >= 0 && m_selectedIdx < static_cast<int>(m_hudTargets.size()))
        label = m_hudTargets[m_selectedIdx].label.c_str();

    int slot = -1;
    const HudTargetKind kind = ClassifyHudTarget(kin, slot);
    pcstr layout = "unknown HUD model";
    if (kind == HudTargetKind::Hands)
        layout = "shared hands skeleton (arms only, no weapon branch)";
    else if (kind == HudTargetKind::MonolithicItem)
        layout = "monolithic item skeleton (weapon and both arm chains in one skeleton)";
    else if (kind == HudTargetKind::SeparateItem)
        layout = "separate item model attached to the shared hands";

    ImGui::Text("Target: %s", label);
    if (slot >= 0)
        ImGui::Text("Attach slot: %d", slot);
    ImGui::TextWrapped("Layout (diagnostic only): %s", layout);

    const CHudIKController* ctrl = g_player_hud->hud_ik(kin);
    if (ctrl && (kind == HudTargetKind::Hands || kind == HudTargetKind::SeparateItem))
        ImGui::TextWrapped("Controller: shared hands controller (the same one opens from [hands] and from the primary separate weapon).");
    else if (ctrl)
        ImGui::TextWrapped("Controller: this skeleton's own controller.");
    else
        ImGui::TextWrapped("Controller: none for this model.");

    for (u16 idx = 0; idx < 2; ++idx)
    {
        attachable_hud_item* item = g_player_hud->attached_item(idx);
        if (!item)
        {
            ImGui::TextDisabled("Slot %u: empty", static_cast<u32>(idx));
            continue;
        }
        pcstr role = idx == 0 ? "primary, equipped weapon" : "secondary";
        pcstr detail = "separate item model, no IK controller";
        if (item->m_monolithic)
            detail = "monolithic item, own skeleton controller";
        else if (idx == 0)
            detail = "separate item model, gun-led through the shared hands controller";
        ImGui::TextDisabled("Slot %u (%s): %s - %s", static_cast<u32>(idx), role, item->m_sect_name.c_str(), detail);
    }
}

pcstr COzzAnimDebugger::EquippedWeaponSection(int& slot)
{
    slot = -1;
    if (!g_player_hud)
        return nullptr;
    attachable_hud_item* item = g_player_hud->attached_item(0);
    if (!item)
        return nullptr;
    slot = 0;
    return item->m_sect_name.c_str();
}

bool COzzAnimDebugger::DrawHudIKNoController(IKinematicsAnimated* kin)
{
    int slot = -1;
    const HudTargetKind kind = ClassifyHudTarget(kin, slot);

    if (kind == HudTargetKind::SeparateItem && slot != 0)
        ImGui::TextWrapped("This is the secondary weapon in slot %d and has no IK controller. Gun-led IK and weapon collision follow the primary weapon (slot 0) only, and arm authoring uses the shared hands controller.", slot);
    else if (kind == HudTargetKind::SeparateItem)
        ImGui::TextWrapped("The shared hands controller is not available for this primary weapon right now. Select [hands] to retry arm authoring.");
    else
        ImGui::TextWrapped("The selected HUD model has no arm IK controller. Select [hands] or the primary weapon (slot 0) to author arm IK and gun-led IK.");

    if (ImGui::Button("Select [hands]") && SelectHudTarget(g_player_hud->get_hands_model()))
        return true;

    attachable_hud_item* primary = g_player_hud->attached_item(0);
    IKinematicsAnimated* primaryKin = primary && primary->m_model ? smart_cast<IKinematicsAnimated*>(primary->m_model) : nullptr;
    if (primaryKin && primaryKin != kin)
    {
        char buttonLabel[160];
        xr_sprintf(buttonLabel, sizeof(buttonLabel), "Select primary weapon 0: %s", primary->m_sect_name.c_str());
        ImGui::SameLine();
        if (ImGui::Button(buttonLabel) && SelectHudTarget(primaryKin))
            return true;
    }
    return false;
}

bool COzzAnimDebugger::DrawHudIKGunUnavailable(IKinematicsAnimated* kin)
{
    int slot = -1;
    const HudTargetKind kind = ClassifyHudTarget(kin, slot);
    const ImVec4 warnColor(1.f, 0.75f, 0.3f, 1.f);

    ImGui::TextColored(warnColor, "Gun-led two-hand IK is not available right now. The controls below are disabled; manual per-arm controls stay available.");

    if (kind == HudTargetKind::MonolithicItem)
    {
        ImGui::TextWrapped("This monolithic weapon's controller is not gun-lead capable yet. Wait for the weapon to finish loading or re-equip it.");
        return false;
    }

    attachable_hud_item* primary = g_player_hud->attached_item(0);
    if (!primary)
    {
        ImGui::TextWrapped("No weapon is equipped in primary slot 0, so there is no equipped weapon to lead from. Equip a weapon to enable the gun controls.");
        if (g_player_hud->attached_item(1))
            ImGui::TextWrapped("The item in slot 1 is secondary; gun-led IK follows the primary weapon only.");
        return false;
    }

    if (primary->m_monolithic)
    {
        ImGui::TextWrapped("The equipped primary weapon (slot 0: %s) uses a monolithic skeleton, so its gun-led IK is authored on that weapon's own controller.", primary->m_sect_name.c_str());
        IKinematicsAnimated* itemKin = primary->m_model ? smart_cast<IKinematicsAnimated*>(primary->m_model) : nullptr;
        if (itemKin && primary->m_hud_ik)
        {
            char buttonLabel[160];
            xr_sprintf(buttonLabel, sizeof(buttonLabel), "Select primary weapon 0: %s", primary->m_sect_name.c_str());
            if (ImGui::Button(buttonLabel) && SelectHudTarget(itemKin))
                return true;
        }
        return false;
    }

    ImGui::TextWrapped("The equipped primary weapon (slot 0: %s) is a separate model but is not configured on the hands controller yet (model or hands skeleton not ready, or no pose evaluated since equip). The gun controls enable once it is configured.",
        primary->m_sect_name.c_str());
    if (g_player_hud->attached_item(1))
        ImGui::TextWrapped("The item in slot 1 is secondary; gun-led IK follows the primary weapon only.");
    return false;
}

void COzzAnimDebugger::DrawHudIKPanel()
{
    IKinematicsAnimated* kin = SelectedKin();
    if (!m_selectedIsHud || !kin || !g_player_hud)
    {
        ImGui::TextWrapped("Select a HUD skeleton ([hands] or a weapon) in the target list to author arm IK.");
        return;
    }

    DrawHudIKTargetHeader(kin);
    ImGui::Separator();

    CHudIKController* ctrl = g_player_hud->hud_ik(kin);
    if (!ctrl || !ctrl->Skeleton())
    {
        DrawHudIKNoController(kin);
        return;
    }

    ctrl->RequestSnapshot();

    ImGui::TextWrapped("Animation-follow IK activates automatically on equip once both arm chains (l_upperarm/l_forearm/l_hand and r_upperarm/r_forearm/r_hand, or the bip01_ equivalents) and a gun reference are valid. Use Activate to retry manually and Release to turn it off until the next equip. It keeps the animated hand motion and moves both hands by the gun displacement every frame. Independent arm targets remain available. VR input and automatic reload release are not implemented.");
    ImGui::TextDisabled("Automatic activation: %s", ctrl->GetAutoStatus());

    int slot = -1;
    const HudTargetKind kind = ClassifyHudTarget(kin, slot);
    const bool externalLayout = ctrl->HasExternalGun() || kind == HudTargetKind::Hands || kind == HudTargetKind::SeparateItem;

    ImGui::SeparatorText("Lead gun / two-hand IK");
    const bool gunSupported = ctrl->SupportsGunLead();
    if (!gunSupported && DrawHudIKGunUnavailable(kin))
        return;
    ImGui::BeginDisabled(!gunSupported);
    DrawHudIKGunEditor(*ctrl, externalLayout);
    ImGui::EndDisabled();

    DrawHudWeaponCollision(kind, slot);

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
    const pcstr spaceLabels[5] = {
        "Animated (offset from animated wrist)",
        "Model (hands/controller model space)",
        "Bone (offset from current target bone)",
        "Fixed gun-relative grip (advanced)",
        "Animated + gun displacement",
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
    if (ImGui::Combo("Target space", &space, spaceLabels, 5))
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
    else if (settings.space == CHudIKController::TargetSpace::Gun)
    {
        int weaponSlot = -1;
        pcstr weaponSection = EquippedWeaponSection(weaponSlot);
        if (ctrl.HasExternalGun() && weaponSection)
            ImGui::TextDisabled("Grip is relative to the equipped weapon origin (slot %d: %s) in hands model space; no target bone is needed.", weaponSlot, weaponSection);
        else if (ctrl.SupportsGunLead() && !ctrl.HasExternalGun())
            ImGui::TextDisabled("Grip is relative to the lead gun target (gun bone: %s); no target bone is needed.", HudIKBoneName(skeleton, ctrl.GetGun().bone));
        else
            ImGui::TextColored(ImVec4(1.f, 0.75f, 0.3f, 1.f), "Gun space needs an equipped lead gun. Capture is disabled until one is available.");
    }
    else if (settings.space == CHudIKController::TargetSpace::GunAnimated)
    {
        ImGui::TextDisabled("Wrist follows the gun; elbow bend follows the animation.");
        if (!ctrl.SupportsGunLead() || !ctrl.GetGun().enabled)
            ImGui::TextDisabled("The gun target is off or unavailable, so this arm plays its unchanged animation.");
    }
    if (settings.space == CHudIKController::TargetSpace::GunAnimated)
        ImGui::TextDisabled("Changing the space keeps the numbers; use the reset below to remove the authored wrist offset.");
    else
        ImGui::TextDisabled("Changing the space keeps the numbers; use capture to preserve the current animated wrist.");
    if (settings.space == CHudIKController::TargetSpace::Bone && ctrl.SupportsGunLead() && ctrl.GetGun().enabled &&
        settings.targetBone != BI_NONE && settings.targetBone == ctrl.GetGun().bone)
        ImGui::TextDisabled("Target bone is the lead gun: the offset follows the planned gun-led target, not the solved pose.");

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
    const bool gunMissing = settings.space == CHudIKController::TargetSpace::Gun && !ctrl.SupportsGunLead();
    ImGui::BeginDisabled(boneMissing || gunMissing);
    pcstr spaceLabel = "Animated";
    if (settings.space == CHudIKController::TargetSpace::Model)
        spaceLabel = "Model";
    else if (settings.space == CHudIKController::TargetSpace::Bone)
        spaceLabel = "Bone";
    else if (settings.space == CHudIKController::TargetSpace::Gun)
        spaceLabel = "Fixed gun grip";
    else if (settings.space == CHudIKController::TargetSpace::GunAnimated)
        spaceLabel = "Animated + gun displacement";
    const bool followSpace = settings.space == CHudIKController::TargetSpace::GunAnimated;
    char captureLabel[96];
    if (followSpace)
        xr_sprintf(captureLabel, sizeof(captureLabel), "%s", "Reset wrist offset (animation-follow)");
    else
        xr_sprintf(captureLabel, sizeof(captureLabel), "Capture animated wrist (%s space)", spaceLabel);
    if (ImGui::Button(captureLabel))
    {
        const bool ok = ctrl.CaptureTarget(arm, settings.space, settings.targetBone);
        string256 msg;
        if (followSpace)
        {
            xr_sprintf(msg, sizeof(msg),
                ok ? "Reset %s arm wrist offset to zero; the animated wrist follows the gun displacement every frame without freezing a pose." :
                     "Reset failed for %s arm: no fresh animated wrist available.",
                armName);
        }
        else
        {
            pcstr failure = settings.space == CHudIKController::TargetSpace::Gun ?
                "Capture failed for %s arm: no fresh animated wrist or equipped gun available." :
                "Capture failed for %s arm: no fresh animated wrist or target bone available.";
            xr_sprintf(msg, sizeof(msg), ok ? "Captured %s animated wrist into the target." : failure, armName);
        }
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
        return HudIKBoneName(skeleton, id);
    };

    pcstr spaceName = "animated";
    if (settings.space == CHudIKController::TargetSpace::Model)
        spaceName = "model";
    else if (settings.space == CHudIKController::TargetSpace::Bone)
        spaceName = "bone";
    else if (settings.space == CHudIKController::TargetSpace::Gun)
        spaceName = "gun";
    else if (settings.space == CHudIKController::TargetSpace::GunAnimated)
        spaceName = "gun_animated";

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
    xr_sprintf(line, sizeof(line), "target_space=%s\n", spaceName);
    text += line;
    if (settings.space == CHudIKController::TargetSpace::Gun)
    {
        if (ctrl.HasExternalGun())
        {
            int weaponSlot = -1;
            pcstr weaponSection = EquippedWeaponSection(weaponSlot);
            xr_sprintf(line, sizeof(line), "target_reference=equipped weapon origin\nequipped_weapon=%s\n",
                weaponSection ? weaponSection : "<none>");
        }
        else
        {
            xr_sprintf(line, sizeof(line), "target_reference=gun bone %s\n", boneName(ctrl.GetGun().bone));
        }
    }
    else if (settings.space == CHudIKController::TargetSpace::GunAnimated)
    {
        xr_sprintf(line, sizeof(line), "target_reference=%s\n", "current animated wrist moved by gun displacement every frame");
    }
    else
    {
        xr_sprintf(line, sizeof(line), "target_bone=%s\n", boneName(settings.targetBone));
    }
    text += line;
    text += "model_space=hands/controller model space\n";
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

    if (ctrl.SupportsGunLead())
        text += BuildHudIKGunText(ctrl);

    ImGui::SetClipboardText(text.c_str());
}

pcstr COzzAnimDebugger::HudIKBoneName(IKinematics* skeleton, u16 bone)
{
    if (bone == BI_NONE)
        return "";
    if (!skeleton || bone >= skeleton->LL_BoneCount())
        return "<invalid>";
    pcstr name = skeleton->LL_BoneName_dbg(bone);
    return name ? name : "<unnamed>";
}

bool COzzAnimDebugger::IsBoneAncestorOrSelf(IKinematics* skeleton, u16 bone, u16 ancestor)
{
    const u16 count = skeleton->LL_BoneCount();
    for (u16 steps = 0; bone != BI_NONE && bone < count && steps <= count; ++steps)
    {
        if (bone == ancestor)
            return true;
        bone = skeleton->GetBoneData(bone).GetParentID();
    }
    return false;
}

bool COzzAnimDebugger::GunBoneConflictsWithArms(const CHudIKController& ctrl, u16 gunBone)
{
    IKinematics* skeleton = ctrl.Skeleton();
    if (!skeleton || gunBone == BI_NONE || gunBone >= skeleton->LL_BoneCount())
        return false;
    for (u16 arm = 0; arm < 2; ++arm)
    {
        for (u16 j = 0; j < 3; ++j)
        {
            const u16 armBone = ctrl.GetArm(arm).bones[j];
            if (armBone != BI_NONE && IsBoneAncestorOrSelf(skeleton, armBone, gunBone))
                return true;
        }
    }
    return false;
}

xr_string COzzAnimDebugger::BuildHudIKGunText(const CHudIKController& ctrl) const
{
    const CHudIKController::GunSettings& gun = ctrl.GetGun();
    pcstr spaceName = gun.space == CHudIKController::TargetSpace::Model ? "model" : "animated";

    xr_string text;
    string512 line;
    if (ctrl.HasExternalGun())
    {
        int weaponSlot = -1;
        pcstr weaponSection = EquippedWeaponSection(weaponSlot);
        xr_sprintf(line, sizeof(line), "gun_enabled=%d\ngun_reference=equipped weapon origin\nequipped_weapon=%s\ngun_target_space=%s\n",
            gun.enabled ? 1 : 0, weaponSection ? weaponSection : "<none>", spaceName);
    }
    else
    {
        xr_sprintf(line, sizeof(line), "gun_enabled=%d\ngun_bone=%s\ngun_target_space=%s\n", gun.enabled ? 1 : 0,
            HudIKBoneName(ctrl.Skeleton(), gun.bone), spaceName);
    }
    text += line;
    text += "model_space=hands/controller model space\n";
    xr_sprintf(line, sizeof(line), "gun_offset_position=%.5f, %.5f, %.5f\n", gun.position.x, gun.position.y, gun.position.z);
    text += line;
    xr_sprintf(line, sizeof(line), "gun_offset_hpb_deg=%.4f, %.4f, %.4f\n", gun.rotation.x, gun.rotation.y, gun.rotation.z);
    text += line;
    return text;
}

void COzzAnimDebugger::CopyHudIKGunSettings(const CHudIKController& ctrl) const
{
    const auto& vec = m_selectedIsHud ? m_hudTargets : m_worldTargets;
    pcstr skeletonLabel = "";
    if (m_selectedIdx >= 0 && m_selectedIdx < static_cast<int>(vec.size()))
        skeletonLabel = vec[m_selectedIdx].label.c_str();

    string512 line;
    xr_sprintf(line, sizeof(line), "hud_ik gun reference/session settings (not auto-loaded) skeleton=%s\n", skeletonLabel);
    xr_string text = line;
    text += BuildHudIKGunText(ctrl);
    ImGui::SetClipboardText(text.c_str());
}

void COzzAnimDebugger::DrawHudIKGunEditor(CHudIKController& ctrl, bool externalLayout)
{
    IKinematics* skeleton = ctrl.Skeleton();
    if (!skeleton)
        return;

    const bool supported = ctrl.SupportsGunLead();

    const u16 boneCount = skeleton->LL_BoneCount();
    const ImVec4 warnColor(1.f, 0.75f, 0.3f, 1.f);

    int weaponSlot = -1;
    pcstr weaponSection = EquippedWeaponSection(weaponSlot);

    ImGui::PushID("gun_lead");

    CHudIKController::GunSettings settings = ctrl.GetGun();
    bool canCapture = supported;

    if (externalLayout)
    {
        ImGui::TextWrapped("The equipped weapon origin is the automatic reference, expressed in hands model space. No gun bone selection is needed.");
        if (weaponSection)
            ImGui::Text("Equipped weapon: slot %d - %s", weaponSlot, weaponSection);
        else
            ImGui::TextDisabled("Equipped weapon: none");
        ImGui::LabelText("Gun reference", "Equipped weapon origin (automatic)");
        if (!supported)
        {
            ImGui::TextColored(warnColor, "%s", weaponSection ?
                "Gun controls are disabled until the equipped weapon is configured on the hands controller." :
                "Gun controls are disabled: no weapon is equipped in slot 0.");
        }
    }
    else
    {
        ImGui::TextWrapped("Select the whole weapon branch root, not a hand, an arm bone or a common root shared with the arms. The gun may be a descendant of a hand but must not be an ancestor of, or the same bone as, any bone in either arm chain. Activation keeps both animated wrists and moves both hands by the gun target displacement.");

        if (supported && !ctrl.HasGunBoneHint())
            ImGui::TextColored(warnColor, "This weapon skeleton has no fire-point bone hint, so automatic gun bone suggestion is unavailable. You can still pick the weapon branch root manually.");

        if (DrawBonePicker("Gun bone", skeleton, settings.bone) && supported)
        {
            ctrl.SetGun(settings);
            settings = ctrl.GetGun();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!supported || !ctrl.HasGunBoneHint());
        const bool suggestPressed = ImGui::Button("Suggest gun bone");
        ImGui::EndDisabled();
        if (suggestPressed && supported)
        {
            const u16 suggested = ctrl.SuggestGunBone();
            settings = ctrl.GetGun();
            if (suggested == BI_NONE || suggested >= boneCount)
            {
                m_ikGunMessage = "No gun bone could be suggested. Pick the weapon branch root manually.";
            }
            else
            {
                settings.bone = suggested;
                ctrl.SetGun(settings);
                string256 msg;
                xr_sprintf(msg, sizeof(msg), "Suggested gun bone: %s. Verify it is the whole weapon branch before activating.",
                    HudIKBoneName(skeleton, suggested));
                m_ikGunMessage = msg;
            }
            settings = ctrl.GetGun();
        }

        const bool boneValid = supported && settings.bone != BI_NONE && settings.bone < boneCount;
        const u16 automaticBone = settings.bone == BI_NONE ? ctrl.SuggestGunBone() : u16(BI_NONE);
        canCapture = boneValid || (supported && automaticBone < boneCount);
        if (supported && !boneValid && canCapture)
            ImGui::Text("Automatic weapon branch: %s", HudIKBoneName(skeleton, automaticBone));
        else if (supported && !boneValid)
            ImGui::TextColored(warnColor, "Pick a gun bone to enable activation and capture.");
        else if (supported && GunBoneConflictsWithArms(ctrl, settings.bone))
            ImGui::TextColored(warnColor, "This gun bone is an ancestor of, or equal to, a bone in an arm chain. Pick the weapon branch itself.");
    }

    bool armBonesMapped = true;
    for (u16 arm = 0; arm < 2; ++arm)
    {
        for (u16 j = 0; j < 3; ++j)
            armBonesMapped &= ctrl.GetArm(arm).bones[j] != BI_NONE;
    }
    if (!armBonesMapped)
        ImGui::TextColored(warnColor, "An arm chain is not fully mapped. Assign upper arm, forearm and hand bones for both arms in Arm authoring.");

    ImGui::BeginDisabled(!canCapture);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.5f, 0.2f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.62f, 0.28f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.42f, 0.17f, 1.f));
    const bool activatePressed = ImGui::Button("Activate animation-follow IK");
    ImGui::PopStyleColor(3);
    if (activatePressed && supported)
    {
        const bool ok = ctrl.ActivateTwoHand();
        settings = ctrl.GetGun();
        if (ok)
        {
            string1024 msg;
            if (externalLayout)
            {
                xr_sprintf(msg, sizeof(msg),
                    "Animation-follow IK active for slot %d: %s. Both arms enabled, weight 1, zero offsets.",
                    weaponSlot, weaponSection ? weaponSection : "<none>");
            }
            else
            {
                xr_sprintf(msg, sizeof(msg),
                    "Animation-follow IK active for gun bone %s. Both arms enabled, weight 1, zero offsets.",
                    HudIKBoneName(skeleton, settings.bone));
            }
            m_ikGunMessage = msg;
        }
        else
        {
            if (externalLayout)
                m_ikGunMessage = "Animation-follow activation failed and no settings were changed. Map both arm chains (upper arm, forearm, hand), keep a weapon equipped in slot 0 and wait for a fresh animated pose.";
            else
                m_ikGunMessage = "Animation-follow activation failed and no settings were changed. Map both arm chains (upper arm, forearm, hand), wait for a fresh animated pose, and check that the gun bone is the whole weapon branch and not an ancestor of or equal to either arm chain.";
            m_ikGunMessage += " Reason: ";
            m_ikGunMessage += ctrl.GetGunCaptureStatus();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Release gun-led IK") && supported)
    {
        ctrl.ReleaseTwoHand();
        m_ikGunMessage = "Released gun-led IK: gun target and both arms disabled. All values were kept.";
    }

    settings = ctrl.GetGun();

    bool changed = false;

    const bool gunReady = externalLayout ? supported : settings.bone != BI_NONE;
    ImGui::BeginDisabled(!gunReady && !settings.enabled);
    changed |= ImGui::Checkbox("Gun target override only", &settings.enabled);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("(does not enable the hands; use Activate / release above)");

    const pcstr externalSpaceLabels[2] = {
        "Animated (offset from raw animated weapon origin)",
        "Model (absolute hands model space)",
    };
    const pcstr boneSpaceLabels[2] = {
        "Animated (offset from raw animated gun)",
        "Model (absolute controller model space)",
    };
    int space = settings.space == CHudIKController::TargetSpace::Model ? 1 : 0;
    if (ImGui::Combo("Gun target space", &space, externalLayout ? externalSpaceLabels : boneSpaceLabels, 2))
    {
        settings.space = space ? CHudIKController::TargetSpace::Model : CHudIKController::TargetSpace::Animated;
        changed = true;
    }
    ImGui::TextDisabled("Changing the space keeps the numbers; capture restores the raw animated gun baseline in the chosen space. Bone space is not supported for the gun.");

    changed |= ImGui::DragFloat3("Gun translation (m)", &settings.position.x, 0.001f, 0.f, 0.f, "%.4f");
    changed |= ImGui::DragFloat3("Gun rotation H/P/B (deg)", &settings.rotation.x, 0.25f, 0.f, 0.f, "%.2f");

    if (changed && supported)
    {
        ctrl.SetGun(settings);
        settings = ctrl.GetGun();
    }

    ImGui::BeginDisabled(!canCapture);
    char captureLabel[96];
    xr_sprintf(captureLabel, sizeof(captureLabel), "Capture gun target (%s space)",
        settings.space == CHudIKController::TargetSpace::Model ? "Model" : "Animated");
    if (ImGui::Button(captureLabel) && supported)
    {
        const bool ok = ctrl.CaptureGunTarget(settings.space);
        m_ikGunMessage = ok ? "Captured the raw animated gun as the gun target baseline." :
            (externalLayout ? "Gun capture failed: no fresh animated pose or equipped weapon available." :
                              "Gun capture failed: no fresh animated pose or valid gun bone available.");
        if (!ok)
        {
            m_ikGunMessage += " Reason: ";
            m_ikGunMessage += ctrl.GetGunCaptureStatus();
        }
        settings = ctrl.GetGun();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Copy gun settings"))
    {
        CopyHudIKGunSettings(ctrl);
        m_ikGunMessage = "Copied gun settings to the clipboard.";
    }

    if (!m_ikGunMessage.empty())
        ImGui::TextWrapped("%s", m_ikGunMessage.c_str());

    DrawHudIKGunDiagnostics(ctrl);

    ImGui::PopID();
}

void COzzAnimDebugger::DrawHudIKGunDiagnostics(const CHudIKController& ctrl)
{
    const CHudIKController::GunSettings& gun = ctrl.GetGun();
    const CHudIKController::GunState& state = ctrl.GetGunState();
    const u32 age = Device.dwFrame >= state.frame ? Device.dwFrame - state.frame : 0;

    ImVec4 color(0.6f, 0.6f, 0.6f, 1.f);
    const bool gunPassthrough = state.status && strcmp(state.status, "animation_passthrough") == 0;
    if (gun.enabled)
        color = (state.applied || gunPassthrough) ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.7f, 0.3f, 1.f);
    ImGui::TextColored(color, "Gun status: %s", state.status ? state.status : "?");
    ImGui::SameLine();
    ImGui::Text("valid %s / applied %s", state.valid ? "yes" : "no", state.applied ? "yes" : "no");
    if (gunPassthrough)
        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "No gun displacement; native animation preserved.");
    if (gun.enabled && age > 2)
        ImGui::TextColored(ImVec4(1.f, 0.7f, 0.3f, 1.f), "Gun frame %u (stale, age %u)", state.frame, age);
    else
        ImGui::Text("Gun frame %u (age %u)", state.frame, age);

    if (!ImGui::BeginTable("##hud_ik_gun_diag", 2,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        return;

    ImGui::TableSetupColumn("Gun pose");
    ImGui::TableSetupColumn(ctrl.HasExternalGun() ? "Position (hands model m)" : "Position (controller model m)");
    ImGui::TableHeadersRow();

    auto row = [](pcstr label, const Fvector& p)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("%.3f %.3f %.3f", p.x, p.y, p.z);
    };

    row("Raw animated gun", state.animated.c);
    row("Target gun (planned)", state.target.c);
    row("Resolved gun", state.resolved.c);
    row("Delta (target vs raw)", state.delta.c);

    ImGui::EndTable();
}

bool COzzAnimDebugger::IsFiniteVector(const Fvector& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

void COzzAnimDebugger::DrawHudWeaponCollision(HudTargetKind kind, int slot)
{
    ImGui::SeparatorText("Primary weapon collision (slot 0, static level)");

    const bool primaryScope = kind == HudTargetKind::Hands ||
        ((kind == HudTargetKind::SeparateItem || kind == HudTargetKind::MonolithicItem) && slot == 0);
    if (!primaryScope)
    {
        ImGui::TextDisabled("Weapon collision applies to the primary weapon (slot 0) only. Select [hands] or the primary weapon to edit it.");
        return;
    }

    if (!g_player_hud)
        return;

    ImGui::PushID("weapon_collision");

    ImGui::TextWrapped("Scope: the primary weapon (slot 0) against static level geometry only. It keeps the original animation probe untouched. The collision correction becomes a translation of the gun target and the arms follow through gun IK. It does not change bullet aim or fire direction.");

    if (!g_player_hud->attached_item(0))
        ImGui::TextDisabled("No weapon is equipped in primary slot 0; values below are stale or unavailable.");

    CHudWeaponCollision& collision = g_player_hud->weapon_collision();
    DrawHudWeaponCollisionSettings(collision);
    DrawHudWeaponCollisionState(collision);

    ImGui::PopID();
}

void COzzAnimDebugger::DrawHudWeaponCollisionSettings(CHudWeaponCollision& collision)
{
    const CHudWeaponCollision::Settings defaults;
    CHudWeaponCollision::Settings settings = collision.GetSettings();

    auto sanitize = [](float value, float fallback, float lo, float hi)
    {
        return std::isfinite(value) ? std::clamp(value, lo, hi) : fallback;
    };

    auto clampAll = [&]()
    {
        settings.radius = sanitize(settings.radius, defaults.radius, 0.005f, 0.15f);
        settings.forwardOffset = sanitize(settings.forwardOffset, defaults.forwardOffset, 0.f, 0.3f);
        settings.maxPush = sanitize(settings.maxPush, defaults.maxPush, 0.f, 1.f);
        settings.anticipation = sanitize(settings.anticipation, defaults.anticipation, 0.f, 0.1f);
        settings.contactSeconds = sanitize(settings.contactSeconds, defaults.contactSeconds, 0.01f, 1.f);
        settings.releaseSeconds = sanitize(settings.releaseSeconds, defaults.releaseSeconds, 0.01f, 2.f);
    };
    clampAll();

    bool changed = false;
    changed |= ImGui::Checkbox("Collision enabled", &settings.enabled);
    changed |= ImGui::Checkbox("Easing enabled (off = hard instant push)", &settings.easing);
    changed |= ImGui::SliderFloat("Sphere radius (m)", &settings.radius, 0.005f, 0.15f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::SliderFloat("Projection ahead of muzzle (m)", &settings.forwardOffset, 0.f, 0.3f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::SliderFloat("Max push (m)", &settings.maxPush, 0.f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::SliderFloat("Anticipation margin (m)", &settings.anticipation, 0.f, 0.1f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::SliderFloat("Contact response (s)", &settings.contactSeconds, 0.01f, 1.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    changed |= ImGui::SliderFloat("Release response (s)", &settings.releaseSeconds, 0.01f, 2.f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextDisabled("Anticipation starts the push earlier and adds that much wall clearance while in contact. Contact/release seconds are critically damped response constants, not exact completion deadlines.");

    if (ImGui::Button("Reset collision defaults"))
    {
        settings = defaults;
        changed = true;
    }

    if (changed)
    {
        clampAll();
        collision.SetSettings(settings);
    }
}

void COzzAnimDebugger::DrawHudWeaponCollisionState(const CHudWeaponCollision& collision)
{
    const CHudWeaponCollision::Settings& settings = collision.GetSettings();
    const CHudWeaponCollision::State& state = collision.GetState();
    const u32 age = Device.dwFrame >= state.frame ? Device.dwFrame - state.frame : 0;
    const ImVec4 okColor(0.4f, 1.f, 0.4f, 1.f);
    const ImVec4 warnColor(1.f, 0.7f, 0.3f, 1.f);
    const ImVec4 offColor(0.6f, 0.6f, 0.6f, 1.f);

    pcstr status = state.status ? state.status : "inactive";
    const bool statusInactive = strcmp(status, "inactive") == 0 || strcmp(status, "disabled") == 0;
    const bool noMuzzle = !state.original.valid && !state.requested.valid && !state.resolved.valid;
    const bool allValid = state.original.valid && state.requested.valid && state.resolved.valid;

    ImVec4 color = warnColor;
    if (statusInactive || !settings.enabled)
        color = offColor;
    else if (strcmp(status, "clear") == 0 && allValid && !state.hit && !state.clamped)
        color = okColor;
    ImGui::TextColored(color, "Collision status: %s", status);

    if (strcmp(status, "inactive") == 0 && noMuzzle)
        ImGui::TextDisabled("Collision frame: none evaluated");
    else if (settings.enabled && age > 2)
        ImGui::TextColored(warnColor, "Collision frame %u (stale, age %u)", state.frame, age);
    else
        ImGui::Text("Collision frame %u (age %u)", state.frame, age);

    ImGui::Text("Candidate triangles: %u", state.candidates);
    ImGui::Text("Hit: %s / push clamped: %s / anticipating: %s", state.hit ? "yes" : "no", state.clamped ? "yes" : "no",
        state.anticipated ? "yes" : "no");

    if (state.hit || state.anticipated)
    {
        if (state.triangle >= 0)
            ImGui::Text("Contact triangle: %d", static_cast<int>(state.triangle));
        else
            ImGui::TextDisabled("Contact triangle: unavailable");

        if (IsFiniteVector(state.contact))
            ImGui::Text("Contact (world m): %.3f %.3f %.3f", state.contact.x, state.contact.y, state.contact.z);
        else
            ImGui::TextDisabled("Contact: unavailable");

        if (IsFiniteVector(state.normal))
            ImGui::Text("Normal (world): %.3f %.3f %.3f", state.normal.x, state.normal.y, state.normal.z);
        else
            ImGui::TextDisabled("Normal: unavailable");

        if (std::isfinite(state.penetration))
            ImGui::Text("Penetration depth: %.4f m", state.penetration);
        else
            ImGui::TextDisabled("Penetration depth: unavailable");
    }
    else
    {
        ImGui::TextDisabled("Contact, normal, penetration and triangle: unavailable (no hit)");
    }

    if (state.requested.valid && IsFiniteVector(state.hardCorrection) && IsFiniteVector(state.targetCorrection) &&
        IsFiniteVector(state.correction) && IsFiniteVector(state.velocity))
    {
        ImGui::Text("Hard desired (world m): |%.4f|  (%.4f %.4f %.4f)", state.hardCorrection.magnitude(),
            state.hardCorrection.x, state.hardCorrection.y, state.hardCorrection.z);
        ImGui::Text("Eased target (anticipation): |%.4f|  (%.4f %.4f %.4f)", state.targetCorrection.magnitude(),
            state.targetCorrection.x, state.targetCorrection.y, state.targetCorrection.z);
        ImGui::Text("Solver output (safety-projected): |%.4f|  (%.4f %.4f %.4f)", state.correction.magnitude(),
            state.correction.x, state.correction.y, state.correction.z);
        ImGui::Text("Spring velocity: |%.4f| m/s / frame dt %.4f s%s", state.velocity.magnitude(), state.deltaTime,
            state.restarted ? " / restarted" : "");
        if (state.safety)
            ImGui::TextColored(warnColor, "Safety override: pushed %.4f m beyond eased candidate", state.safetyPush);
        else
            ImGui::TextDisabled("Safety override: none");
    }
    else
    {
        ImGui::TextDisabled("Hard / eased / solver correction: unavailable");
    }

    if (state.requested.valid && state.resolved.valid && IsFiniteVector(state.requested.sphereCenter) &&
        IsFiniteVector(state.resolved.sphereCenter))
    {
        Fvector actual;
        actual.sub(state.resolved.sphereCenter, state.requested.sphereCenter);
        ImGui::Text("Actual pose displacement (world m): |%.4f|  (%.4f %.4f %.4f)", actual.magnitude(),
            actual.x, actual.y, actual.z);
    }
    else
    {
        ImGui::TextDisabled("Actual pose displacement: unavailable");
    }

    ImGui::TextDisabled("Positions, sphere centers and corrections are perceived world meters; fire directions and normals are world unit vectors. Nothing here is HUD model space or drawn in the model-space plot.");

    if (!ImGui::BeginTable("##hud_weapon_collision_muzzle", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
        return;

    ImGui::TableSetupColumn("Muzzle (world)");
    ImGui::TableSetupColumn("Fire point");
    ImGui::TableSetupColumn("Fire direction");
    ImGui::TableSetupColumn("Sphere center");
    ImGui::TableHeadersRow();

    auto cell = [](bool valid, const Fvector& v)
    {
        if (!valid)
        {
            ImGui::TextDisabled("unavailable");
            return;
        }
        if (!IsFiniteVector(v))
        {
            ImGui::TextDisabled("non-finite");
            return;
        }
        ImGui::Text("%.3f %.3f %.3f", v.x, v.y, v.z);
    };

    auto row = [&cell](pcstr label, const CHudWeaponCollision::Muzzle& muzzle)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(label);
        ImGui::TableSetColumnIndex(1);
        cell(muzzle.valid, muzzle.firePoint);
        ImGui::TableSetColumnIndex(2);
        cell(muzzle.valid, muzzle.direction);
        ImGui::TableSetColumnIndex(3);
        cell(muzzle.valid, muzzle.sphereCenter);
    };

    row("Original (no IK)", state.original);
    row("Requested (authored IK, no collision)", state.requested);
    row("Resolved (final IK + collision)", state.resolved);

    ImGui::EndTable();
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
        {
            const bool passthrough = s.status && strcmp(s.status, "animation_passthrough") == 0;
            color = (s.solved || passthrough) ? ImVec4(0.4f, 1.f, 0.4f, 1.f) : ImVec4(1.f, 0.7f, 0.3f, 1.f);
        }
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

    for (u16 a = 0; a < 2; ++a)
    {
        const CHudIKController::ArmState& s = ctrl.GetState(a);
        if (ctrl.GetArm(a).enabled && s.status && strcmp(s.status, "animation_passthrough") == 0)
            ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "%s arm: no procedural displacement; native animation preserved (IK intentionally skipped, solved stays no).", a ? "Right" : "Left");
    }
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

    const CHudIKController::GunState& gunState = ctrl.GetGunState();
    const bool gunDraw = ctrl.SupportsGunLead() && gunState.valid;
    const bool gunAnimOk = gunDraw && okPoint(gunState.animated.c);
    const bool gunTargetOk = gunDraw && okPoint(gunState.target.c);
    if (gunAnimOk)
        include(gunState.animated.c);
    else if (gunDraw)
        ++omitted;
    if (gunTargetOk)
        include(gunState.target.c);
    else if (gunDraw)
        ++omitted;

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

    if (gunAnimOk && m_ikShowAnimated)
    {
        const ImVec2 gp = project(gunState.animated.c);
        const ImU32 gunDim = IM_COL32(230, 90, 255, 110);
        dl->AddCircle(gp, 6.f, gunDim, 16, 1.5f);
        dl->AddText(ImVec2(gp.x + 9.f, gp.y + 3.f), gunDim, "G anim");
    }
    if (gunTargetOk && m_ikShowDesired)
    {
        const ImVec2 gp = project(gunState.target.c);
        const ImU32 gunColor = IM_COL32(230, 90, 255, 255);
        if (gunAnimOk && m_ikShowAnimated)
            dl->AddLine(project(gunState.animated.c), gp, IM_COL32(230, 90, 255, 140), 1.f);
        dl->AddLine(ImVec2(gp.x - 9.f, gp.y), ImVec2(gp.x + 9.f, gp.y), gunColor, 2.f);
        dl->AddLine(ImVec2(gp.x, gp.y - 9.f), ImVec2(gp.x, gp.y + 9.f), gunColor, 2.f);
        dl->AddCircle(gp, 7.f, gunColor, 16, 2.f);
        dl->AddText(ImVec2(gp.x + 10.f, gp.y - 16.f), gunColor, "G target");
    }

    dl->PopClipRect();
    dl->AddRect(p0, p1, IM_COL32(90, 95, 110, 255));
}
