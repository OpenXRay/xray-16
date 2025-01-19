/*
Copyright (c) 2014 OpenXRay

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "pch.hpp"

#include "ui_focus.h"
#include "Windows/UIWindow.h"
#include "Cursor/UICursor.h"

#include "xrCore/buffer_vector.h"

#include <array>

namespace
{
std::array<FocusDirection, 3> allowed_directions(FocusDirection direction)
{
    switch (direction)
    {
    case FocusDirection::Up:
        return { FocusDirection::UpperLeft, FocusDirection::Up, FocusDirection::UpperRight };

    default:
    case FocusDirection::Same:
        // Normally, you should not request same direction.
        VERIFY(false);

        // Down would be the most natural default direction
        // With the semantical meaning "enter into something"
        [[fallthrough]];

    case FocusDirection::Down:
        return { FocusDirection::LowerLeft, FocusDirection::Down, FocusDirection::LowerRight };

    case FocusDirection::Left:
        return { FocusDirection::UpperLeft, FocusDirection::Left, FocusDirection::LowerLeft };

    case FocusDirection::Right:
        return { FocusDirection::UpperRight, FocusDirection::Right, FocusDirection::LowerRight };

    case FocusDirection::UpperLeft:
        return { FocusDirection::Up, FocusDirection::UpperLeft, FocusDirection::Left };

    case FocusDirection::UpperRight:
        return { FocusDirection::Up, FocusDirection::UpperRight, FocusDirection::Right };

    case FocusDirection::LowerLeft:
        return { FocusDirection::Left, FocusDirection::LowerLeft, FocusDirection::Down };

    case FocusDirection::LowerRight:
        return { FocusDirection::Right, FocusDirection::LowerRight, FocusDirection::Down };
    } // switch (direction)
}

float get_distance(const Fvector2& a, const Fvector2& b)
{
    Fvector2 c;
    c.sub(b, a);
    return c.dotproduct(c);
}

FocusDirection get_focus_direction(const Fvector2& a, const Fvector2& b)
{
    if (a.similar(b, EPS_S))
        return FocusDirection::Same;

    Fvector2 delta;
    delta.sub(b, a);

    if (fis_zero(delta.y)) // same y
    {
        if (delta.x < 0)
            return FocusDirection::Left;

        return FocusDirection::Right;
    }
    if (delta.y < 0) // 'to' is above
    {
        if (fis_zero(delta.x)) // same x
            return FocusDirection::Up;
        if (delta.x < 0)
            return FocusDirection::UpperLeft;

        return FocusDirection::UpperRight;
    }

    // 'to' is below
    if (fis_zero(delta.x)) // same x
        return FocusDirection::Down;
    if (delta.x < 0)
        return FocusDirection::LowerLeft;

    return FocusDirection::LowerRight;
}
} // namespace

void CUIFocusSystem::RegisterFocusable(const CUIWindow* focusable)
{
    if (!focusable || IsRegistered(focusable))
        return;

    m_non_valuable.emplace_back(focusable);
}

void CUIFocusSystem::UnregisterFocusable(const CUIWindow* focusable)
{
    if (!focusable)
        return;

    if (m_current_focused == focusable)
        m_current_focused = nullptr;

    if (const auto it = std::find(m_valuable.begin(), m_valuable.end(), focusable);
        it != m_valuable.end())
    {
        m_valuable.erase(it);
    }

    if (const auto it = std::find(m_non_valuable.begin(), m_non_valuable.end(), focusable);
        it != m_non_valuable.end())
    {
        m_non_valuable.erase(it);
    }
}

bool CUIFocusSystem::IsRegistered(const CUIWindow* focusable) const
{
    return IsValuable(focusable) || IsNonValuable(focusable);
}

bool CUIFocusSystem::IsValuable(const CUIWindow* focusable) const
{
    if (!focusable)
        return false;
    const auto it = std::find(m_valuable.begin(), m_valuable.end(), focusable);
    return it != m_valuable.end();
}

bool CUIFocusSystem::IsNonValuable(const CUIWindow* focusable) const
{
    if (!focusable)
        return false;
    const auto it = std::find(m_non_valuable.begin(), m_non_valuable.end(), focusable);
    return it != m_non_valuable.end();

}

void CUIFocusSystem::Update(const CUIWindow* root)
{
    // temp vector allows to prevent calling for IsFocusValuable twice.
    buffer_vector<const CUIWindow*> temp{ xr_alloca(sizeof(CUIWindow*) * m_valuable.size()), m_valuable.size() };

    for (auto it = m_valuable.begin(); it != m_valuable.end(); ++it)
    {
        if ((*it)->IsFocusValuable(root, m_focus_locker))
            continue;
        temp.push_back(*it);
        it = m_valuable.erase(it);

        if (*it == m_current_focused)
            m_current_focused = nullptr;
    }

    for (auto it = m_non_valuable.begin(); it != m_non_valuable.end(); ++it)
    {
        if (!(*it)->IsFocusValuable(root, m_focus_locker))
            continue;
        m_valuable.emplace_back(*it);
        it = m_non_valuable.erase(it);
    }

    for (const auto window : temp)
        m_non_valuable.emplace_back(window);

    // no need to clear temp vector, it's stack allocated.

    if (m_current_focused && !m_current_focused->CursorOverWindow())
        m_current_focused = nullptr;
}

void CUIFocusSystem::SetFocused(const CUIWindow* window)
{
    m_current_focused = window;
    UI().GetUICursor().WarpToWindow(window);
}

std::pair<CUIWindow*, CUIWindow*> CUIFocusSystem::FindClosestFocusable(const Fvector2& from, FocusDirection direction) const
{
    const CUIWindow* closest  = nullptr;
    const CUIWindow* closest2 = nullptr;
    float min_distance  = type_max<float>;
    float min_distance2 = type_max<float>;

    const auto [dir1, mainDir, dir2] = allowed_directions(direction);

    for (const auto& window : m_valuable)
    {
        const auto to_pos = window->GetAbsoluteCenterPos();

        const auto dist = get_distance(from, to_pos);
        const auto dir  = get_focus_direction(from, to_pos);

        if (dist < min_distance && dir == mainDir)
        {
            min_distance = dist;
            closest = window;
        }
        if (dist < min_distance2 && (dir == dir1 || dir == dir2))
        {
            min_distance2 = dist;
            closest2 = window;
        }
    }

    // We hold const pointers to guarantee that we don't do anything.
    // But the caller can do anything.
    return
    {
        const_cast<CUIWindow*>(closest),
        const_cast<CUIWindow*>(closest2)
    };
}

bool CUIFocusSystem::FillDebugTree(const CUIDebugState& debugState)
{
#ifndef MASTER_GOLD
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
    if (debugState.selected == this)
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool open = ImGui::TreeNodeEx(this, flags, "Focus system (%s)", GetDebugType());
    if (ImGui::IsItemClicked())
        debugState.select(this);

    if (open)
    {
        if (m_valuable.empty())
            ImGui::BulletText("Valuable: 0");
        else
        {
            if (ImGui::TreeNode(&m_valuable, "Valuable: %zu", m_valuable.size()))
            {
                for (auto& window : m_valuable)
                    const_cast<CUIWindow*>(window)->FillDebugTree(debugState);
                ImGui::TreePop();
            }
        }

        if (m_non_valuable.empty())
            ImGui::BulletText("Non valuable: 0");
        else
        {
            if (ImGui::TreeNode(&m_non_valuable, "Valuable: %zu", m_non_valuable.size()))
            {
                for (auto& window : m_non_valuable)
                    const_cast<CUIWindow*>(window)->FillDebugTree(debugState);
                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    return open;
#else
    return true;
#endif
}
void CUIFocusSystem::FillDebugInfo()
{
#ifndef MASTER_GOLD
    if (!ImGui::CollapsingHeader(CUIFocusSystem::GetDebugType()))
        return;

    ImGui::LabelText("Current focused", "%s", m_current_focused ? m_current_focused->WindowName().c_str() : "none");
    ImGui::LabelText("Locker", "%s", m_focus_locker ? m_focus_locker->WindowName().c_str() : "none");
#endif
}
