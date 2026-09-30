#include "xrEngine/stdafx.h"
#include "VulkanUIRender.h"
#include "VulkanUIShader.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xray::render::vulkan
{
void VulkanUIRender::configure(VkDevice device, const VkPhysicalDeviceMemoryProperties& memory,
    const BufferResourceDispatch& dispatch, const ScenePass& pass)
{
    DestroyUIGeom();
    device_ = device;
    memory_ = memory;
    dispatch_ = dispatch;
    pass_ = &pass;
}

void VulkanUIRender::reset_frame()
{
    R_ASSERT2(primitive_ == ptNone, "Vulkan UI primitive was not flushed");
    vertices_.clear();
    indices_.clear();
    batches_.clear();
}

bool VulkanUIRender::ensure(BufferResource& buffer, VkDeviceSize bytes,
    VkBufferUsageFlags usage, std::string& error)
{
    if (buffer.size() >= bytes) return true;
    const VkDeviceSize capacity = std::max<VkDeviceSize>(bytes, buffer.size() ? buffer.size() * 2 : 4096);
    return buffer.initialize(device_, capacity, usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        memory_, dispatch_, error);
}

bool VulkanUIRender::record(const FrameRecordingContext& frame, std::string& error)
{
    if (primitive_ != ptNone || !pass_ || !device_ || frame.frame_index >= frames_.size())
    {
        error = "Vulkan UI frame is not initialized or has an unfinished primitive";
        return false;
    }
    if (indices_.empty()) return true;
    FrameBuffers& buffers = frames_[frame.frame_index];
    const VkDeviceSize vertex_bytes = vertices_.size() * sizeof(UiVertex);
    const VkDeviceSize index_bytes = indices_.size() * sizeof(uint32_t);
    if (!ensure(buffers.vertices, vertex_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, error) ||
        !ensure(buffers.indices, index_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, error) ||
        !buffers.vertices.write(0, vertices_.data(), vertex_bytes, error) ||
        !buffers.indices.write(0, indices_.data(), index_bytes, error)) return false;
    for (const Batch& batch : batches_)
        if (!pass_->record_ui(frame, buffers.vertices.handle(), buffers.indices.handle(),
                VK_INDEX_TYPE_UINT32, batch.count, batch.texture,
                batch.has_scissor ? &batch.scissor : nullptr, 0,
                VkDeviceSize(batch.first_index) * sizeof(uint32_t), batch.alpha_ref))
        {
            error = "Vulkan UI draw command recording failed";
            return false;
        }
    error.clear();
    return true;
}

void VulkanUIRender::CreateUIGeom() { reset_frame(); }

void VulkanUIRender::DestroyUIGeom()
{
    // The device owner waits for idle before destroying or reconfiguring UI.
    for (auto& buffers : frames_)
    {
        buffers.vertices.destroy();
        buffers.indices.destroy();
    }
    primitive_ = ptNone;
    reset_frame();
    texture_ = VK_NULL_HANDLE;
    has_scissor_ = false;
    alpha_ref_ = 0;
    cull_ = cmNONE;
}

void VulkanUIRender::SetShader(IUIShader& shader)
{
    auto* vk_shader = dynamic_cast<VulkanUIShader*>(&shader);
    R_ASSERT2(vk_shader && vk_shader->inited(), "Vulkan UI requires a loaded Vulkan texture shader");
    texture_ = vk_shader->descriptor();
}

void VulkanUIRender::SetAlphaRef(int aref) { alpha_ref_ = std::clamp(aref, 0, 255); }

void VulkanUIRender::SetScissor(Irect* rect)
{
    has_scissor_ = rect != nullptr;
    if (rect)
    {
        const int x = std::max(rect->x1, 0), y = std::max(rect->y1, 0);
        scissor_ = {{x, y}, {static_cast<uint32_t>(std::max(rect->x2 - x, 0)),
            static_cast<uint32_t>(std::max(rect->y2 - y, 0))}};
    }
}

void VulkanUIRender::StartPrimitive(u32 max_vertices, ePrimitiveType type, ePointType point_type)
{
    R_ASSERT2(primitive_ == ptNone && max_vertices &&
        (type == ptTriList || type == ptTriStrip || type == ptLineStrip || type == ptLineList) &&
        (point_type == pttTL || point_type == pttLIT), "invalid Vulkan UI primitive");
    R_ASSERT2(vertices_.size() <= UINT32_MAX - max_vertices, "Vulkan UI vertex limit exceeded");
    primitive_ = type;
    point_type_ = point_type;
    first_vertex_ = static_cast<uint32_t>(vertices_.size());
    limit_ = max_vertices;
    vertices_.reserve(vertices_.size() + max_vertices);
}

void VulkanUIRender::PushPoint(float x, float y, float, u32 color, float u, float v)
{
    R_ASSERT2(primitive_ != ptNone && vertices_.size() - first_vertex_ < limit_,
        "Vulkan UI primitive exceeds its reserved vertex count");
    // D3D's packed ARGB bytes are BGRA in memory; Vulkan's R8G8B8A8
    // vertex input expects RGBA bytes.
    const u32 rgba = (color & 0xff00ff00u) | ((color >> 16) & 0xffu) | ((color & 0xffu) << 16);
    vertices_.push_back({{x, y}, {u, v}, rgba});
}

void VulkanUIRender::add_line(uint32_t a, uint32_t b)
{
    const UiVertex& va = vertices_[a];
    const UiVertex& vb = vertices_[b];
    const float dx = vb.position[0] - va.position[0], dy = vb.position[1] - va.position[1];
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.0001f) return;
    const float nx = -dy / (2 * length), ny = dx / (2 * length);
    const uint32_t base = static_cast<uint32_t>(vertices_.size());
    UiVertex a0 = va, a1 = va, b0 = vb, b1 = vb;
    a0.position[0] += nx; a0.position[1] += ny;
    a1.position[0] -= nx; a1.position[1] -= ny;
    b0.position[0] += nx; b0.position[1] += ny;
    b1.position[0] -= nx; b1.position[1] -= ny;
    vertices_.insert(vertices_.end(), {a0, a1, b0, b1});
    indices_.insert(indices_.end(), {base, base + 1, base + 2, base + 2, base + 1, base + 3});
}

