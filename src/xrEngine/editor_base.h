#pragma once

#include "IInputReceiver.h"
#include "Include/xrRender/ImGuiRender.h"

#define IMGUI_DISABLE_OBSOLETE_KEYIO
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace xray::editor
{
class XR_NOVTABLE ENGINE_API ide_tool
{
    bool is_opened{};

public:
    ide_tool();
    virtual ~ide_tool();

    virtual void on_tool_frame() = 0;

    virtual pcstr tool_name() const = 0;

    bool& get_open_state() { return is_opened; }
    bool is_open() const { return is_opened; }
    virtual bool is_active() const { return is_opened; }

    ImGuiWindowFlags get_default_window_flags() const;

    virtual void reset_settings() {}
    virtual void apply_setting(pcstr /*line*/) {}
    virtual void apply_settings() {}
    virtual void save_settings(ImGuiTextBuffer* /*buffer*/) const {}
    virtual size_t estimate_settings_size() const { return 0; }
};

class ENGINE_API ide final :
    public pureFrame,
    public pureAppActivate,
    public pureAppDeactivate,
    public pureAppStart,
    public pureAppEnd,
    public IInputReceiver
{
    friend class ide_tool;

public:
    enum class visible_state : u8
    {
        hidden, // all ide windows are hidden
        full,   // input captured, opaque windows
        light,  // input not captured, transparent windows
    };

public:
    ide();
    ~ide() override;

    void InitBackend();

    void ProcessEvent(const SDL_Event& event);

    [[nodiscard]]
    bool is_shown() const;

    [[nodiscard]]
    auto GetState() const { return m_state; }
    void SetState(visible_state state);
    void SwitchToNextState();
    bool IsActiveState() const { return m_state == visible_state::full; }

    void UpdateTextInput(bool force_disable = false);

public:
    // Interface implementations
    void OnFrame() override;

    void OnAppActivate() override;
    void OnAppDeactivate() override;

    void OnAppStart() override;
    void OnAppEnd() override;

    void IR_OnActivate() override;
    void IR_OnDeactivate() override;

    void IR_OnMousePress(int key) override;
    void IR_OnMouseRelease(int key) override;
    void IR_OnMouseHold(int key) override;
    void IR_OnMouseWheel(float x, float y) override;
    void IR_OnMouseMove(int x, int y) override;

    void IR_OnKeyboardPress(int key) override;
    void IR_OnKeyboardRelease(int key) override;
    void IR_OnKeyboardHold(int key) override;
    void IR_OnTextInput(pcstr text) override;

    void IR_OnControllerPress(int key, const ControllerAxisState& state) override;
    void IR_OnControllerRelease(int key, const ControllerAxisState& state) override;
    void IR_OnControllerHold(int key, const ControllerAxisState& state) override;

    void IR_OnControllerAttitudeChange(Fvector change) override;

private:
    ImGuiWindowFlags get_default_window_flags() const;

private:
    void ShowMain();

    void RegisterTool(ide_tool* tool);
    void UnregisterTool(const ide_tool* tool);

    void UpdateMouseCursor();
    void UpdateMouseData();

private:
    visible_state m_state{};

    struct ImGuiBackend
    {
        Uint32 mouse_window_id{};
        int    mouse_last_leave_frame{};
        bool   mouse_can_report_hovered_viewport{};
        bool   text_input_enabled{};
    };
    ImGuiBackend m_imgui_backend{};

    xr_vector<ide_tool*> m_tools;
};
} // namespace xray::editor
