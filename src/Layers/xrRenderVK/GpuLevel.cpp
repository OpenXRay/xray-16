#include "xrEngine/stdafx.h"
#include "GpuLevel.h"
#include "LevelVisibility.h"
#include "VulkanVisual.h"
#include "SpecialVisuals.h"
#include "SlidingWindows.h"
#include "xrEngine/device.h"
#include "xrEngine/IGame_Level.h"
#include "xrCDB/xr_area.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <limits>

namespace xray::render::vulkan
{
namespace
{
std::string lightmap_texture(const std::string& textures)
{
    const size_t comma = textures.find(',');
    if (comma == std::string::npos) return {};
    const size_t next = textures.find(',', comma + 1);
    std::string name = textures.substr(comma + 1, next == std::string::npos ? next : next - comma - 1);
    const size_t start = name.find_first_not_of(" \t");
    if (start == std::string::npos) return {};
    name.erase(0, start);
    const size_t end = name.find_last_not_of(" \t");
    name.resize(end + 1);
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return lower.find("lmap") != std::string::npos ? name : std::string{};
}
bool named_surface(const LevelMaterial& material, const char* marker)
{
    std::string name = material.shader + " " + material.textures;
    std::transform(name.begin(), name.end(), name.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return material.mode == SurfaceMode::Transparent && name.find(marker) != std::string::npos;
}
}
GpuLevel::~GpuLevel() { destroy(); }

bool GpuLevel::load(IReader& level, VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
    GameTextureFactory& textures, DeferredPass& pass, std::string& error)
{
    if (!device || !queue || !pool)
    {
        error = "Vulkan level requires a device and command pool";
        return false;
    }
    LevelModelData models;
    if (!load_engine_level_models(level, models, error)) return false;
    DetailAssets details;
    if (!load_engine_detail_assets(details, error)) return false;
    // Portal data is an optimization input. If an old or incomplete level
    // omits it, the renderer uses the complete root list rather than hiding
    // geometry based on partial visibility information.
    std::string visibility_error;
    if (!load_engine_level_visibility(level, models, visibility_error))
        Msg("! Vulkan: level visibility data unavailable (%s); rendering all level roots",
            visibility_error.c_str());
    GpuLevel prepared;
    prepared.device_ = device;
    prepared.pool_ = pool;
    prepared.upload_ = upload;
    prepared.textures_ = &textures;
    prepared.pass_ = &pass;
    prepared.visuals_ = std::move(models.visuals);
    prepared.roots_ = std::move(models.roots);
    prepared.sectors_ = std::move(models.sectors);
    prepared.portals_ = std::move(models.portals);
    prepared.meshes_.reserve(models.models.size());
    const auto upload_mesh = [&](const LevelModel& model, Mesh& mesh) -> bool
    {
        if (model.material >= models.materials.size() || model.vertices.empty() ||
            model.indices.empty() || model.indices.size() > std::numeric_limits<uint32_t>::max())
        {
            error = "invalid or empty Vulkan level model";
            return false;
        }
        const auto& material = models.materials[model.material];
        const auto mode = material.mode;
        const std::string lightmap = mode == SurfaceMode::Transparent ? std::string{} : lightmap_texture(material.textures);
        if (!lightmap.empty() && !model.lightmap_uv)
        {
            error = "level lightmap material has no TEXCOORD1: " + material.textures;
            return false;
        }
        const char *family = mode == SurfaceMode::Transparent ? "object_blended" :
            !lightmap.empty() ? mode == SurfaceMode::AlphaTest ? "level_lightmap_cutout" : "level_lightmap" :
            mode == SurfaceMode::AlphaTest ? "level_cutout" : "level_opaque";
        const std::string shader = std::string("vk\\") + family;
        mesh.lightmapped = !lightmap.empty();
        mesh.water = named_surface(material, "water") || named_surface(material, "glass");
        mesh.glass = named_surface(material, "glass");
        prepared.has_water_ |= mesh.water;
        if (!pass.request_game_pipeline(shader + ".vs", shader + ".ps", mode, false, false, error))
        {
            error = "level / material id=" + std::to_string(model.material) + " shader pair: " + error;
            return false;
        }
        const size_t comma = material.textures.find(',');
        const std::string diffuse = material.textures.substr(0, comma);
        if (!(lightmap.empty() ? textures.material(material.textures, pass, mesh.material, error) :
                textures.lightmapped_material(diffuse, lightmap, pass, mesh.material, error)) ||
            !upload_buffer(device, queue, pool, memory, upload, model.vertices.data(), model.vertices.size() * sizeof(LevelVertex),
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertices, prepared.pending_, error) ||
            !upload_buffer(device, queue, pool, memory, upload, model.indices.data(), model.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           mesh.indices, prepared.pending_, error))
            return false;
        mesh.index_count = static_cast<uint32_t>(model.indices.size());
        mesh.windows = model.windows;
        mesh.mode = models.materials[model.material].mode;
        return true;
    };
    for (const LevelModel& model : models.models)
    {
        prepared.meshes_.emplace_back();
        Mesh& mesh = prepared.meshes_.back();
        if (!upload_mesh(model, mesh))
            return false;
        if (model.fast)
        {
            mesh.fast = std::make_unique<Mesh>();
            if (!upload_mesh(*model.fast, *mesh.fast))
                return false;
        }
    }
    prepared.details_ = std::move(details);
    prepared.detail_meshes_.reserve(prepared.details_.prototypes.size());
    for (const DetailPrototype& prototype : prepared.details_.prototypes)
    {
        prepared.detail_meshes_.emplace_back();
        Mesh& mesh = prepared.detail_meshes_.back();
        if (!pass.request_game_pipeline("vk\\level_cutout.vs", "vk\\level_cutout.ps",
                SurfaceMode::AlphaTest, false, false, error) ||
            !textures.material(prototype.texture, pass, mesh.material, error) ||
            !upload_buffer(device, queue, pool, memory, upload, prototype.vertices.data(),
                prototype.vertices.size() * sizeof(LevelVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                mesh.vertices, prepared.pending_, error) ||
            !upload_buffer(device, queue, pool, memory, upload, prototype.indices.data(),
                prototype.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                mesh.indices, prepared.pending_, error))
        {
            error = "level.details / " + prototype.texture + ": " + error;
            return false;
        }
        mesh.index_count = static_cast<uint32_t>(prototype.indices.size());
        mesh.mode = SurfaceMode::AlphaTest;
    }
    // The old level may still have been submitted. Its owner must wait for
    // those frames before replacing the level. Uploads within prepared use
    // the same queue as the first draw and preserve staging until retired.
    destroy();
    device_ = prepared.device_;
    pool_ = prepared.pool_;
    upload_ = prepared.upload_;
    textures_ = prepared.textures_;
    pass_ = prepared.pass_;
    pending_ = std::move(prepared.pending_);
    meshes_ = std::move(prepared.meshes_);
    has_water_ = prepared.has_water_;
    details_ = std::move(prepared.details_);
    detail_meshes_ = std::move(prepared.detail_meshes_);
    visuals_ = std::move(prepared.visuals_);
    roots_ = std::move(prepared.roots_);
    sectors_ = std::move(prepared.sectors_);
    portals_ = std::move(prepared.portals_);
    visual_objects_.reserve(visuals_.size());
    for (size_t index = 0; index < visuals_.size(); ++index)
    {
        visual_objects_.emplace_back(std::make_unique<VulkanVisual>(*this,
            static_cast<uint32_t>(index), visuals_[index]));
        visual_indices_.emplace(visual_objects_.back().get(), static_cast<uint32_t>(index));
    }
    prepared.device_ = VK_NULL_HANDLE;
    prepared.textures_ = nullptr;
    prepared.pass_ = nullptr;
    prepared.pool_ = VK_NULL_HANDLE;
    error.clear();
    return true;
}

bool GpuLevel::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16]) const
{
    for (GeometryPhase phase : {GeometryPhase::OpaqueAndAlphaTest, GeometryPhase::Transparent})
        if (!record(frame, pass, mvp, phase)) return false;
    return true;
}

bool GpuLevel::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16], GeometryPhase phase) const
{
    for (uint32_t root : roots_)
        if (!record_visual(root, frame, pass, mvp, phase)) return false;
    if (phase == GeometryPhase::OpaqueAndAlphaTest &&
        !record_details(frame, pass, mvp)) return false;
    return true;
}

