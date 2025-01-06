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

#include "xrCommon/xr_list.h"

class CUIWindow;

enum class FocusDirection : u8
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

// Doesn't own CUIWindow* pointers it holds
class XRUICORE_API CUIFocusSystem
{
    xr_list<const CUIWindow*> m_valuable;
    xr_list<const CUIWindow*> m_non_valuable;

    const CUIWindow* m_current_focused{};

public:
    virtual ~CUIFocusSystem() = default;

    void RegisterFocusable(const CUIWindow* focusable);
    void UnregisterFocusable(const CUIWindow* focusable);
    bool IsRegistered(const CUIWindow* focusable) const;

    void Update(const CUIWindow* root);

    auto GetFocused() const { return const_cast<CUIWindow*>(m_current_focused);}
    void SetFocused(const CUIWindow* window) { m_current_focused = window; }

    std::pair<CUIWindow*, bool> FindClosestFocusable(const Fvector2& from, FocusDirection direction) const;
};
