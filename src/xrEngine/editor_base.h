#pragma once

#include "IInputReceiver.h"

struct ImGuiContext;

namespace xray::editor
{
class ENGINE_API ide :
    public IInputReceiver,
    public pureRender,
    public pureFrame
{
public:
    ide();
    ~ide();

    bool is_shown() const { return m_shown; }

public:
    void UpdateWindowProps();
    void UpdateInputAsync();

public:
    // Interface implementations
    void OnFrame() override;
    void OnRender() override;

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
    ImGuiContext* m_context;
    bool m_shown{};
};
} // namespace xray::editor
