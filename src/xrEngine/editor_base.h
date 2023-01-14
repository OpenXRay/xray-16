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
    void OnFrame() override;
    void OnRender() override;

    void OnAppActivate() override;
    void OnAppDeactivate() override;

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
    IImGuiRender* m_render{};
    ImGuiContext* m_context{};
    bool m_shown{};
};
} // namespace xray::editor
