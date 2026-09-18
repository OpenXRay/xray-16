#include "StdAfx.h"
#include "ozz_anim_debugger.h"
#include "player_hud.h"

#include "xrEngine/device.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/xr_object_list.h"

#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/KinematicsAnimated.h"

#include <imgui.h>
#include <algorithm>

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
    }
    ImGui::EndChild();

    ImGui::End();
}
