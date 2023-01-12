#pragma once

#include "IInputReceiver.h"

namespace xray::editor
{
class ENGINE_API ide :
    public IInputReceiver,
    public pureRender,
    public pureFrame
{

public:
    void UpdateWindowProps();

    operator bool();

public:
    // Interface implementations
    void OnFrame() override;
    void OnRender() override;

    void IR_Capture() override;
    void IR_Release() override;
};
} // namespace xray::editor