bool GpuLevel::record_sun_shadow(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&sun_mvp)[16]) const
{
    Fmatrix transform;
    std::memcpy(&transform, sun_mvp, sizeof(transform));
    const auto visible = [&](const LevelVisual& visual)
    {
        const float radius = visual.bounds[9];
        if (!std::isfinite(radius) || radius <= 0.f) return true;
        Fvector4 world, clip;
        world.set(visual.bounds[6], visual.bounds[7], visual.bounds[8], 1.f);
        transform.transform(clip, world);
        const auto extent = [radius](float x, float y, float z)
        { return radius * std::sqrt(x * x + y * y + z * z); };
        return clip.x + clip.w >= -extent(transform._11 + transform._14,
                transform._21 + transform._24, transform._31 + transform._34) &&
            clip.w - clip.x >= -extent(transform._14 - transform._11,
                transform._24 - transform._21, transform._34 - transform._31) &&
            clip.y + clip.w >= -extent(transform._12 + transform._14,
                transform._22 + transform._24, transform._32 + transform._34) &&
            clip.w - clip.y >= -extent(transform._14 - transform._12,
                transform._24 - transform._22, transform._34 - transform._32) &&
            clip.z >= -extent(transform._13, transform._23, transform._33) &&
            clip.w - clip.z >= -extent(transform._14 - transform._13,
                transform._24 - transform._23, transform._34 - transform._33);
    };
    const auto draw = [&](const auto& self, uint32_t index) -> bool
    {
        if (index >= visuals_.size()) return false;
        const LevelVisual& visual = visuals_[index];
        if (!visible(visual)) return true;
        if (visual.mesh >= 0)
        {
            if (size_t(visual.mesh) >= meshes_.size()) return false;
            const Mesh& mesh = meshes_[visual.mesh];
            if (mesh.mode != SurfaceMode::Transparent && mesh.index_count &&
                !pass.record_sun_shadow(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    mesh.index_count, sun_mvp, mesh.material, mesh.mode == SurfaceMode::AlphaTest))
                return false;
        }
        for (uint32_t child : visual.children)
            if (!self(self, child)) return false;
        return true;
    };
    for (uint32_t root : roots_)
        if (!draw(draw, root)) return false;
    return true;
}

