#include "xrEngine/stdafx.h"
#include "VulkanDebugRender.h"

#ifdef DEBUG
#include "VulkanGameDevice.h"
#include "VulkanUIShader.h"
#include "Include/xrRender/DrawUtils.h"

namespace xray::render::vulkan
{
VulkanDebugRender::~VulkanDebugRender()
{
    OnDeviceDestroy();
}

void VulkanDebugRender::OnDeviceDestroy()
{
    for (unsigned i = 0; i < shaders_.size(); ++i)
        DestroyDebugShader(static_cast<dbgShaderHandle>(i));
}

void VulkanDebugRender::add_lines(const Fvector* vertices, const u32& count,
    const u16* pairs, const u32& pair_count, const u32& color)
{
    if (!vertices || !pairs) return;
    lines_.reserve(lines_.size() + pair_count);
    for (u32 i = 0; i < pair_count; ++i)
    {
        if (pairs[2 * i] >= count || pairs[2 * i + 1] >= count) continue;
        Line line;
        world_.transform_tiny(line.a, vertices[pairs[2 * i]]);
        world_.transform_tiny(line.b, vertices[pairs[2 * i + 1]]);
        line.color = color;
        lines_.push_back(line);
    }
}

void VulkanDebugRender::Render()
{
    if (GEnv.DU)
        for (const Line& line : lines_) GEnv.DU->DrawLine(line.a, line.b, line.color);
    lines_.clear();
}

void VulkanDebugRender::NextSceneMode()
{
    scene_mode_ = (scene_mode_ + 1) % 3;
    cull_ = scene_mode_ == 0 ? cmNONE : scene_mode_ == 1 ? cmCW : cmCCW;
}

void VulkanDebugRender::SetShader(const debug_shader& shader)
{
    auto* vk_shader = dynamic_cast<VulkanUIShader*>(&*shader);
    selected_ = vk_shader ? vk_shader->current_descriptor(Device.dwTimeContinual) : VK_NULL_HANDLE;
}

void VulkanDebugRender::SetDebugShader(dbgShaderHandle handle)
{
    R_ASSERT2(handle < dbgShaderCount, "invalid debug shader handle");
    if (!shaders_[handle])
    {
        std::string error;
        if (!device_.textures().ui("ui\\ui_pop_up_active_back", device_.ui_pass(),
                shaders_[handle], error))
            Msg("! [renderer-vulkan] debug texture: %s", error.c_str());
    }
    selected_ = shaders_[handle];
}

void VulkanDebugRender::DestroyDebugShader(dbgShaderHandle handle)
{
    R_ASSERT2(handle < dbgShaderCount, "invalid debug shader handle");
    if (selected_ == shaders_[handle]) selected_ = VK_NULL_HANDLE;
    if (shaders_[handle])
        device_.textures().release_ui(shaders_[handle], device_.ui_pass());
    shaders_[handle] = VK_NULL_HANDLE;
}

void VulkanDebugRender::dbg_DrawTRI(Fmatrix& transform, Fvector& a,
    Fvector& b, Fvector& c, u32 color)
{
    if (!selected_)
    {
        if (!GEnv.DU) return;
        Fvector points[3];
        transform.transform_tiny(points[0], a);
        transform.transform_tiny(points[1], b);
        transform.transform_tiny(points[2], c);
        GEnv.DU->DrawFace(points[0], points[1], points[2], color, color, TRUE, FALSE);
        return;
    }
    auto& ui = device_.ui();
    ui.SetTextureDescriptor(selected_);
    ui.CacheSetXformWorld(transform);
    ui.CacheSetCullMode(static_cast<IUIRender::CullMode>(cull_));
    ui.StartPrimitive(3, IUIRender::ptTriList, IUIRender::pttLIT);
    ui.PushPoint(a.x, a.y, a.z, color, 0.f, 0.f);
    ui.PushPoint(b.x, b.y, b.z, color, 1.f, 0.f);
    ui.PushPoint(c.x, c.y, c.z, color, 0.f, 1.f);
    ui.FlushPrimitive();
}

void VulkanObjectSpaceRender::Copy(IObjectSpaceRender& source)
{
    if (&source != this)
        spheres_ = static_cast<VulkanObjectSpaceRender&>(source).spheres_;
}
void VulkanObjectSpaceRender::SetShader()
{
    // dbg_draw_frustum calls this before emitting lines through IDrawUtils.
    // The Vulkan draw utility owns its pipeline; ensure that route is live.
    R_ASSERT2(GEnv.DU, "Vulkan object-space debug drawing requires IDrawUtils");
}
void VulkanObjectSpaceRender::dbgAddSphere(const Fsphere& sphere, u32 color)
{ spheres_.emplace_back(sphere, color); }
void VulkanObjectSpaceRender::dbgRender()
{
    if (GEnv.DU)
        for (const auto& [sphere, color] : spheres_)
            GEnv.DU->DrawSphere(Fidentity, sphere, color, color, FALSE, TRUE);
    spheres_.clear();
}
}
#endif
