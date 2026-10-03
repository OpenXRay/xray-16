#include "xrEngine/stdafx.h"
#include "VulkanUIRender.h"
#include "VulkanUIShader.h"
#include "xrEngine/device.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xray::render::vulkan
{
void VulkanUIRender::configure(VkDevice device, const VkPhysicalDeviceMemoryProperties &memory, const BufferResourceDispatch &dispatch, const ScenePass &pass,
                               UiDescriptorLeases leases)
{
    DestroyUIGeom();
    device_ = device;
    memory_ = memory;
    dispatch_ = dispatch;
    pass_ = &pass;
    R_ASSERT2(bool(leases.retain) == bool(leases.release), "Vulkan UI descriptor lease callbacks are incomplete");
    leases_ = std::move(leases);
}

void VulkanUIRender::reset_frame()
{
    R_ASSERT2(primitive_ == ptNone, "Vulkan UI primitive was not flushed");
    // CPU batches can outlive their shader/video producer in the same frame.
    // The factory's final release waits for submitted frames before retirement.
    if (leases_.release)
        for (const auto &batch : batches_)
            leases_.release(batch.texture);
    vertices_.clear();
    world_positions_.clear();
    visible_.clear();
    indices_.clear();
    batches_.clear();
}

void VulkanUIRender::setup_states()
{
    R_ASSERT2(primitive_ == ptNone, "Vulkan UI state reset with an unfinished primitive");
    reset_frame();
    texture_ = VK_NULL_HANDLE;
    shader_ = nullptr;
    scissor_ = {};
    has_scissor_ = false;
    alpha_ref_ = 0;
    cull_ = cmNONE;
    world_.identity();
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

void VulkanUIRender::CreateUIGeom() { setup_states(); }

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
    shader_ = nullptr;
    has_scissor_ = false;
    alpha_ref_ = 0;
    cull_ = cmNONE;
}

void VulkanUIRender::SetShader(IUIShader& shader)
{
    auto* vk_shader = dynamic_cast<VulkanUIShader*>(&shader);
    R_ASSERT2(vk_shader && vk_shader->inited(), "Vulkan UI requires a loaded Vulkan texture shader");
    shader_ = vk_shader;
    texture_ = vk_shader->current_descriptor(Device.dwTimeContinual);
}

void VulkanUIRender::SetTextureDescriptor(VkDescriptorSet descriptor)
{
    R_ASSERT2(descriptor, "Vulkan UI requires a valid texture descriptor");
    shader_ = nullptr;
    texture_ = descriptor;
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

void VulkanUIRender::PushPoint(float x, float y, float z, u32 color, float u, float v)
{
    R_ASSERT2(primitive_ != ptNone && vertices_.size() - first_vertex_ < limit_,
        "Vulkan UI primitive exceeds its reserved vertex count");
    // D3D's packed ARGB bytes are BGRA in memory; Vulkan's R8G8B8A8
    // vertex input expects RGBA bytes.
    const u32 rgba = (color & 0xff00ff00u) | ((color >> 16) & 0xffu) | ((color & 0xffu) << 16);
    R_ASSERT2(world_positions_.size() == vertices_.size(), "Vulkan UI position buffers diverged");
    Fvector position; position.set(x, y, z);
    world_positions_.push_back(position);
    visible_.push_back(1);
    vertices_.push_back({{x, y}, {u, v}, rgba});
}

void VulkanUIRender::add_line(uint32_t a, uint32_t b)
{
    if (!visible_[a] || !visible_[b]) return;
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
    if (!visible_[a] || !visible_[b] || !visible_[c]) return;
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
    if (point_type_ == pttLIT)
        for (uint32_t i = first_vertex_; i < first_vertex_ + count; ++i)
        {
            Fvector position;
            world_.transform_tiny(position, world_positions_[i]);
            Fvector4 clip;
            Device.mFullTransform.transform(clip, position);
            if (std::isfinite(clip.w) && std::abs(clip.w) > 1.e-6f)
            {
                if (clip.w <= 0.f) { visible_[i] = 0; continue; }
                vertices_[i].position[0] = (clip.x / clip.w + 1.f) * .5f * Device.dwWidth;
                vertices_[i].position[1] = (1.f - clip.y / clip.w) * .5f * Device.dwHeight;
            }
            else visible_[i] = 0;
        }
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
        for (uint32_t i = 0; i + 1 < count; i += 2)
            add_line(first_vertex_ + i, first_vertex_ + i + 1);
    if (indices_.size() != first_index)
    {
        R_ASSERT2(!leases_.retain || leases_.retain(texture_), "Vulkan UI batch references an unleased texture");
        batches_.push_back({first_index, static_cast<uint32_t>(indices_.size()) - first_index, texture_, scissor_, has_scissor_, float(alpha_ref_) / 255.0f});
    }
    primitive_ = ptNone;
    world_positions_.resize(vertices_.size());
    visible_.resize(vertices_.size(), 1);
    point_type_ = pttNone;
}

void VulkanUIRender::append_imgui(ImDrawData* data)
{
    if (!data || !data->Valid || data->DisplaySize.x <= 0 || data->DisplaySize.y <= 0) return;
    R_ASSERT2(primitive_ == ptNone, "ImGui cannot interrupt an unfinished UI primitive");
    const ImVec2 origin = data->DisplayPos;
    const ImVec2 scale = data->FramebufferScale;
    for (int list_index = 0; list_index < data->CmdListsCount; ++list_index)
    {
        const ImDrawList* list = data->CmdLists[list_index];
        const size_t first_vertex = vertices_.size();
        vertices_.reserve(first_vertex + list->VtxBuffer.Size);
        for (const ImDrawVert& vertex : list->VtxBuffer)
            vertices_.push_back({{(vertex.pos.x - origin.x) * scale.x,
                (vertex.pos.y - origin.y) * scale.y},
                {vertex.uv.x, vertex.uv.y}, vertex.col});
        world_positions_.resize(vertices_.size());
        visible_.resize(vertices_.size(), 1);
        for (const ImDrawCmd& command : list->CmdBuffer)
        {
            if (command.UserCallback)
            {
                if (command.UserCallback != ImDrawCallback_ResetRenderState)
                    command.UserCallback(list, &command);
                continue;
            }
            const float x1 = (command.ClipRect.x - origin.x) * scale.x;
            const float y1 = (command.ClipRect.y - origin.y) * scale.y;
            const float x2 = (command.ClipRect.z - origin.x) * scale.x;
            const float y2 = (command.ClipRect.w - origin.y) * scale.y;
            const int32_t x = std::max(0, static_cast<int32_t>(std::floor(x1)));
            const int32_t y = std::max(0, static_cast<int32_t>(std::floor(y1)));
            const int32_t right = std::min<int32_t>(Device.dwWidth, static_cast<int32_t>(std::ceil(x2)));
            const int32_t bottom = std::min<int32_t>(Device.dwHeight, static_cast<int32_t>(std::ceil(y2)));
            if (right <= x || bottom <= y || !command.ElemCount) continue;
            VkDescriptorSet descriptor{};
            const ImTextureID id = command.GetTexID();
            static_assert(sizeof(descriptor) <= sizeof(id));
            std::memcpy(&descriptor, &id, sizeof(descriptor));
            if (!descriptor) continue;
            const uint32_t begin = static_cast<uint32_t>(indices_.size());
            for (unsigned i = 0; i < command.ElemCount; ++i)
            {
                const size_t index = size_t(command.IdxOffset) + i;
                R_ASSERT2(index < size_t(list->IdxBuffer.Size), "invalid ImGui index offset");
                const size_t vertex = first_vertex + command.VtxOffset + list->IdxBuffer[index];
                R_ASSERT2(vertex < first_vertex + size_t(list->VtxBuffer.Size), "invalid ImGui vertex offset");
                indices_.push_back(static_cast<uint32_t>(vertex));
            }
            R_ASSERT2(!leases_.retain || leases_.retain(descriptor), "Vulkan ImGui references an unleased texture");
            batches_.push_back(
                {begin, command.ElemCount, descriptor, {{x, y}, {static_cast<uint32_t>(right - x), static_cast<uint32_t>(bottom - y)}}, true, 0.f});
        }
    }
}

LPCSTR VulkanUIRender::UpdateShaderName(LPCSTR texture, LPCSTR shader)
{
    string_path path;
    return texture && FS.exist(path, "$game_textures$", texture, ".ogm") ? "hud\\movie" : shader;
}
void VulkanUIRender::CacheSetXformWorld(const Fmatrix& matrix) { world_ = matrix; }
void VulkanUIRender::CacheSetCullMode(CullMode mode) { cull_ = mode; }
} // namespace xray::render::vulkan
