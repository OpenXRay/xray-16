#pragma once

struct CUIDebugState;

class XR_NOVTABLE XRUICORE_API CUIDebuggable
{
public:
    virtual ~CUIDebuggable();

    void RegisterDebuggable();
    void UnregisterDebuggable();

    virtual pcstr GetDebugType() = 0;

    virtual bool FillDebugTree(const CUIDebugState& debugState) = 0;
    virtual void FillDebugInfo() = 0;
};

inline pcstr CUIDebuggable::GetDebugType() { return "CUIDebuggable"; }

struct CUIDebuggerSettings
{
    struct
    {
        // Just a window
        u32 normal;
        // Just a window hovered by in-game cursor
        u32 normalHovered;
        // Any window that is hovered in the ImGui UI Debugger window
        u32 examined;
        // Window selected in the focus system
        u32 focused;
        // Window that is currently valuable in the focus system
        u32 focusableValuable;
        // Valuable window hovered by in-game cursor
        u32 focusableValuableHovered;
        // Window that is currently non-valuable in the focus system
        u32 focusableNonValuable;
        // Non-valuable window hovered by in-game cursor
        u32 focusableNonValuableHovered;
    } colors;

    bool drawWndRects;
    bool coloredRects;
};

struct CUIDebugState
{
    CUIDebuggable* selected{};
    mutable CUIDebuggable* newSelected{};
    mutable CUIDebuggable* examined{};

    CUIDebuggerSettings settings;

    void select(CUIDebuggable* debuggable) const
    {
        if (selected == debuggable)
            newSelected = nullptr;
        else
            newSelected = debuggable;
    }
};

class XRUICORE_API CUIDebugger final : public xray::editor::ide_tool
{
    xr_vector<CUIDebuggable*> m_root_windows;
    CUIDebugState m_state;

public:
    CUIDebugger();

    void Register(CUIDebuggable* debuggable);
    void Unregister(CUIDebuggable* debuggable);

    void on_tool_frame() override;

    [[nodiscard]]
    CUIDebuggable* GetSelected() const { return m_state.selected; }
    void SetSelected(CUIDebuggable* debuggable);

    [[nodiscard]]
    bool ShouldDrawRects() const { return m_state.settings.drawWndRects; }

private:
    pcstr tool_name() const override { return "UI Debugger"; }

    void reset_settings() override;
    void apply_setting(pcstr line) override;
    void save_settings(ImGuiTextBuffer* buffer) const override;
    size_t estimate_settings_size() const override;
};
