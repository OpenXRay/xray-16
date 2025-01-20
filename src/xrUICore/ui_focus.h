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
class XRUICORE_API CUIFocusSystem : public CUIDebuggable
{
    xr_list<const CUIWindow*> m_valuable;
    xr_list<const CUIWindow*> m_non_valuable;

    const CUIWindow* m_current_focused{};
    const CUIWindow* m_focus_locker{};

public:
    virtual ~CUIFocusSystem() = default;

    void RegisterFocusable(const CUIWindow* focusable);
    void UnregisterFocusable(const CUIWindow* focusable);

    bool IsRegistered(const CUIWindow* focusable) const;
    bool IsValuable(const CUIWindow* focusable) const;
    bool IsNonValuable(const CUIWindow* focusable) const;

    void Update(const CUIWindow* root);

    // Make locker's children the only valuable
    void LockToWindow(const CUIWindow* locker) { m_focus_locker = locker; }
    void Unlock() { LockToWindow(nullptr); }
    auto GetLocker() const { return const_cast<CUIWindow*>(m_focus_locker); }

    auto GetFocused() const { return const_cast<CUIWindow*>(m_current_focused);}
    void SetFocused(const CUIWindow* window);

    std::pair<CUIWindow*, CUIWindow*> FindClosestFocusable(const Fvector2& from, FocusDirection direction) const;

    pcstr GetDebugType() override { return "CUIFocusSystem"; }
    bool FillDebugTree(const CUIDebugState& debugState) override;
    void FillDebugInfo() override;

    void DrawDebugInfo(const CUIWindow& from, const CUIWindow& to, u32 color, u32 textColor) const;
};
