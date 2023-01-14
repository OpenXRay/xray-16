#pragma once

#include "IInputReceiver.h"
#include "Include/xrRender/ImGuiRender.h"

#define IMGUI_DISABLE_OBSOLETE_KEYIO
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
struct ImGuiContext;

namespace xray::editor
{
class ENGINE_API ide :
    public pureRender,
    public pureFrame,
    public pureAppActivate,
    public pureAppDeactivate,
    public IInputReceiver
{
public:
    ide();
    ~ide();

    bool is_shown() const { return m_shown; }

public:
    void UpdateWindowProps();
    void UpdateInputAsync();

    void OnDeviceCreate();
    void OnDeviceDestroy();
    void OnDeviceResetBegin() const;
    void OnDeviceResetEnd() const;

public:
    // Interface implementations
    void OnFrame() final;
    void OnRender() final;

    void OnAppActivate() final;
    void OnAppDeactivate() final;

    void IR_Capture() final;
    void IR_Release() final;

    void IR_OnMousePress(int key) final;
    void IR_OnMouseRelease(int key) final;
    void IR_OnMouseHold(int key) final;
    void IR_OnMouseWheel(int x, int y) final;
    void IR_OnMouseMove(int x, int y) final;

    void IR_OnKeyboardPress(int key) final;
    void IR_OnKeyboardRelease(int key) final;
    void IR_OnKeyboardHold(int key) final;
    void IR_OnTextInput(pcstr text) final;

    void IR_OnControllerPress(int key, float x, float y) final;
    void IR_OnControllerRelease(int key, float x, float y) final;
    void IR_OnControllerHold(int key, float x, float y) final;

    void IR_OnControllerAttitudeChange(Fvector change) final;

private:
    IImGuiRender* m_render{};
    ImGuiContext* m_context{};
    bool m_shown{};
};
} // namespace xray::editor
