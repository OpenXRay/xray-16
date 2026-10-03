#include "xrEngine/stdafx.h"
#include "VulkanRainRender.h"
#include "VulkanGameDevice.h"
#include "xrEngine/Rain.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/device.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
VulkanRainRender::~VulkanRainRender()
{
    device_.discard_rain(this);
    R_ASSERT2(device_.wait_idle(), "Vulkan rain buffers require idle GPU frames on destruction");
    if (material_) device_.textures().release_material(material_, device_.deferred());
}

void VulkanRainRender::Copy(IRainRender&)
{
    // The rain simulation belongs to CEffect_Rain. Each renderer instance
    // keeps independent frame buffers and acquires its own texture lease.
    vertices_.clear();
    indices_.clear();
}

void VulkanRainRender::quad(const Fvector& a, const Fvector& b, const Fvector& right,
    float width)
{
    const uint32_t base = static_cast<uint32_t>(vertices_.size());
    const Fvector corners[4]{
        Fvector().mad(a, right, -width), Fvector().mad(a, right, width),
        Fvector().mad(b, right, -width), Fvector().mad(b, right, width)};
    constexpr float uv[4][2]{{0, 1}, {1, 1}, {0, 0}, {1, 0}};
    for (size_t i = 0; i < 4; ++i)
    {
        LevelVertex vertex;
        vertex.position[0] = corners[i].x;
        vertex.position[1] = corners[i].y;
        vertex.position[2] = corners[i].z;
        vertex.normal[1] = 1.f;
        vertex.uv[0] = uv[i][0];
        vertex.uv[1] = uv[i][1];
        vertices_.push_back(vertex);
    }
    indices_.insert(indices_.end(), {base, base + 1, base + 2,
        base + 2, base + 1, base + 3});
}

void VulkanRainRender::Render(CEffect_Rain& owner)
{
    if (!g_pGamePersistent) return;
    const float density = g_pGamePersistent->Environment().CurrentEnv.rain_density;
    vertices_.clear();
    indices_.clear();
    if (!(density > EPS_L)) return;
    if (!material_)
    {
        std::string error;
        if (!device_.textures().material("fx\\fx_rain", device_.deferred(), material_, error))
        {
            Msg("! [renderer-vulkan] rain texture: %s", error.c_str());
            return;
        }
    }

    constexpr float radius = 12.5f;
    constexpr float source_height = 40.f;
    const size_t count = static_cast<size_t>(std::clamp(
        1250.f * (1.f + density), 0.f, 2500.f));
    while (owner.items.size() < count)
    {
        CEffect_Rain::Item item{};
        owner.Born(item, radius);
        owner.items.push_back(item);
    }
    const Fvector eye = Device.vCameraPosition;
    for (size_t i = 0; i < count; ++i)
    {
        auto& item = owner.items[i];
        if (item.dwTime_Hit < Device.dwTimeGlobal) owner.Hit(item.Phit);
        if (item.dwTime_Life < Device.dwTimeGlobal) owner.Born(item, radius);
        Fvector step;
        step.mul(item.D, item.fSpeed * Device.fTimeDelta);
        item.P.add(step);
        if (item.P.y < eye.y - 10.f ||
            item.P.distance_to_sqr(eye) > (source_height + radius) * (source_height + radius))
        {
            item.invalidate();
            continue;
        }
        if (item.dwTime_Hit == item.dwTime_Life && item.Phit.y > eye.y &&
            item.P.y >= item.Phit.y) continue;
        Fvector tail;
        tail.mad(item.P, item.D, -5.f * (.5f + .5f * density));
        Fvector view, right;
        view.sub(item.P, eye).normalize_safe();
        right.crossproduct(view, item.D).normalize_safe();
        if (right.square_magnitude() < EPS_L) continue;
        quad(tail, item.P, right, .3f);
    }

    auto* particle = owner.particle_active;
    while (particle)
    {
        auto* next = particle->next;
        particle->time -= Device.fTimeDelta;
        if (particle->time <= 0.f)
            owner.p_free(particle);
        else
        {
            const float size = particle->time / .3f * .6f;
            Fvector center = particle->mXForm.c;
            Fvector a, b, right;
            a.set(center.x, center.y + .01f, center.z - size);
            b.set(center.x, center.y + .01f, center.z + size);
            right.set(1.f, 0.f, 0.f);
            quad(a, b, right, size);
        }
        particle = next;
    }
    if (indices_.empty()) return;
    std::memcpy(mvp_, &Device.mFullTransform, sizeof(mvp_));
    device_.queue_rain(*this);
}

bool VulkanRainRender::ensure(BufferResource& resource, size_t bytes,
    VkBufferUsageFlags usage, std::string& error)
{
    if (resource.size() >= bytes) return true;
    const size_t capacity = std::max(bytes, size_t(4096));
    return resource.initialize(device_.window().device(), capacity, usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device_.window().physical().memory,
        device_.buffer_upload().buffer, error);
}

bool VulkanRainRender::record(const FrameRecordingContext& frame, std::string& error)
{
    if (indices_.empty()) return true;
    auto& buffers = frames_[frame.frame_index % frames_.size()];
    return ensure(buffers.vertices, vertices_.size() * sizeof(LevelVertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, error) &&
        ensure(buffers.indices, indices_.size() * sizeof(uint32_t),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, error) &&
        buffers.vertices.write(0, vertices_.data(), vertices_.size() * sizeof(LevelVertex), error) &&
        buffers.indices.write(0, indices_.data(), indices_.size() * sizeof(uint32_t), error) &&
        device_.deferred().record_transparent(frame, buffers.vertices.handle(),
            buffers.indices.handle(), static_cast<uint32_t>(indices_.size()), mvp_, material_);
}
}
