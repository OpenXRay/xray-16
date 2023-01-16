#pragma once

struct ImGuiContext;
struct ImDrawData;

class XR_NOVTABLE IImGuiRender
{
public:
    virtual ~IImGuiRender() = 0;
    virtual void Copy(IImGuiRender& _in) = 0;

    virtual void Frame() = 0;
    virtual void Render(ImDrawData* data) = 0;

    virtual void OnDeviceCreate(ImGuiContext* context) = 0;
    virtual void OnDeviceDestroy() = 0;
    virtual void OnDeviceResetBegin() = 0;
    virtual void OnDeviceResetEnd() = 0;
};

inline IImGuiRender::~IImGuiRender() = default;
