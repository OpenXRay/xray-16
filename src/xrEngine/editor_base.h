#pragma once

#include "IInputReceiver.h"
#include "Include/xrRender/ImGuiRender.h"

#define IMGUI_DISABLE_OBSOLETE_KEYIO
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>

namespace xray::editor
{
struct ide_backend;

class ENGINE_API ide final :
    public pureRender,
    public pureFrame,
    public pureAppActivate,
    public pureAppDeactivate,
    public pureAppStart,
    public pureAppEnd,
    public IInputReceiver
{
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

public:
    // Interface implementations
    void OnFrame() override;
    void OnRender() override;

    void OnAppActivate() override;
    void OnAppDeactivate() override;

    void OnAppStart() override;
    void OnAppEnd() override;

    void IR_Capture() override;
    void IR_Release() override;

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

private:
    CTimer m_timer;
    IImGuiRender* m_render{};
    ImGuiContext* m_context{};
    ide_backend* m_backend_data{};

    visible_state m_state;
    struct
    {
        bool weather;
        bool imgui_demo;
        bool imgui_metrics;
    } m_windows{};
};
} // namespace xray::editor