namespace
{
uint32_t detail_random(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}
}

void GpuLevel::prepare_details(const Fvector& camera)
{
    visible_details_.clear();
    if (details_.cells.empty() || !g_pGameLevel || !std::isfinite(camera.x) ||
        !std::isfinite(camera.y) || !std::isfinite(camera.z)) return;
    constexpr float max_distance = 40.f;
    const int cx = static_cast<int>(std::floor(camera.x / 2.f)) + details_.offset_x;
    const int cz = static_cast<int>(std::floor(camera.z / 2.f)) + details_.offset_z;
    const int radius = static_cast<int>(max_distance / 2.f) + 1;
    for (int z = std::max(0, cz - radius); z <= std::min(int(details_.height) - 1, cz + radius); ++z)
        for (int x = std::max(0, cx - radius); x <= std::min(int(details_.width) - 1, cx + radius); ++x)
        {
            const size_t index = size_t(z) * details_.width + x;
            const DetailCell& cell = details_.cells[index];
            const float slot_x = float(x - details_.offset_x) * 2.f;
            const float slot_z = float(z - details_.offset_z) * 2.f;
            const float dx = slot_x + 1.f - camera.x, dz = slot_z + 1.f - camera.z;
            if (dx * dx + dz * dz > max_distance * max_distance ||
                std::abs(cell.ground - camera.y) > max_distance + cell.height) continue;
            if (std::all_of(cell.ids.begin(), cell.ids.end(), [](uint8_t id) { return id == 63; })) continue;
            auto found = detail_cache_.find(index);
            if (found == detail_cache_.end())
            {
                std::vector<DetailPlacement> placements;
                for (unsigned rz = 0; rz < 4; ++rz)
                    for (unsigned rx = 0; rx < 4; ++rx)
                    {
                        const uint32_t seed = detail_random(uint32_t(index) ^
                            (uint32_t(rx + rz * 4) * 0x9e3779b9u));
                        const float u = (float(rx) + .5f) / 4.f;
                        const float v = (float(rz) + .5f) / 4.f;
                        float weights[4]{};
                        float sum = 0.f;
                        for (unsigned n = 0; n < 4; ++n)
                        {
                            if (cell.ids[n] == 63) continue;
                            const auto& p = cell.palette[n];
                            weights[n] = (p[0] * (1 - u) + p[1] * u) * (1 - v) +
                                (p[2] * (1 - u) + p[3] * u) * v;
                            sum += weights[n];
                        }
                        if (sum <= 0.f || float(seed & 255u) / 255.f > std::min(1.f, sum / 15.f)) continue;
                        float choice = float((seed >> 8) & 65535u) / 65536.f * sum;
                        unsigned model = 0;
                        for (; model < 3 && choice >= weights[model]; ++model) choice -= weights[model];
                        if (cell.ids[model] >= detail_meshes_.size()) continue;
                        const float jx = float((seed >> 16) & 15u) / 16.f - .5f;
                        const float jz = float((seed >> 20) & 15u) / 16.f - .5f;
                        Fvector origin, down;
                        origin.set(slot_x + (float(rx) + .5f + jx * .5f) * .5f,
                            cell.ground + cell.height + 5.f,
                            slot_z + (float(rz) + .5f + jz * .5f) * .5f);
                        down.set(0.f, -1.f, 0.f);
                        collide::rq_result hit{};
                        if (!g_pGameLevel->ObjectSpace.RayPick(origin, down,
                                cell.height + 10.f, collide::rqtStatic, hit, nullptr)) continue;
                        const DetailPrototype& prototype = details_.prototypes[cell.ids[model]];
                        const float t = float((seed >> 24) & 255u) / 255.f;
                        placements.push_back({cell.ids[model], origin.x, origin.y - hit.range, origin.z,
                            float(seed & 65535u) * (6.2831853f / 65536.f),
                            prototype.min_scale + t * (prototype.max_scale - prototype.min_scale)});
                    }
                found = detail_cache_.emplace(index, std::move(placements)).first;
            }
            visible_details_.insert(visible_details_.end(), found->second.begin(), found->second.end());
        }
    if (detail_cache_.size() > 4096)
    {
        for (auto it = detail_cache_.begin(); it != detail_cache_.end(); )
        {
            const int x = static_cast<int>(it->first % details_.width);
            const int z = static_cast<int>(it->first / details_.width);
            if (std::abs(x - cx) > radius * 2 || std::abs(z - cz) > radius * 2)
                it = detail_cache_.erase(it);
            else ++it;
        }
    }
}

