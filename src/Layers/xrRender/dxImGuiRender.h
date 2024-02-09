#pragma once

#include "Include/xrRender/ImGuiRender.h"

class dxImGuiRender final : public IImGuiRender
{
    void SetState(ImDrawData* data);

#if defined(USE_DX12)
    ID3D12DescriptorHeap* m_pDescriptorHeap = NULL;
#endif

public:
    void Copy(IImGuiRender& _in) override;

    void Frame() override;
    void Render(ImDrawData* data) override;

    void OnDeviceCreate(ImGuiContext* context) override;
    void OnDeviceDestroy() override;
    void OnDeviceResetBegin() override;
    void OnDeviceResetEnd() override;
};
