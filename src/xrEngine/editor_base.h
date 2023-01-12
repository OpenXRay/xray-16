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
    void UpdateWindowProps();

    operator bool() const;

public:
    // Interface implementations
    void OnFrame() override;
    void OnRender() override;

    void IR_Capture() override;
    void IR_Release() override;

private:
    ImGuiContext* m_context;
};
} // namespace xray::editor
