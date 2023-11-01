#pragma once

#include "IInputReceiver.h"
#include "Include/xrRender/ImGuiRender.h"

#define IMGUI_DISABLE_OBSOLETE_KEYIO
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>

namespace xray::editor
{
struct ide_backend;

class XR_NOVTABLE ENGINE_API ide_tool : public pureFrame
{
    bool is_opened{};

public:
    ide_tool();
    virtual ~ide_tool();

    virtual pcstr tool_name() = 0;

    bool& get_open_state() { return is_opened; }
    ImGuiWindowFlags get_default_window_flags() const;
};

class ENGINE_API ide final :
    public pureRender,
    public pureFrame,
    public pureAppActivate,
    public pureAppDeactivate,
    public pureAppStart,
    public pureAppEnd,
    public IInputReceiver
{
    friend class ide_tool;

public:
    enum class visible_state
    {
        hidden, // all ide windows are hidden
        full,   // input captured, opaque windows
        light,  // input not captured, transparent windows
    };

public:
    ide();
    ~ide() override;

    [[nodiscard]]
    bool is_shown() const;

public:
    void UpdateWindowProps();

    void OnDeviceCreate();
    void OnDeviceDestroy();
    void OnDeviceResetBegin() const;
    void OnDeviceResetEnd() const;

    void SetState(visible_state state);
    void SwitchToNextState();

    auto GetImGuiContext() const { return m_context; }

public:
    // Interface implementations
    void OnFrame() override;
    void OnRender() override;

    void OnAppActivate() override;
    void OnAppDeactivate() override;

    void OnAppStart() override;
    void OnAppEnd() override;

    void IR_OnActivate() override;
    void IR_OnDeactivate() override;

    void IR_OnMousePress(int key) override;
    void IR_OnMouseRelease(int key) override;
    void IR_OnMouseHold(int key) override;
    void IR_OnMouseWheel(int x, int y) override;
    void IR_OnMouseMove(int x, int y) override;

    void IR_OnKeyboardPress(int key) override;
    void IR_OnKeyboardRelease(int key) override;
    void IR_OnKeyboardHold(int key) override;
    void IR_OnTextInput(pcstr text) override;

    void IR_OnControllerPress(int key, float x, float y) override;
    void IR_OnControllerRelease(int key, float x, float y) override;
    void IR_OnControllerHold(int key, float x, float y) override;

    void IR_OnControllerAttitudeChange(Fvector change) override;

private:
    ImGuiWindowFlags get_default_window_flags() const;

private:
    void InitBackend();
    void ShutdownBackend();

private:
    void ShowMain();
    void ShowWeatherEditor();

    void RegisterTool(ide_tool* tool);
    void UnregisterTool(const ide_tool* tool);

    void UpdateTextInput(bool force_disable = false);

private:
    CTimer m_timer;
    IImGuiRender* m_render{};
    ImGuiContext* m_context{};
    ide_backend* m_backend_data{};

    visible_state m_state;
    bool m_show_weather_editor; // to be refactored
    bool m_text_input_enabled{};

    xr_vector<ide_tool*> m_tools;
};
} // namespace xray::editor
