#pragma once

class XR_NOVTABLE XRUICORE_API CUIDebuggable
{
public:
    virtual ~CUIDebuggable() = 0;

    void RegisterDebuggable();
    void UnregisterDebuggable();

    virtual pcstr GetDebugType() { return "Debuggable"; }

    virtual bool FillDebugInfo() = 0;
};

inline CUIDebuggable::~CUIDebuggable() = default;

class XRUICORE_API CUIDebugger final : public xray::editor::ide_tool
{
    xr_vector<CUIDebuggable*> m_root_windows;

    bool m_draw_wnd_rects{};

public:
    CUIDebugger();

    void Register(CUIDebuggable* debuggable);
    void Unregister(CUIDebuggable* debuggable);

    void OnFrame() override;

    [[nodiscard]]
    bool ShouldDrawRects() const { return m_draw_wnd_rects; }

private:
    pcstr tool_name() override { return "UI Debugger"; }
};
