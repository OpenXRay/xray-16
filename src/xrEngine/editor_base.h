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

public:
    operator bool() const;

    void UpdateWindowProps();
    void UpdateInputAsync();

public:
    // Interface implementations
    void OnFrame() override;
    void OnRender() override;

    void IR_OnMousePress(int btn) override;
    void IR_OnMouseRelease(int btn) override;
    void IR_OnMouseHold(int btn) override;
    void IR_OnMouseWheel(int x, int y) override;
    void IR_OnMouseMove(int x, int y) override;

    void IR_OnKeyboardPress(int btn) override;
    void IR_OnKeyboardRelease(int btn) override;
    void IR_OnKeyboardHold(int btn) override;
    void IR_OnTextInput(pcstr text) override;

    void IR_OnControllerPress(int btn, float x, float y) override;
    void IR_OnControllerRelease(int btn, float x, float y) override;
    void IR_OnControllerHold(int btn, float x, float y) override;

    void IR_OnControllerAttitudeChange(Fvector change) override;

private:
    ImGuiContext* m_context;
};
} // namespace xray::editor
