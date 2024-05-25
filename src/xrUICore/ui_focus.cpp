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

#include <cmath>

namespace
{
float euclidean_distance(const CUIWindow* from, const CUIWindow* to)
{
    const auto& [aX, aY] = from->GetWndPos();
    const auto& [bX, bY] = to->GetWndPos();

    const auto& [aWidth, aHeight] = from->GetWndSize();
    const auto& [bWidth, bHeight] = to->GetWndSize();

    const Fvector2 centerA = { aX + aWidth / 2.0f, aY + aHeight / 2.0f };
    const Fvector2 centerB = { bX + bWidth / 2.0f, bY + bHeight / 2.0f };

    return sqrtf(powf(centerB.x - centerA.x, 2) + powf(centerB.y - centerA.y, 2));
}
}

CUIFocusSystem::FocusData CUIFocusSystem::CalculateFocusData(const CUIWindow* from, const CUIWindow* to)
{
    const auto& fromPos = from->GetWndPos();
    const auto& toPos = to->GetWndPos();

    const bool upper = fromPos.y > toPos.y;
    const bool lower = fromPos.y < toPos.y;
    const bool left  = fromPos.x < toPos.x;
    const bool right = fromPos.x > toPos.x;

    FocusDirection direction;
    if (upper)
    {
        if (left)
            direction = FocusDirection::UpperLeft;
        else if (right)
            direction = FocusDirection::UpperRight;
        else
            direction = FocusDirection::Up;
    }
    else if (lower)
    {
        if (left)
            direction = FocusDirection::LowerLeft;
        else if (right)
            direction = FocusDirection::LowerRight;
        else
            direction = FocusDirection::Down;
    }
    else if (left)
    {
        direction = FocusDirection::Left;
    }
    else if (right)
    {
        direction = FocusDirection::Right;
    }
    else
    {
        direction = FocusDirection::Same;
    }

    return
    {
        euclidean_distance(from, to),
        direction
    };
}

void CUIFocusSystem::RegisterFocusable(CUIWindow* focusable)
{
    ZoneScoped;

    auto& my_relates = m_structure[focusable];

    // Split calculations to make it more CPU-cache friendly:
    // 1. Calculate relations from focusable to all windows
    for (auto& window : m_all_windows)
        my_relates[window] = CalculateFocusData(focusable, window);

    // 2. Calculate relations from all windows to focusable
    for (auto& window : m_all_windows)
        m_structure[window][focusable] = CalculateFocusData(window, focusable);

    m_all_windows.emplace_back(focusable);
}

void CUIFocusSystem::UnregisterFocusable(CUIWindow* focusable)
{
    if (const auto it = m_structure.find(focusable);
        it != m_structure.end())
    {
        it->second.clear();
        m_structure.erase(it);
    }

    if (const auto it = std::find(m_all_windows.begin(), m_all_windows.end(), focusable);
        it != m_all_windows.end())
    {
        m_all_windows.erase(it);
    }
}

bool CUIFocusSystem::IsRegistered(const CUIWindow* focusable) const
{
    const auto& it = m_structure.find(const_cast<CUIWindow*>(focusable));
    return it != m_structure.end();
}

CUIWindow* CUIFocusSystem::FindClosestFocusable(CUIWindow* target, FocusDirection direction) const
{
    const auto& it = m_structure.find(target);
    if (it == m_structure.end())
    {
        VERIFY2(false, "Target CUIWindow is not registered in the focus system.");
        return nullptr;
    }

    const auto& my_relates = it->second;
    CUIWindow* closest = nullptr;
    float minDistance = type_max<float>;

    for (const auto& [window, data] : my_relates)
    {
        if (data.distance < minDistance && data.direction == direction)
        {
            minDistance = data.distance;
            closest = window;
        }
    }

    // We hold const pointers to guarantee that we don't do anything.
    // But the caller can do anything.
    return closest;
}
