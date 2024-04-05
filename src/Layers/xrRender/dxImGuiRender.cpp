#include "stdafx.h"

#include "dxImGuiRender.h"

#if defined(USE_DX9)
#include <backends/imgui_impl_dx9.h>
#elif defined(USE_DX11)
#include <backends/imgui_impl_dx11.h>
#elif defined(USE_DX12)
#include <backends/imgui_impl_dx12.h>
#elif defined(USE_OGL)
#include <backends/imgui_impl_opengl3.h>
#endif

void dxImGuiRender::Copy(IImGuiRender& _in) { *this = *dynamic_cast<dxImGuiRender*>(&_in); }

void dxImGuiRender::SetState(ImDrawData* data)
{
    RCache.SetViewport({0.f, 0.f, data->DisplaySize.x, data->DisplaySize.y, 0.f, 1.f});

    // Setup shader and vertex buffers
    /*unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
    ctx->IASetInputLayout(bd->pInputLayout);
    ctx->IASetVertexBuffers(0, 1, &bd->pVB, &stride, &offset);
    ctx->IASetIndexBuffer(bd->pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(bd->pVertexShader, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &bd->pVertexConstantBuffer);
    ctx->PSSetShader(bd->pPixelShader, nullptr, 0);
    ctx->PSSetSamplers(0, 1, &bd->pFontSampler);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently
    used.. ctx->DSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very
    infrequently used.. ctx->CSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well..
    very infrequently used..

    // Setup blend state
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ctx->OMSetBlendState(bd->pBlendState, blend_factor, 0xffffffff);
    ctx->OMSetDepthStencilState(bd->pDepthStencilState, 0);
    ctx->RSSetState(bd->pRasterizerState);*/
}

#if defined(USE_DX12)
#include "Layers/xrRenderPC_R5/DX12/CryDX12.hpp"
#endif

void dxImGuiRender::Frame()
{
#if defined(USE_DX9)
    ImGui_ImplDX9_NewFrame();
#elif defined(USE_DX11)
    ImGui_ImplDX11_NewFrame();
#elif defined(USE_DX12)
    ImGui_ImplDX12_NewFrame();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_NewFrame();
#endif
}

// ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void dxImGuiRender::Render(ImDrawData* data)
{
#if defined(USE_DX9)
    ImGui_ImplDX9_RenderDrawData(data);
#elif defined(USE_DX11)
    ImGui_ImplDX11_RenderDrawData(data);
#elif defined(USE_DX12)
    CCryDX12DeviceContext* pDeviceContext = reinterpret_cast<CCryDX12DeviceContext*>(HW.get_context(CHW::IMM_CTX_ID));
    DX12::CommandList* commandList = pDeviceContext->GetCoreGraphicsCommandList();
    ImGui_ImplDX12_RenderDrawData(data, commandList->GetD3D12CommandList());
    pDeviceContext->Flush();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_RenderDrawData(data);
#endif
}

void dxImGuiRender::OnDeviceCreate(ImGuiContext* context)
{
    ImGui::SetAllocatorFunctions([](size_t size, void* /*user_data*/) { return xr_malloc(size); },
        [](void* ptr, void* /*user_data*/) { xr_free(ptr); });

    ImGui::SetCurrentContext(context);

#if defined(USE_DX9)
    ImGui_ImplDX9_Init(HW.pDevice);
#elif defined(USE_DX11)
    ImGui_ImplDX11_Init(HW.pDevice, HW.get_context(CHW::IMM_CTX_ID));
#elif defined(USE_DX12)
    CCryDX12Device* pDevice = reinterpret_cast<CCryDX12Device*>(HW.pDevice);
    DX12::ResourceView* rtv = DX12_EXTRACT_DX12VIEW(RImplementation.Target->get_base_rt());
    DX12::Device* dx12Device = pDevice->GetDX12Device();
    DX12::DescriptorBlock descriptorBlock =
        dx12Device->GetGlobalDescriptorBlock(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, HW.BackBufferCount);
    DX12::DescriptorHeap* descriptorHeap = descriptorBlock.GetDescriptorHeap();
    ImGui_ImplDX12_Init(pDevice->GetD3D12Device(), HW.BackBufferCount, rtv->GetSRVDesc().Format,
        descriptorHeap->GetD3D12DescriptorHeap(), descriptorBlock.GetHandleOffsetCPU(0),
        descriptorBlock.GetHandleOffsetGPU(0));
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_Init();
#endif
}

void dxImGuiRender::OnDeviceDestroy()
{
#if defined(USE_DX9)
    ImGui_ImplDX9_Shutdown();
#elif defined(USE_DX11)
    ImGui_ImplDX11_Shutdown();
#elif defined(USE_DX12)
    ImGui_ImplDX12_Shutdown();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_Shutdown();
#endif
}

void dxImGuiRender::OnDeviceResetBegin()
{
#if defined(USE_DX9)
    ImGui_ImplDX9_InvalidateDeviceObjects();
#elif defined(USE_DX11)
    ImGui_ImplDX11_InvalidateDeviceObjects();
#elif defined(USE_DX12)
    ImGui_ImplDX12_InvalidateDeviceObjects();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
#endif
}

void dxImGuiRender::OnDeviceResetEnd()
{
#if defined(USE_DX9)
    ImGui_ImplDX9_CreateDeviceObjects();
#elif defined(USE_DX11)
    ImGui_ImplDX11_CreateDeviceObjects();
#elif defined(USE_DX12)
    ImGui_ImplDX12_CreateDeviceObjects();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_CreateDeviceObjects();
#endif
}