void VulkanUIRender::add_triangle(uint32_t a, uint32_t b, uint32_t c)
{
    const auto& p = vertices_[a].position;
    const auto& q = vertices_[b].position;
    const auto& r = vertices_[c].position;
    const float signed_area = (q[0] - p[0]) * (r[1] - p[1]) -
        (q[1] - p[1]) * (r[0] - p[0]);
    // Engine UI coordinates increase downwards: positive signed area is CW.
    if ((cull_ == cmCW && signed_area > 0) ||
        (cull_ == cmCCW && signed_area < 0)) return;
    indices_.insert(indices_.end(), {a, b, c});
}

void VulkanUIRender::FlushPrimitive()
{
    R_ASSERT2(primitive_ != ptNone && texture_, "Vulkan UI primitive has no texture");
    const uint32_t count = static_cast<uint32_t>(vertices_.size()) - first_vertex_;
    const uint32_t first_index = static_cast<uint32_t>(indices_.size());
    if (primitive_ == ptTriList)
        for (uint32_t i = 0; i + 2 < count; i += 3)
            add_triangle(first_vertex_ + i, first_vertex_ + i + 1, first_vertex_ + i + 2);
    else if (primitive_ == ptTriStrip)
        for (uint32_t i = 0; i + 2 < count; ++i)
            if (i & 1) add_triangle(first_vertex_ + i + 1, first_vertex_ + i, first_vertex_ + i + 2);
            else add_triangle(first_vertex_ + i, first_vertex_ + i + 1, first_vertex_ + i + 2);
    else if (primitive_ == ptLineStrip)
        for (uint32_t i = 0; i + 1 < count; ++i) add_line(first_vertex_ + i, first_vertex_ + i + 1);
    else
        for (uint32_t i = 0; i + 1 < count; i += 2) add_line(first_vertex_ + i, first_vertex_ + i + 1);
    if (indices_.size() != first_index)
        batches_.push_back({first_index, static_cast<uint32_t>(indices_.size()) - first_index,
            texture_, scissor_, has_scissor_, float(alpha_ref_) / 255.0f});
    primitive_ = ptNone;
    point_type_ = pttNone;
}

LPCSTR VulkanUIRender::UpdateShaderName(LPCSTR, LPCSTR shader) { return shader; }
void VulkanUIRender::CacheSetXformWorld(const Fmatrix&) {}
void VulkanUIRender::CacheSetCullMode(CullMode mode) { cull_ = mode; }
}