bool GpuLevel::record_details(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16]) const
{
    Fmatrix view;
    static_assert(sizeof(view) == sizeof(float) * 16);
    std::memcpy(&view, mvp, sizeof(view));
    for (const DetailPlacement& placement : visible_details_)
    {
        if (placement.model >= detail_meshes_.size()) return false;
        const Mesh& mesh = detail_meshes_[placement.model];
        Fmatrix rotation, scale, world, transform;
        Fvector position;
        position.set(placement.x, placement.y, placement.z);
        rotation.rotateY(placement.yaw);
        rotation.translate_over(position);
        scale.scale(placement.scale, placement.scale, placement.scale);
        world.mul_43(rotation, scale);
        transform.mul(view, world);
        float matrix[16];
        std::memcpy(matrix, &transform, sizeof(matrix));
        if (!pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                mesh.index_count, matrix, mesh.material, SurfaceMode::AlphaTest, 0,
                "vk\\level_cutout.vs", "vk\\level_cutout.ps")) return false;
    }
    return true;
}

bool GpuLevel::record_visual(size_t index, const FrameRecordingContext& frame,
    const DeferredPass& pass, const float (&mvp)[16], GeometryPhase phase, float lod) const
{
    if (index >= visuals_.size()) return false;
    const LevelVisual& visual = visuals_[index];
    if (visual.type == 6 && lod < .33f)
    {
        Fvector direction;
        direction.set(visual.bounds[6] - Device.vCameraPosition.x,
            visual.bounds[7] - Device.vCameraPosition.y,
            visual.bounds[8] - Device.vCameraPosition.z);
        const auto facet = select_lod_facet(visual.lod_normals, {direction.x, direction.y, direction.z});
        const int32_t mesh_index = visual.lod_facets[facet];
        if (mesh_index < 0 || size_t(mesh_index) >= meshes_.size()) return false;
        const Mesh& mesh = meshes_[mesh_index];
        const bool transparent = mesh.mode == SurfaceMode::Transparent;
        const char* vertex = mesh.lightmapped ? mesh.mode == SurfaceMode::AlphaTest ?
            "vk\\level_lightmap_cutout.vs" : "vk\\level_lightmap.vs" : transparent ? "vk\\object_blended.vs" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.vs" : "vk\\level_opaque.vs";
        const char* fragment = mesh.lightmapped ? mesh.mode == SurfaceMode::AlphaTest ?
            "vk\\level_lightmap_cutout.ps" : "vk\\level_lightmap.ps" : transparent ? "vk\\object_blended.ps" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.ps" : "vk\\level_opaque.ps";
        std::string shader_error;
        const bool named = pass.require_game_pipeline(vertex, fragment, mesh.mode, false, false, shader_error);
        if (!named) { Msg("! Vulkan level: %s", shader_error.c_str()); return false; }
        if (phase == GeometryPhase::Transparent ? transparent : !transparent)
            return mesh.water && transparent ? pass.record_water(frame, mesh.vertices.handle(), mesh.indices.handle(),
                mesh.index_count, mvp, mesh.material, 0, Device.fTimeGlobal, mesh.glass ? -.35f : .72f) :
                transparent ? pass.record_transparent(frame, mesh.vertices.handle(), mesh.indices.handle(),
                mesh.index_count, mvp, mesh.material, 0, named ? vertex : nullptr, named ? fragment : nullptr) :
                pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    mesh.index_count, mvp, mesh.material, mesh.mode, 0, named ? vertex : nullptr, named ? fragment : nullptr);
        return true;
    }
    if (visual.mesh >= 0)
    {
        if (static_cast<size_t>(visual.mesh) >= meshes_.size()) return false;
        const Mesh& base = meshes_[visual.mesh];
        const Mesh& mesh = base.fast && lod < 0.33f ? *base.fast : base;
        const bool transparent = mesh.mode == SurfaceMode::Transparent;
        const bool selected = phase == GeometryPhase::Transparent ? transparent :
            phase == GeometryPhase::OpaqueAndAlphaTest && !transparent;
        const SlideWindow window = select_slide_window(mesh.windows, lod, mesh.index_count);
        const char* vertex = mesh.lightmapped ? mesh.mode == SurfaceMode::AlphaTest ?
            "vk\\level_lightmap_cutout.vs" : "vk\\level_lightmap.vs" : transparent ? "vk\\object_blended.vs" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.vs" :
            (&mesh != &base) ? "vk\\progressive_opaque.vs" :
            (visual.type == 7 || visual.type == 11) ? "vk\\tree_opaque.vs" : "vk\\level_opaque.vs";
        const char* fragment = mesh.lightmapped ? mesh.mode == SurfaceMode::AlphaTest ?
            "vk\\level_lightmap_cutout.ps" : "vk\\level_lightmap.ps" : transparent ? "vk\\object_blended.ps" :
            mesh.mode == SurfaceMode::AlphaTest ? "vk\\level_cutout.ps" : "vk\\level_opaque.ps";
        std::string shader_error;
        const bool named = pass.require_game_pipeline(vertex, fragment, mesh.mode, false, false, shader_error);
        if (!named) { Msg("! Vulkan level: %s", shader_error.c_str()); return false; }
        if (selected && !(transparent ?
                mesh.water ? pass.record_water(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, window.offset, Device.fTimeGlobal,
                    mesh.glass ? -.35f : .72f) :
                pass.record_transparent(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, window.offset,
                    named ? vertex : nullptr, named ? fragment : nullptr) :
                pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, mesh.mode, window.offset,
                    named ? vertex : nullptr, named ? fragment : nullptr))) return false;
    }
    for (uint32_t child : visual.children)
    {
        if (child >= visuals_.size()) return false;
        const auto& bounds = visuals_[child].bounds;
        const float dx = bounds[6] - Device.vCameraPosition.x;
        const float dy = bounds[7] - Device.vCameraPosition.y;
        const float dz = bounds[8] - Device.vCameraPosition.z;
        const float child_lod = lod_for_distance(bounds[9], dx * dx + dy * dy + dz * dz);
        if (!record_visual(child, frame, pass, mvp, phase, child_lod)) return false;
    }
    return true;
}

