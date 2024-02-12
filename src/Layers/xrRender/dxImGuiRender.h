#pragma once

#include "Include/xrRender/ImGuiRender.h"

class dxImGuiRender final : public IImGuiRender
{
    void SetState(ImDrawData* data);

#if defined(USE_DX12)
    _smart_ptr<ID3D12DescriptorHeap> m_descriptorHeap;
    _smart_ptr<ID3D12CommandAllocator> m_cmdAlloc;
    _smart_ptr<ID3D12GraphicsCommandList> m_cmdList;
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
