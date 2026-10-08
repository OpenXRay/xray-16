#include "xrEngine/stdafx.h"
#include "VulkanThunderboltRender.h"
#include "VulkanGameDevice.h"
#include "VulkanLensFlareRender.h"
#include "xrEngine/thunderbolt.h"
#include "xrEngine/device.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
VulkanThunderboltRender::~VulkanThunderboltRender()
{
    device_.discard_thunderbolt(this);
    R_ASSERT2(device_.wait_idle(), "Vulkan lightning buffers require idle GPU frames on destruction");
}

namespace
{
struct Cursor
{
    const uint8_t* bytes{};
    size_t length{}, position{};
    bool read(void* destination, size_t count)
    {
        if (!bytes || position > length || count > length - position) return false;
        std::memcpy(destination, bytes + position, count);
        position += count;
        return true;
    }
    bool name(std::string& result)
    {
        const size_t start = position;
        while (position < length && bytes[position] && position - start < 255) ++position;
        if (position == length || position - start == 255) return false;
        result.assign(reinterpret_cast<const char*>(bytes + start), position++ - start);
        return true;
    }
};

void gradient(VulkanGameDevice& device, const Fvector& center,
    const SThunderboltDesc::SFlare* flare, float size, float phase)
{
    if (!flare || !flare->m_pFlare || phase <= EPS_L) return;
    auto* texture = dynamic_cast<VulkanFlareRender*>(&*flare->m_pFlare);
    if (!texture || !texture->descriptor()) return;
    const float x = flare->fRadius.x * size, y = flare->fRadius.y * size;
    const Fvector right = Device.vCameraRight, up = Device.vCameraTop;
    const float opacity = std::clamp(flare->fOpacity * phase, 0.f, 1.f);
    const u32 color = color_rgba_f(opacity, opacity, opacity, opacity);
    Fvector points[4];
    points[0].mad(center, right, x).mad(up, -y);
    points[1].mad(center, right, x).mad(up, y);
    points[2].mad(center, right, -x).mad(up, -y);
    points[3].mad(center, right, -x).mad(up, y);
    auto& ui = device.ui();
    ui.SetTextureDescriptor(texture->descriptor());
    ui.CacheSetXformWorld(Fidentity);
    ui.CacheSetCullMode(IUIRender::cmNONE);
    ui.StartPrimitive(4, IUIRender::ptTriStrip, IUIRender::pttLIT);
    constexpr float uv[4][2]{{0, 0}, {0, 1}, {1, 0}, {1, 1}};
    for (size_t i = 0; i < 4; ++i)
        ui.PushPoint(points[i].x, points[i].y, points[i].z, color, uv[i][0], uv[i][1]);
    ui.FlushPrimitive();
}
}

void VulkanThunderboltDescRender::Copy(IThunderboltDescRender& source)
{
    if (&source == this) return;
    const auto& other = static_cast<VulkanThunderboltDescRender&>(source);
    const std::string name = other.name_;
    DestroyModel();
    if (!name.empty()) CreateModel(name.c_str());
}

void VulkanThunderboltDescRender::CreateModel(LPCSTR name)
{
    DestroyModel();
    if (!name || !*name) return;
    IReader* reader = FS.r_open("$game_meshes$", name);
    if (!reader)
    {
        Msg("! [renderer-vulkan] lightning model not found: %s", name);
        return;
    }
    Cursor cursor{static_cast<const uint8_t*>(reader->pointer()), reader->length()};
    std::string shader, texture;
    uint32_t flags{}, count_vertices{}, count_indices{};
    float min_scale{}, max_scale{};
    bool valid = cursor.name(shader) && cursor.name(texture) &&
        cursor.read(&flags, 4) && cursor.read(&min_scale, 4) &&
        cursor.read(&max_scale, 4) && cursor.read(&count_vertices, 4) &&
        cursor.read(&count_indices, 4) && count_vertices > 0 && count_vertices <= 65535 &&
        count_indices > 0 && count_indices <= 196605 && count_indices % 3 == 0 &&
        cursor.length - cursor.position == size_t(count_vertices) * 20 + size_t(count_indices) * 2;
    std::vector<LevelVertex> vertices;
    std::vector<uint32_t> indices;
    if (valid)
    {
        vertices.resize(count_vertices);
        indices.resize(count_indices);
        for (auto& vertex : vertices)
        {
            valid = cursor.read(vertex.position, 12) && cursor.read(vertex.uv, 8);
            vertex.normal[1] = 1.f;
            if (!valid) break;
        }
        for (auto& index : indices)
        {
            uint16_t small{};
            if (!(valid = valid && cursor.read(&small, 2) && small < count_vertices)) break;
            index = small;
        }
    }
    FS.r_close(reader);
    if (!valid)
    {
        Msg("! [renderer-vulkan] invalid lightning detail model: %s", name);
        return;
    }
    std::string error;
    if (!device_.textures().material(texture, device_.deferred(), material_, error))
    {
        Msg("! [renderer-vulkan] lightning texture '%s': %s", texture.c_str(), error.c_str());
        return;
    }
    name_ = name;
    vertices_ = std::move(vertices);
    indices_ = std::move(indices);
}

void VulkanThunderboltDescRender::DestroyModel()
{
    if (material_)
        device_.textures().release_material(material_, device_.deferred());
    material_ = VK_NULL_HANDLE;
    name_.clear();
    vertices_.clear();
    indices_.clear();
}

void VulkanThunderboltRender::Render(CEffect_Thunderbolt& owner)
{
    if (!owner.current) return;
    auto* model = dynamic_cast<VulkanThunderboltDescRender*>(&*owner.current->m_pRender);
    if (!model || !model->material()) return;
    vertices_ = model->vertices();
    indices_ = model->indices();
    material_ = model->material();
    const float dv = owner.lightning_phase > .5f ?
        float(Random.randI(2)) * .5f : owner.lightning_phase * .5f;
    for (auto& vertex : vertices_)
    {
        Fvector position, transformed;
        position.set(vertex.position[0], vertex.position[1], vertex.position[2]);
        owner.current_xform.transform_tiny(transformed, position);
        vertex.position[0] = transformed.x;
        vertex.position[1] = transformed.y;
        vertex.position[2] = transformed.z;
        vertex.uv[1] += dv;
    }
    std::memcpy(mvp_, &Device.mFullTransform, sizeof(mvp_));
    device_.queue_thunderbolt(*this);
    gradient(device_, owner.current_xform.c, owner.current->m_GradientTop,
        owner.lightning_size, owner.lightning_phase);
    gradient(device_, owner.lightning_center, owner.current->m_GradientCenter,
        owner.lightning_size, owner.lightning_phase);
}

bool VulkanThunderboltRender::ensure(BufferResource& resource, size_t bytes,
    VkBufferUsageFlags usage, std::string& error)
{
    if (resource.size() >= bytes) return true;
    return resource.initialize(device_.window().device(), std::max(bytes, size_t(4096)), usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, device_.window().physical().memory,
        device_.buffer_upload().buffer, error);
}

bool VulkanThunderboltRender::record(const FrameRecordingContext& frame, std::string& error)
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