bool GpuLevel::record_hud_visual(size_t index, const FrameRecordingContext& frame,
    const DeferredPass& pass, const float (&mvp)[16], float lod) const
{
    if (index >= visuals_.size()) return false;
    const LevelVisual& visual = visuals_[index];
    if (visual.type == 6 && lod < .33f)
    {
        Fvector direction;
        direction.set(visual.bounds[6] - Device.vCameraPosition.x,
            visual.bounds[7] - Device.vCameraPosition.y,
            visual.bounds[8] - Device.vCameraPosition.z);
        const auto facet = select_lod_facet(visual.lod_normals, {direction.x, direction.y, direction.z});
        const int32_t mesh_index = visual.lod_facets[facet];
        if (mesh_index < 0 || size_t(mesh_index) >= meshes_.size()) return false;
        const Mesh& mesh = meshes_[mesh_index];
        return pass.record_hud(frame, mesh.vertices.handle(), mesh.indices.handle(),
            mesh.index_count, mvp, mesh.material);
    }
    if (visual.mesh >= 0)
    {
        if (static_cast<size_t>(visual.mesh) >= meshes_.size()) return false;
        const Mesh& base = meshes_[visual.mesh];
        const Mesh& mesh = base.fast && lod < 0.33f ? *base.fast : base;
        const SlideWindow window = select_slide_window(mesh.windows, lod, mesh.index_count);
        if (!pass.record_hud(frame, mesh.vertices.handle(), mesh.indices.handle(),
                window.index_count, mvp, mesh.material, window.offset)) return false;
    }
    for (uint32_t child : visual.children)
        if (!record_hud_visual(child, frame, pass, mvp, lod)) return false;
    return true;
}

