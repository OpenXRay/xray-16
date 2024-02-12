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

void dxImGuiRender::Copy(IImGuiRender& _in)
{
    *this = *dynamic_cast<dxImGuiRender*>(&_in);
}

void dxImGuiRender::SetState(ImDrawData* data)
{
    RCache.SetViewport({ 0.f, 0.f, data->DisplaySize.x, data->DisplaySize.y, 0.f, 1.f });

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
    ctx->HSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..
    ctx->DSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..
    ctx->CSSetShader(nullptr, nullptr, 0); // In theory we should backup and restore this as well.. very infrequently used..

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

ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void dxImGuiRender::Render(ImDrawData* data)
{
#if defined(USE_DX9)
    ImGui_ImplDX9_RenderDrawData(data);
#elif defined(USE_DX11)
    ImGui_ImplDX11_RenderDrawData(data);
#elif defined(USE_DX12)       
    ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap };
    DX12::ResourceView* rtv = DX12_EXTRACT_DX12VIEW(RImplementation.Target->get_base_rt()); 
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtv->GetDescriptorHandle();
    m_cmdList->Reset(m_cmdAlloc, nullptr);
    m_cmdList->ClearRenderTargetView(handle, (float*)&clear_color, 0, NULL);
    m_cmdList->OMSetRenderTargets(1, &handle, FALSE, NULL);
    m_cmdList->SetDescriptorHeaps(1, heaps);
    ImGui_ImplDX12_RenderDrawData(data, m_cmdList);
    R_ASSERT(SUCCEEDED(m_cmdList->Close()));
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_RenderDrawData(data);
#endif
}

void dxImGuiRender::OnDeviceCreate(ImGuiContext* context)
{
    ImGui::SetAllocatorFunctions(
        [](size_t size, void* /*user_data*/)
        {
            return xr_malloc(size);
        },
        [](void* ptr, void* /*user_data*/)
        {
            xr_free(ptr);
        }
    );
   
    ImGui::SetCurrentContext(context);

#if defined(USE_DX9)
    ImGui_ImplDX9_Init(HW.pDevice);
#elif defined(USE_DX11)
    ImGui_ImplDX11_Init(HW.pDevice, HW.get_context(CHW::IMM_CTX_ID));
#elif defined(USE_DX12)

    CCryDX12Device* pDevice = reinterpret_cast<CCryDX12Device*>(HW.pDevice);
  
    D3D12_DESCRIPTOR_HEAP_DESC descriptorImGuiRender = {};
    descriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorImGuiRender.NumDescriptors = HW.BackBufferCount;
    descriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = S_OK;
  
    // Create Descriptor Heap IMGUI render
    ID3D12DescriptorHeap* descriptorHeap = NULL;
    hr = pDevice->GetD3D12Device()->CreateDescriptorHeap(&descriptorImGuiRender, IID_PPV_ARGS(&descriptorHeap));  
    R_ASSERT(SUCCEEDED(hr));
    m_descriptorHeap = descriptorHeap;
    descriptorHeap->Release();

    ID3D12CommandAllocator* cmdAlloc = NULL;
    hr = pDevice->GetD3D12Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc));
    R_ASSERT(SUCCEEDED(hr));
    m_cmdAlloc = cmdAlloc;
    cmdAlloc->Release();

    ID3D12GraphicsCommandList* cmdList = NULL;
    hr = pDevice->GetD3D12Device()->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList));
    R_ASSERT(SUCCEEDED(hr));
    m_cmdList = cmdList;
    cmdList->Release();

    DX12::ResourceView* rtv = DX12_EXTRACT_DX12VIEW(RImplementation.Target->get_base_rt()); 

    ImGui_ImplDX12_Init(pDevice->GetD3D12Device(), HW.BackBufferCount, rtv->GetSRVDesc().Format,
        m_descriptorHeap, 
        m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
        m_descriptorHeap->GetGPUDescriptorHandleForHeapStart()
    );

    R_ASSERT(SUCCEEDED(m_cmdList->Close()));

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
