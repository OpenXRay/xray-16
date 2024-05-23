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

#pragma once

#include "ui_defs.h"

#include "xrCommon/xr_map.h"
#include "xrCommon/xr_unordered_map.h"

class CUIWindow;

enum class FocusDirection
{
    Same, // exactly same coordinates with the target
    Up,
    Down,
    Left,
    Right,
    UpperLeft,
    UpperRight,
    LowerLeft,
    LowerRight,
};

// 1. Doesn't own CUIWindow* pointers it holds
class XRUICORE_API CUIFocusSystem
{
    struct FocusData
    {
        float distance;
        FocusDirection direction;
    };

    using relates_t = xr_map<CUIWindow*, FocusData>;
    using targets_t = xr_unordered_map<CUIWindow*, relates_t>;

    targets_t m_structure;

    xr_list<CUIWindow*> m_all_windows; // helper to make code simpler

    CUIWindow* m_current_focused{};

private:
    static FocusData CalculateFocusData(const CUIWindow* from, const CUIWindow* to);

public:
    virtual ~CUIFocusSystem() = default;

    void RegisterFocusable(CUIWindow* focusable);
    void UnregisterFocusable(CUIWindow* focusable);

    bool IsRegistered(const CUIWindow* focusable) const;

    CUIWindow* FindClosestFocusable(CUIWindow* target, FocusDirection direction) const;

    CUIWindow* GetFocused() const { return m_current_focused;}
    void SetFocused(CUIWindow* window) { m_current_focused = window; }
};
