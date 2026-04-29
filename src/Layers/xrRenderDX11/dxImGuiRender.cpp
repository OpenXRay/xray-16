#include "stdafx.h"

#include "dxImGuiRender.h"

#if defined(USE_DX11)
#include <backends/imgui_impl_dx11.h>
#elif defined(USE_OGL)
#include <backends/imgui_impl_opengl3.h>
#endif

namespace xray::render::RENDER_NAMESPACE
{
void dxImGuiRender::Copy(IImGuiRender& _in)
{
    *this = *dynamic_cast<dxImGuiRender*>(&_in);
}

void dxImGuiRender::SetState(ImDrawData* data)
{
    RCache.SetViewport({ 0.f, 0.f, data->DisplaySize.x, data->DisplaySize.y, 0.f, 1.f });

}

void dxImGuiRender::Frame()
{
#if defined(USE_DX11)
    ImGui_ImplDX11_NewFrame();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_NewFrame();
#endif
}

void dxImGuiRender::Render(ImDrawData* data)
{
#ifdef XR_PLATFORM_WEB
    if (data->CmdListsCount == 0)
        return;
#endif

#if defined(USE_DX11)
    ImGui_ImplDX11_RenderDrawData(data);
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_RenderDrawData(data);
#endif
}

void dxImGuiRender::OnDeviceCreate(ImGuiContext* context)
{
    ImGui::SetAllocatorFunctions(
        [](size_t size, void* )
        {
            return xr_malloc(size);
        },
        [](void* ptr, void* )
        {
            xr_free(ptr);
        }
    );
    ImGui::SetCurrentContext(context);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "xrRender";

#if defined(USE_DX11)
    ImGui_ImplDX11_Init(HW.pDevice, HW.get_context(CHW::IMM_CTX_ID));
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_Init();
#endif
}
void dxImGuiRender::OnDeviceDestroy()
{
#if defined(USE_DX11)
    ImGui_ImplDX11_Shutdown();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_Shutdown();
#endif
}

void dxImGuiRender::OnDeviceResetBegin()
{
#if defined(USE_DX11)
    ImGui_ImplDX11_InvalidateDeviceObjects();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
#endif
}

void dxImGuiRender::OnDeviceResetEnd()
{
#if defined(USE_DX11)
    ImGui_ImplDX11_CreateDeviceObjects();
#elif defined(USE_OGL)
    ImGui_ImplOpenGL3_CreateDeviceObjects();
#endif
}
}
