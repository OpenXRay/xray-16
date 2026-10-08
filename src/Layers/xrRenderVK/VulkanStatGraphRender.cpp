#include "xrEngine/stdafx.h"
#include "VulkanStatGraphRender.h"
#include "VulkanGameDevice.h"
#include "xrEngine/device.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
void VulkanStatGraphRender::Copy(IStatGraphRender& source)
{
    const auto* other = dynamic_cast<VulkanStatGraphRender*>(&source);
    R_ASSERT2(other && &other->device_ == &device_,
        "Vulkan stat graph can only be copied within its owning device");
    if (other == this) return;
    // No graph data or GPU handles are shared: CStatGraph owns its samples.
    if (other->white_) OnDeviceCreate();
    else OnDeviceDestroy();
}

void VulkanStatGraphRender::OnDeviceCreate()
{
    if (white_) return;
    constexpr uint8_t white[] = {255, 255, 255, 255};
    std::string error;
    if (!device_.textures().ui_pixels(white, 1, 1, device_.ui_pass(), white_, error))
        Msg("! [renderer-vulkan] stat graph texture: %s", error.c_str());
}

void VulkanStatGraphRender::OnDeviceDestroy()
{
    if (!white_) return;
    R_ASSERT2(device_.wait_idle(), "Vulkan stat graph texture is still in use");
    device_.textures().release_ui(white_, device_.ui_pass());
    white_ = VK_NULL_HANDLE;
}

void VulkanStatGraphRender::line(float x0, float y0, float x1, float y1, u32 color)
{
    if (!std::isfinite(x0) || !std::isfinite(y0) ||
        !std::isfinite(x1) || !std::isfinite(y1)) return;
    auto& ui = device_.ui();
    ui.StartPrimitive(2, IUIRender::ptLineList, IUIRender::pttTL);
    ui.PushPoint(x0, y0, 0, color, .5f, .5f);
    ui.PushPoint(x1, y1, 0, color, .5f, .5f);
    ui.FlushPrimitive();
}

void VulkanStatGraphRender::rectangle(float x0, float y0, float x1, float y1, u32 color)
{
    if (!std::isfinite(x0) || !std::isfinite(y0) ||
        !std::isfinite(x1) || !std::isfinite(y1) || x0 == x1 || y0 == y1) return;
    auto& ui = device_.ui();
    ui.StartPrimitive(6, IUIRender::ptTriList, IUIRender::pttTL);
    ui.PushPoint(x0, y0, 0, color, .5f, .5f);
    ui.PushPoint(x1, y0, 0, color, .5f, .5f);
    ui.PushPoint(x0, y1, 0, color, .5f, .5f);
    ui.PushPoint(x0, y1, 0, color, .5f, .5f);
    ui.PushPoint(x1, y0, 0, color, .5f, .5f);
    ui.PushPoint(x1, y1, 0, color, .5f, .5f);
    ui.FlushPrimitive();
}

void VulkanStatGraphRender::OnRender(CStatGraph& owner)
{
    if (!white_) return;
    const float left = owner.lt.x, top = owner.lt.y;
    const float right = owner.rb.x, bottom = owner.rb.y;
    const float range = owner.mx - owner.mn;
    if (!(right > left && bottom > top) || !std::isfinite(range) || range <= 0 ||
        !owner.max_item_count) return;

    auto& ui = device_.ui();
    ui.SetTextureDescriptor(white_);
    ui.SetAlphaRef(0);
    ui.CacheSetCullMode(IUIRender::cmNONE);
    Irect clip;
    clip.set(std::max(0, int(std::floor(left))), std::max(0, int(std::floor(top))),
        std::min(int(Device.dwWidth), int(std::ceil(right))),
        std::min(int(Device.dwHeight), int(std::ceil(bottom))));
    if (clip.x2 <= clip.x1 || clip.y2 <= clip.y1) return;
    ui.SetScissor(&clip);

    rectangle(left, top, right, bottom, owner.back_color);
    line(left, top, right - 1, top, owner.rect_color);
    line(right - 1, top, right - 1, bottom, owner.rect_color);
    line(right - 1, bottom, left, bottom, owner.rect_color);
    line(left, bottom, left, top, owner.rect_color);

    const float factor = (bottom - top) / range;
    const float base = bottom + owner.mn * factor;
    line(left, base, right, base, owner.base_color);
    if (owner.grid_step.x > 0 && owner.grid_step.y > 0)
    {
        for (int i = 1; i <= std::min(owner.grid.x, 1024); ++i)
        {
            float x = left + i * owner.grid_step.x * factor;
            if (x >= right) break;
            line(x, top, x, bottom, owner.grid_color);
        }
        for (int i = 1; i <= std::min(owner.grid.y, 1024); ++i)
        {
            float delta = i * owner.grid_step.y * factor;
            if (base + delta < bottom) line(left, base + delta, right, base + delta, owner.grid_color);
            if (base - delta > top) line(left, base - delta, right, base - delta, owner.grid_color);
        }
    }

    const float step = (right - left) / owner.max_item_count;
    for (const auto& graph : owner.subgraphs)
    {
        for (size_t i = 0; i < graph.elements.size(); ++i)
        {
            const auto& item = graph.elements[i];
            const float x = left + float(i) * step;
            const float y = base - item.data * factor;
            if (graph.style == CStatGraph::stBar)
            {
                const float width = std::max(0.f, step - (step > 1.f ? 1.f : 0.f));
                rectangle(x, std::min(base, y), x + width, std::max(base, y), item.color);
            }
            else if (graph.style == CStatGraph::stPoint)
            {
                rectangle(x - 1, y - 1, x + 2, y + 2, item.color);
            }
            else if (i)
            {
                const auto& previous = graph.elements[i - 1];
                const float old_y = base - previous.data * factor;
                if (graph.style == CStatGraph::stBarLine)
                {
                    line(x, old_y, x, y, item.color);
                    line(x, y, x + step, y, item.color);
                }
                else if (graph.style == CStatGraph::stCurve)
                    line(x - step, old_y, x, y, item.color);
            }
        }
    }
    for (const auto& marker : owner.m_Markers)
    {
        if (marker.m_eStyle == CStatGraph::stVert)
        {
            const float x = std::clamp(left + marker.m_fPos * step, left, right);
            line(x, top, x, bottom, marker.m_dwColor);
        }
        else if (marker.m_eStyle == CStatGraph::stHor)
        {
            const float y = std::clamp(base - marker.m_fPos * factor, top, bottom);
            line(left, y, right, y, marker.m_dwColor);
        }
    }
    ui.SetScissor();
}
}