IRenderVisual* GpuLevel::get_visual(size_t index) const
{
    return index < visual_objects_.size() ? visual_objects_[index].get() : nullptr;
}

int GpuLevel::find_visual_index(const IRenderVisual* visual) const
{
    const auto it = visual_indices_.find(visual);
    return it == visual_indices_.end() ? -1 : static_cast<int>(it->second);
}

const LevelVisual* GpuLevel::visual_node(size_t index) const
{
    return index < visuals_.size() ? &visuals_[index] : nullptr;
}

bool GpuLevel::visible_sector_roots(size_t camera_sector, const Fmatrix& view_projection,
    const Fvector& camera_position, std::vector<uint32_t>& roots) const
{
    if (!select_visible_sector_roots(sectors_, portals_, visuals_.size(),
            camera_sector, view_projection, camera_position, roots))
        return false;
    append_unsectored_level_roots(roots_, sectors_, visuals_.size(), roots);
    return true;
}

void GpuLevel::all_level_roots(std::vector<uint32_t>& roots) const
{
    roots.clear();
    std::vector<uint8_t> added(visuals_.size(), 0);
    const auto add = [&](uint32_t root)
    {
        if (root < visuals_.size() && !added[root])
        {
            roots.push_back(root);
            added[root] = 1;
        }
    };
    for (uint32_t root : roots_)
        add(root);
    for (const LevelSector& sector : sectors_)
        add(sector.root);
}

void GpuLevel::destroy()
{
    ++revision_;
    if (device_) wait_for_buffer_uploads(device_, pool_, upload_, pending_);
    visual_indices_.clear();
    visual_objects_.clear();
    if (textures_ && pass_)
        for (const Mesh& mesh : meshes_)
        {
            if (mesh.lightmapped) textures_->release_lightmapped_material(mesh.material, *pass_);
            else textures_->release_material(mesh.material, *pass_);
            if (mesh.fast)
            {
                if (mesh.fast->lightmapped) textures_->release_lightmapped_material(mesh.fast->material, *pass_);
                else textures_->release_material(mesh.fast->material, *pass_);
            }
        }
    if (textures_ && pass_)
        for (const Mesh& mesh : detail_meshes_)
            textures_->release_material(mesh.material, *pass_);
    meshes_.clear();
    has_water_ = false;
    detail_meshes_.clear();
    details_ = {};
    detail_cache_.clear();
    visible_details_.clear();
    visuals_.clear();
    roots_.clear();
    sectors_.clear();
    portals_.clear();
    pending_.clear();
    device_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    upload_ = {};
    textures_ = nullptr;
    pass_ = nullptr;
}
}
