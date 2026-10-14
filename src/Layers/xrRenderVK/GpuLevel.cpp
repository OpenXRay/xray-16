#include "xrEngine/stdafx.h"
#include "GpuLevel.h"
#include "LevelVisibility.h"
#include "VulkanVisual.h"
#include "SpecialVisuals.h"
#include "SlidingWindows.h"
#include "xrEngine/device.h"
#if defined(XR_PLATFORM_ANDROID)
#include "xrEngine/x_ray.h"
#endif
#include "xrEngine/IGame_Level.h"
#include "xrCDB/xr_area.h"
#include "xrCDB/Intersect.hpp"
#include "xrMaterialSystem/GameMtlLib.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#if defined(XR_PLATFORM_ANDROID)
#include <unistd.h>
#endif

namespace xray::render::vulkan
{
namespace
{
#if defined(XR_PLATFORM_ANDROID)
unsigned long long descriptor_number(VkDescriptorSet set)
{
    uint64_t bits = 0;
    static_assert(sizeof(set) <= sizeof(bits));
    std::memcpy(&bits, &set, sizeof(set));
    return static_cast<unsigned long long>(bits);
}
size_t current_rss_kib()
{
    FILE* statm = std::fopen("/proc/self/statm", "r");
    if (!statm) return 0;
    unsigned long total = 0, resident = 0;
    const bool parsed = std::fscanf(statm, "%lu %lu", &total, &resident) == 2;
    std::fclose(statm);
    const long page_size = sysconf(_SC_PAGESIZE);
    return parsed && page_size > 0 ? size_t(resident) * size_t(page_size) / 1024 : 0;
}
#endif
bool visible_sphere(const float (&mvp)[16], float x, float y, float z, float radius)
{
    if (!std::isfinite(radius) || radius <= 0.f) return true;
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return true;
    const auto component = [&](int column)
    { return mvp[column] * x + mvp[column + 4] * y + mvp[column + 8] * z + mvp[column + 12]; };
    const float cx = component(0), cy = component(1), cz = component(2), cw = component(3);
    const auto outside = [&](float distance, float a, float b, float c)
    { return distance < -radius * std::sqrt(a * a + b * b + c * c); };
    return !outside(cx + cw, mvp[0] + mvp[3], mvp[4] + mvp[7], mvp[8] + mvp[11]) &&
        !outside(cw - cx, mvp[3] - mvp[0], mvp[7] - mvp[4], mvp[11] - mvp[8]) &&
        !outside(cy + cw, mvp[1] + mvp[3], mvp[5] + mvp[7], mvp[9] + mvp[11]) &&
        !outside(cw - cy, mvp[3] - mvp[1], mvp[7] - mvp[5], mvp[11] - mvp[9]) &&
        !outside(cz, mvp[2], mvp[6], mvp[10]) &&
        !outside(cw - cz, mvp[3] - mvp[2], mvp[7] - mvp[6], mvp[11] - mvp[10]);
}
bool visible_sphere(const float (&mvp)[16], const LevelVisual& visual)
{
    return visible_sphere(mvp, visual.bounds[6], visual.bounds[7],
        visual.bounds[8], visual.bounds[9]);
}

std::string lightmap_texture(const std::string& textures, const MaterialDefinition& definition)
{
    const std::string& cls = definition.class_name;
    if (cls != "LM      " && cls != "LM_AREF " && cls != "BmmD    " && cls != "BmmDold " &&
        cls != "LmBmmD  " && cls != "LmEbB   " && cls != "LaEmB   ") return {};
    const size_t comma = textures.find(',');
    if (comma == std::string::npos) return {};
    const size_t next = textures.find(',', comma + 1);
    std::string name = textures.substr(comma + 1, next == std::string::npos ? next : next - comma - 1);
    const size_t start = name.find_first_not_of(" \t");
    if (start == std::string::npos) return {};
    name.erase(0, start);
    const size_t end = name.find_last_not_of(" \t");
    name.resize(end + 1);
    return name;
}
}
GpuLevel::~GpuLevel() { destroy(); }

void GpuLevel::report_visual_record_failure(size_t visual, int32_t mesh, GeometryPhase phase,
    bool water, uint32_t indices, float lod, const char* reason) const
{
    const std::string cause = reason ? reason : "unknown";
    const std::string key = std::to_string(visual) + "/" + std::to_string(mesh) + "/" +
        std::to_string(static_cast<int>(phase)) + "/" + cause;
    if (++draw_failures_[key] != 1) return;
    const Mesh* resource = mesh >= 0 && size_t(mesh) < meshes_.size() ? &meshes_[mesh] : nullptr;
    const std::string material_id = resource ? std::to_string(resource->material_id) : "unknown";
    Msg("! [renderer-vulkan] file=level.geom visual=%zu material=%s shader='%s' mesh=%d phase=%d water=%d indices=%u lod=%.3f reason=%s (repeats summarized on unload)",
        visual, material_id.c_str(), resource ? resource->shader.c_str() : "unknown",
        mesh, static_cast<int>(phase), water ? 1 : 0, indices, lod, cause.c_str());
}

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
    Msg("[renderer-vulkan] level.geometry decoded models=%zu materials=%zu visuals=%zu",
        models.models.size(), models.materials.size(), models.visuals.size());
    // The export build audits every material before the first GPU upload.
    // One unsupported blender must not hide the rest of the level's material
    // requirements in the next device log.
    if (std::strstr(Core.Params, "-vk_export_shaders_xr"))
    {
        size_t unsupported = 0;
        for (size_t id = 0; id < models.materials.size(); ++id)
        {
            const auto& material = models.materials[id];
            if (material.shader.empty() && material.textures.empty()) continue; // Reserved table slot.
            SurfaceMode mode{};
            std::string issue;
            if (!textures.surface_mode(material.shader, material.textures, mode, issue,
                    nullptr, nullptr, false, false, true))
            {
                ++unsupported;
                Msg("! [renderer-vulkan] level.material.audit id=%zu shader='%s' textures='%s': %s",
                    id, material.shader.c_str(), material.textures.c_str(), issue.c_str());
            }
        }
        Msg("[renderer-vulkan] level.material.audit total=%zu unsupported=%zu",
            models.materials.size(), unsupported);
    }
    DetailAssets details;
    Msg("[renderer-vulkan] level.details decoding");
    if (!load_engine_detail_assets(details, error)) return false;
    Msg("[renderer-vulkan] level.details decoded prototypes=%zu", details.prototypes.size());
    // Portal data is an optimization input. If an old or incomplete level
    // omits it, the renderer uses the complete root list rather than hiding
    // geometry based on partial visibility information.
    std::string visibility_error;
    if (!load_engine_level_visibility(level, models, visibility_error))
        Msg("! Vulkan: level visibility data unavailable (%s); rendering all level roots",
            visibility_error.c_str());
    GpuLevel prepared;
    prepared.device_ = device;
    prepared.memory_ = memory;
    prepared.pool_ = pool;
    prepared.upload_ = upload;
    prepared.textures_ = &textures;
    prepared.pass_ = &pass;
    prepared.visuals_ = std::move(models.visuals);
    prepared.roots_ = std::move(models.roots);
    prepared.sectors_ = std::move(models.sectors);
    prepared.portals_ = std::move(models.portals);
    prepared.meshes_.reserve(models.models.size());
    const auto upload_start = std::chrono::steady_clock::now();
    size_t uploaded_models = 0;
    bool direct_upload = true;
    const auto upload_geometry = [&](const void* data, size_t bytes, VkBufferUsageFlags usage,
        BufferResource& destination) -> bool
    {
#if defined(XR_PLATFORM_ANDROID)
        if (direct_upload)
        {
            if (destination.initialize(device, bytes, usage,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    memory, upload.buffer, error, true))
                return destination.write(0, data, bytes, error);
            if (error != "no Vulkan memory type satisfies the buffer requirements")
                return false;
            direct_upload = false;
            Msg("[renderer-vulkan] level.geometry host-visible buffers unavailable; using staging uploads");
        }
#endif
        return upload_buffer(device, queue, pool, memory, upload, data, bytes, usage,
            destination, prepared.pending_, error, true);
    };
    const auto upload_mesh = [&](const LevelModel& model, Mesh& mesh) -> bool
    {
        if (prepared.pending_.size() >= 128 &&
            !wait_for_buffer_uploads(device, pool, upload, prepared.pending_))
        {
            error = "Vulkan level geometry upload batch did not complete";
            return false;
        }
        if (model.material >= models.materials.size() || model.vertices.empty() ||
            model.indices.empty() || model.indices.size() > std::numeric_limits<uint32_t>::max())
        {
            error = "invalid or empty Vulkan level model";
            return false;
        }
        const auto& material = models.materials[model.material];
        mesh.material_id = model.material;
        mesh.shader = material.shader;
        SurfaceMode mode;
        int blend_mode = -1;
        uint32_t material_flags = 0;
        TerrainMaterialTextures terrain;
        MaterialDefinition definition;
        if (!textures.surface_mode(material.shader, material.textures, mode, error, &mesh.alpha_ref,
                &blend_mode, false, false, true, false, &material_flags, &terrain, &definition))
        {
            error = "level material id=" + std::to_string(model.material) + " (" +
                material.shader + "/" + material.textures + "): " + error;
            return false;
        }
        if (definition.class_name == "MODELEbB")
        {
            error = "level material id=" + std::to_string(model.material) +
                " shader='" + material.shader + "' class='MODELEbB' requires OGF model geometry";
            return false;
        }
        if (definition.class_name == "LaEmB   " && mode != SurfaceMode::Opaque)
        {
            error = "level material id=" + std::to_string(model.material) +
                " shader='" + material.shader + "' class='LaEmB' requires opaque two-pass rendering";
            return false;
        }
        std::string lightmap = lightmap_texture(material.textures, definition);
        const size_t comma = material.textures.find(',');
        const std::string diffuse = material.textures.substr(0, comma);
        std::string detail_texture = definition.detail_texture;
        GameTextureFactory::DetailMetadata metadata;
        const bool implicit_detail = definition.class_name == "LM      " ||
            definition.class_name == "LM_AREF " ||
            definition.class_name == "V       " ||
            definition.class_name == "V_AREF  ";
        if (!textures.texture_detail_metadata(diffuse, metadata, error))
        {
            error = "level material id=" + std::to_string(model.material) +
                " shader='" + material.shader + "': " + error;
            return false;
        }
        if (implicit_detail && (metadata.diffuse || metadata.normal)) detail_texture = metadata.texture;
        if (mode == SurfaceMode::Transparent && detail_texture.empty() &&
            definition.class_name != "LmEbB   ") lightmap.clear();
        if ((terrain.enabled || !definition.detail_texture.empty() ||
                definition.class_name == "LmEbB   " || definition.class_name == "LaEmB   ") && lightmap.empty())
        {
            error = "level material id=" + std::to_string(model.material) +
                " shader='" + material.shader + "' requires lightmap texture slot 1: " + material.textures;
            return false;
        }
        if (!detail_texture.empty() && mode == SurfaceMode::AlphaTest && lightmap.empty() &&
            definition.class_name != "V_AREF  ")
        {
            error = "level material id=" + std::to_string(model.material) +
                " shader='" + material.shader + "': cutout detail requires lightmap texture slot 1";
            return false;
        }
        if (!lightmap.empty() && !model.lightmap_uv)
        {
            error = "level lightmap material has no TEXCOORD1: " + material.textures;
            return false;
        }
        mesh.wallmark_multiply = blend_mode == 6;
        mesh.glow_alpha_add = blend_mode == 5;
        mesh.fog = definition.class_name == "S_SET   " && definition.pass.fog;
        mesh.laemb = definition.class_name == "LaEmB   ";
        mesh.environment = definition.class_name == "LmEbB   " || mesh.laemb;
        mesh.environment_constant = definition.has_environment_constant;
        mesh.constant_waveforms = definition.constant_waveforms;
        mesh.lightmapped = !lightmap.empty();
        mesh.terrain = terrain.enabled;
        mesh.single_detail = !detail_texture.empty();
        mesh.water = mode == SurfaceMode::Transparent &&
            definition.role == MaterialDefinition::Role::Water;
        mesh.glass = false;
        prepared.has_water_ |= mesh.water;
        const uint16_t roles = (metadata.bump_mode >= 2 ? BumpRole | BumpAuxRole | GlossRole : 0u) |
            (!metadata.external_normal_texture.empty() || metadata.normal ? NormalRole : 0u);
        if (!build_render_pass_graph(LevelPassSelection{mode, mesh.lightmapped, mesh.terrain, mesh.single_detail,
                mesh.environment, mesh.laemb, mesh.wallmark_multiply, mesh.glow_alpha_add, mesh.fog, roles,
                definition.class_name == "S_SET   " ? blend_mode : -1},
                mesh.pass_graph, error))
        {
            error = "level / material id=" + std::to_string(model.material) + " shader='" +
                material.shader + "': " + error;
            return false;
        }
        for (uint8_t index = 0; index < mesh.pass_graph.count; ++index)
        {
            const auto& node = mesh.pass_graph.passes[index];
            if (!pass.request_game_pipeline(node.vertex, node.fragment, mode, false, false, error))
            {
                error = "level / material id=" + std::to_string(model.material) + " shader='" +
                    material.shader + "' pass=" + std::to_string(index) + " shader pair: " + error;
                return false;
            }
        }
        if (mesh.terrain) mesh.terrain_scale = metadata.scale;
        if (mesh.single_detail) mesh.terrain_scale = metadata.scale;
        const bool surface_roles = metadata.bump_mode >= 2 || metadata.normal ||
            !metadata.external_normal_texture.empty() || metadata.material != 1 ||
            metadata.material_weight != 0.f || mesh.terrain;
        mesh.surface = surface_roles;
        bool surface_ready = false;
        if (surface_roles)
        {
            std::array<std::string, 20> names;
            names.fill(diffuse);
            names[1] = lightmap.empty() ? diffuse : lightmap;
            if (mesh.environment) names[2] = definition.environment_texture;
            if (mesh.terrain)
            {
                names[2] = diffuse + "_mask";
                for (size_t channel = 0; channel < 4; ++channel)
                {
                    names[3 + channel] = terrain.detail[channel];
                    GameTextureFactory::DetailMetadata channel_metadata;
                    if (!textures.texture_detail_metadata(terrain.detail[channel], channel_metadata, error)) return false;
                    if (channel_metadata.bump_mode >= 2)
                        { names[11 + channel] = channel_metadata.bump_texture;
                          names[16 + channel] = channel_metadata.bump_texture + "#"; }
                }
            }
            if (mesh.single_detail)
            {
                for (size_t slot = 2; slot <= 6; ++slot) names[slot] = detail_texture;
            }
            if (metadata.bump_mode >= 2)
            { names[7] = metadata.bump_texture; names[8] = metadata.bump_texture + "#"; }
            if (!metadata.external_normal_texture.empty()) names[15] = metadata.external_normal_texture;
            SurfaceParameters surface;
            surface.bump_mode = float(metadata.bump_mode >= 2 ? metadata.bump_mode : 0);
            surface.external_normal = !metadata.external_normal_texture.empty() ? 1.f : 0.f;
            surface.material = float(metadata.material);
            surface.material_weight = metadata.material_weight;
            surface.bump_height = metadata.bump_height;
            if (mesh.environment) surface.terrain_bump = metadata.scale;
            for (size_t channel = 0; channel < 4 && mesh.terrain; ++channel)
                if (names[11 + channel] != diffuse) surface.terrain_bump += float(1u << channel);
            if (metadata.normal && (mesh.single_detail || mesh.environment))
            {
                GameTextureFactory::DetailMetadata detail_metadata;
                const std::string normal_detail = mesh.environment ? metadata.texture : detail_texture;
                if (!textures.texture_detail_metadata(normal_detail, detail_metadata, error)) return false;
                if (mesh.environment) names[3] = normal_detail;
                names[9] = detail_metadata.bump_mode >= 2 ? detail_metadata.bump_texture : normal_detail + "_bump";
                names[10] = names[9] + "#";
                surface.detail_normal = metadata.diffuse ? 2.f : 1.f;
            }
            surface_ready = textures.surface_material(names, surface, pass, mesh.material, error,
                mesh.environment && !mesh.laemb,
                definition.class_name == "S_SET   " && definition.pass.clamp);
        }
        const bool material_ready = surface_roles ? surface_ready :
                mesh.terrain ? textures.terrain_material(diffuse, lightmap, terrain, pass, mesh.material, error) :
                mesh.single_detail ? textures.detail_material(diffuse, lightmap.empty() ? diffuse : lightmap, detail_texture,
                    pass, mesh.material, error) :
                mesh.environment ? textures.environment_material(diffuse, lightmap,
                    definition.environment_texture, !mesh.laemb, pass, mesh.material, error) :
                lightmap.empty() ? textures.material(material.textures, pass, mesh.material, error,
                    definition.class_name == "S_SET   " && definition.pass.clamp) :
                textures.lightmapped_material(diffuse, lightmap, pass, mesh.material, error);
        if (!material_ready)
        {
            error = "level material id=" + std::to_string(model.material) + " shader='" +
                material.shader + "' textures='" + material.textures + "': " + error;
            return false;
        }
        if (!upload_geometry(model.vertices.data(), model.vertices.size() * sizeof(LevelVertex),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertices) ||
            !upload_geometry(model.indices.data(), model.indices.size() * sizeof(uint32_t),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indices))
        {
            error = "level material id=" + std::to_string(model.material) +
                " geometry upload: " + error;
            return false;
        }
        mesh.index_count = static_cast<uint32_t>(model.indices.size());
        mesh.windows = model.windows;
        mesh.mode = mode;
        return true;
    };
    size_t released_cpu_bytes = 0;
    for (LevelModel& model : models.models)
    {
        if (uploaded_models % 64 == 0)
            Msg("[renderer-vulkan] level.geometry progress=%zu/%zu pending=%zu elapsed-ms=%lld",
                uploaded_models, models.models.size(), prepared.pending_.size(),
                static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - upload_start).count()));
        prepared.meshes_.emplace_back();
        Mesh& mesh = prepared.meshes_.back();
        if (!upload_mesh(model, mesh))
            return false;
        if (model.fast)
        {
            mesh.fast = std::make_unique<Mesh>();
            if (!upload_mesh(*model.fast, *mesh.fast))
                return false;
            released_cpu_bytes += model.fast->vertices.capacity() * sizeof(LevelVertex) +
                model.fast->indices.capacity() * sizeof(uint32_t);
            model.fast.reset();
        }
        // Both the direct host-visible upload and the staging path have copied
        // the source before returning. Release each decoded mesh now instead
        // of keeping the complete CPU copy next to 41k GPU buffers.
        released_cpu_bytes += model.vertices.capacity() * sizeof(LevelVertex) +
            model.indices.capacity() * sizeof(uint32_t);
        std::vector<LevelVertex>().swap(model.vertices);
        std::vector<uint32_t>().swap(model.indices);
        std::vector<SlideWindow>().swap(model.windows);
        ++uploaded_models;
    }
    Msg("[renderer-vulkan] level.geometry uploaded=%zu elapsed-ms=%lld pending=%zu cpu-released=%zu",
        uploaded_models, static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - upload_start).count()), prepared.pending_.size(), released_cpu_bytes);
#if defined(XR_PLATFORM_ANDROID)
    Msg("[renderer-vulkan] level.geometry resident-current=%zuK resident-peak=%zuK",
        current_rss_kib(), Memory.mem_usage() / 1024);
#endif
    prepared.details_ = std::move(details);
    prepared.detail_meshes_.reserve(prepared.details_.prototypes.size());
    for (const DetailPrototype& prototype : prepared.details_.prototypes)
    {
        if (prepared.pending_.size() >= 128 &&
            !wait_for_buffer_uploads(device, pool, upload, prepared.pending_))
        {
            error = "Vulkan level detail upload batch did not complete";
            return false;
        }
        prepared.detail_meshes_.emplace_back();
        Mesh& mesh = prepared.detail_meshes_.back();
        if (!textures.surface_mode(prototype.shader, prototype.texture, mesh.mode,
                error, &mesh.alpha_ref))
        {
            error = "level.details / " + prototype.shader + ": " + error;
            return false;
        }
        GameTextureFactory::DetailMetadata metadata;
        if (!textures.texture_detail_metadata(prototype.texture, metadata, error))
        {
            error = "level.details / " + prototype.texture + ": " + error;
            return false;
        }
        mesh.single_detail = metadata.diffuse;
        mesh.terrain_scale = metadata.scale;
        const bool surface_roles = metadata.bump_mode >= 2 || metadata.normal ||
            !metadata.external_normal_texture.empty() || metadata.material != 1 ||
            metadata.material_weight != 0.f;
        mesh.surface = surface_roles;
        const uint16_t roles = (metadata.bump_mode >= 2 ? BumpRole | BumpAuxRole | GlossRole : 0u) |
            (metadata.normal || !metadata.external_normal_texture.empty() ? NormalRole : 0u);
        if (!build_vegetation_pass_graph(mesh.mode, mesh.single_detail,
                roles, mesh.pass_graph, error))
        {
            error = "level.details / " + prototype.shader + ": " + error;
            return false;
        }
        const auto& node = mesh.pass_graph.passes[0];
        bool material_ready = false;
        if (surface_roles)
        {
            std::array<std::string, 20> names;
            names.fill(prototype.texture);
            if (mesh.single_detail)
                for (size_t slot = 2; slot <= 6; ++slot) names[slot] = metadata.texture;
            if (metadata.bump_mode >= 2)
            { names[7] = metadata.bump_texture; names[8] = metadata.bump_texture + "#"; }
            if (!metadata.external_normal_texture.empty()) names[15] = metadata.external_normal_texture;
            SurfaceParameters surface;
            surface.bump_mode = metadata.bump_mode >= 2 ? float(metadata.bump_mode) : 0.f;
            surface.external_normal = !metadata.external_normal_texture.empty() ? 1.f : 0.f;
            surface.material = float(metadata.material);
            surface.material_weight = metadata.material_weight;
            surface.bump_height = metadata.bump_height;
            if (metadata.normal)
            {
                GameTextureFactory::DetailMetadata detail;
                if (!textures.texture_detail_metadata(metadata.texture, detail, error))
                {
                    error = "level.details / " + metadata.texture + ": " + error;
                    return false;
                }
                names[9] = detail.bump_mode >= 2 ? detail.bump_texture : metadata.texture + "_bump";
                names[10] = names[9] + "#";
                surface.detail_normal = mesh.single_detail ? 2.f : 1.f;
            }
            material_ready = textures.surface_material(names, surface, pass, mesh.material, error);
        }
        else if (mesh.single_detail)
            material_ready = textures.detail_material(prototype.texture, prototype.texture,
                metadata.texture, pass, mesh.material, error);
        else
            material_ready = textures.material(prototype.texture, pass, mesh.material, error);
        if (!pass.request_game_pipeline(node.vertex, node.fragment,
                mesh.mode, false, false, error) ||
            !material_ready ||
            !upload_geometry(prototype.vertices.data(), prototype.vertices.size() * sizeof(LevelVertex),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertices) ||
            !upload_geometry(prototype.indices.data(), prototype.indices.size() * sizeof(uint32_t),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indices))
        {
            error = "level.details / " + prototype.texture + ": " + error;
            return false;
        }
        mesh.index_count = static_cast<uint32_t>(prototype.indices.size());
    }
    // The old level may still have been submitted. Its owner must wait for
    // those frames before replacing the level. Uploads within prepared use
    // the same queue as the first draw and preserve staging until retired.
    destroy();
    device_ = prepared.device_;
    memory_ = prepared.memory_;
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
    visual_phases_.assign(visuals_.size(), 0);
    std::vector<uint8_t> phase_state(visuals_.size(), 0);
    const auto phases = [&](const auto& self, size_t index) -> uint8_t
    {
        if (index >= visuals_.size()) return 3;
        if (phase_state[index] == 2) return visual_phases_[index];
        if (phase_state[index] == 1) return 3; // malformed cycle: never omit a pass
        phase_state[index] = 1;
        const LevelVisual& visual = visuals_[index];
        uint8_t mask = 0;
        if (visual.mesh >= 0 && size_t(visual.mesh) < meshes_.size())
            mask |= meshes_[visual.mesh].mode == SurfaceMode::Transparent ? 2 : 1;
        for (int32_t facet : visual.lod_facets)
            if (facet >= 0 && size_t(facet) < meshes_.size())
                mask |= meshes_[facet].mode == SurfaceMode::Transparent ? 2 : 1;
        for (uint32_t child : visual.children) mask |= self(self, child);
        phase_state[index] = 2;
        return visual_phases_[index] = mask;
    };
    for (size_t index = 0; index < visuals_.size(); ++index) phases(phases, index);
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
    Msg("[renderer-vulkan] level geometry buffers ready models=%zu details=%zu pending-uploads=%zu bytes=%llu",
        meshes_.size(), detail_meshes_.size(), pending_.size(),
        static_cast<unsigned long long>(geometry_bytes()));
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
                    mesh.index_count, sun_mvp, mesh.material, mesh.mode == SurfaceMode::AlphaTest,
                    0, float(mesh.alpha_ref) / 255.f))
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
    CDB::COLLIDER collider;
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
                // Query the slot once, then test its triangles for every
                // placement. A ray query per blade causes long frame stalls
                // whenever a new group of cells enters the camera radius.
                Fvector box_center, box_half;
                box_center.set(slot_x + 1.f, cell.ground + cell.height * .5f, slot_z + 1.f);
                box_half.set(1.f, cell.height * .5f + 5.f, 1.f);
                collider.box_query(CDB::OPT_FULL_TEST,
                    g_pGameLevel->ObjectSpace.GetStaticModel(), box_center, box_half);
                const auto* tris = g_pGameLevel->ObjectSpace.GetStaticTris();
                const auto* verts = g_pGameLevel->ObjectSpace.GetStaticVerts();
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
                        // Match the GLES decompressor: ignore passable faces
                        // and intersect only triangles belonging to this slot.
                        float ground = cell.ground - 5.f;
                        for (const auto& hit : *collider.r_get())
                        {
                            const auto& triangle = tris[hit.id];
                            const auto* material = GMLib.GetMaterialByIdx(triangle.material);
                            if (!material || material->Flags.test(SGameMtl::flPassable)) continue;
                            Fvector points[3]{verts[triangle.verts[0]],
                                verts[triangle.verts[1]], verts[triangle.verts[2]]};
                            float u, v, range;
                            if (CDB::TestRayTri(origin, down, points, u, v, range, TRUE) &&
                                range >= 0.f && range <= cell.height + 10.f)
                            {
                                const float height = origin.y - range;
                                if (height >= cell.ground && height <= cell.ground + cell.height + .1f)
                                    ground = std::max(ground, height);
                            }
                        }
                        if (ground < cell.ground) continue;
                        const DetailPrototype& prototype = details_.prototypes[cell.ids[model]];
                        const float t = float((seed >> 24) & 255u) / 255.f;
                        placements.push_back({cell.ids[model], origin.x, ground, origin.z,
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
    if (frame.frame_index >= detail_batches_.size()) return false;
    // Geometry is in world space. If the same placements survive frustum
    // culling, re-use the frame slot's upload even when the camera moves.
    uint64_t signature = 14695981039346656037ull;
    for (size_t i = 0; i < visible_details_.size(); ++i)
    {
        const DetailPlacement& p = visible_details_[i];
        if (!visible_sphere(mvp, p.x, p.y, p.z, 6.f)) continue;
        signature = (signature ^ (uint64_t(i) << 8 | p.model)) * 1099511628211ull;
        // Include coordinates: the visible list can start at another cell
        // while retaining the same number and model ordering.
        uint32_t bits{};
        std::memcpy(&bits, &p.x, sizeof(bits));
        signature = (signature ^ bits) * 1099511628211ull;
        std::memcpy(&bits, &p.z, sizeof(bits));
        signature = (signature ^ bits) * 1099511628211ull;
    }
    auto& buffers = detail_batches_[frame.frame_index];
    if (detail_signatures_[frame.frame_index] == signature && !buffers.empty())
    {
        for (size_t model = 0; model < buffers.size(); ++model)
        {
            const DetailBatch& buffer = buffers[model];
            if (!buffer.index_count) continue;
            const Mesh& mesh = detail_meshes_[model];
            if (mesh.mode == SurfaceMode::Transparent) continue;
            const auto& node = mesh.pass_graph.passes[0];
            if (!pass.record_geometry(frame, buffer.vertices.handle(), buffer.indices.handle(),
                    buffer.index_count, mvp, mesh.material, mesh.mode, 0,
                    node.vertex.c_str(), node.fragment.c_str(),
                    float(mesh.alpha_ref) / 255.f, nullptr, mesh.terrain_scale)) return false;
        }
        return true;
    }
    struct Geometry { std::vector<LevelVertex> vertices; std::vector<uint32_t> indices; };
    std::vector<Geometry> grouped(details_.prototypes.size());
    for (const DetailPlacement& placement : visible_details_)
    {
        if (placement.model >= detail_meshes_.size()) return false;
        if (detail_meshes_[placement.model].mode == SurfaceMode::Transparent) continue;
        if (!visible_sphere(mvp, placement.x, placement.y, placement.z, 6.f)) continue;
        const DetailPrototype& prototype = details_.prototypes[placement.model];
        Geometry& batch = grouped[placement.model];
        if (batch.vertices.size() > UINT32_MAX - prototype.vertices.size()) return false;
        const uint32_t first = static_cast<uint32_t>(batch.vertices.size());
        Fmatrix rotation, scale, world;
        Fvector position;
        position.set(placement.x, placement.y, placement.z);
        rotation.rotateY(placement.yaw);
        rotation.translate_over(position);
        scale.scale(placement.scale, placement.scale, placement.scale);
        world.mul_43(rotation, scale);
        for (const LevelVertex& original : prototype.vertices)
        {
            LevelVertex vertex = original;
            Fvector local, transformed, normal;
            local.set(original.position[0], original.position[1], original.position[2]);
            world.transform_tiny(transformed, local);
            local.set(original.normal[0], original.normal[1], original.normal[2]);
            rotation.transform_dir(normal, local);
            normal.normalize_safe();
            std::copy_n(&transformed.x, 3, vertex.position);
            std::copy_n(&normal.x, 3, vertex.normal);
            batch.vertices.push_back(vertex);
        }
        for (uint32_t index : prototype.indices) batch.indices.push_back(first + index);
    }
    if (buffers.size() != grouped.size()) buffers.resize(grouped.size());
    const auto write_buffer = [this](BufferResource& buffer, const void* data,
        size_t bytes, VkBufferUsageFlags usage, std::string& error)
    {
        if (buffer.size() < bytes && !buffer.initialize(device_,
                std::max<VkDeviceSize>(bytes, buffer.size() ? buffer.size() * 2 : 4096),
                usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memory_, upload_.buffer, error)) return false;
        return buffer.write(0, data, bytes, error);
    };
    for (size_t model = 0; model < grouped.size(); ++model)
    {
        const Geometry& batch = grouped[model];
        buffers[model].index_count = 0;
        if (batch.indices.empty()) continue;
        DetailBatch& buffer = buffers[model];
        std::string error;
        if (!write_buffer(buffer.vertices, batch.vertices.data(),
                batch.vertices.size() * sizeof(LevelVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, error) ||
            !write_buffer(buffer.indices, batch.indices.data(),
                batch.indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, error))
        {
            Msg("! [renderer-vulkan] detail batch upload: %s", error.c_str());
            return false;
        }
        buffer.index_count = static_cast<uint32_t>(batch.indices.size());
        const Mesh& mesh = detail_meshes_[model];
        const auto& node = mesh.pass_graph.passes[0];
        if (!pass.record_geometry(frame, buffer.vertices.handle(), buffer.indices.handle(),
                static_cast<uint32_t>(batch.indices.size()), mvp, mesh.material,
                mesh.mode, 0, node.vertex.c_str(), node.fragment.c_str(),
                float(mesh.alpha_ref) / 255.f, nullptr, mesh.terrain_scale)) return false;
    }
    detail_signatures_[frame.frame_index] = signature;
    return true;
}

bool GpuLevel::prepare_transparent_details(const FrameRecordingContext& frame,
    const float (&mvp)[16], std::vector<float>& sort_distances) const
{
    if (frame.frame_index >= transparent_detail_batches_.size()) return false;
    sort_distances.clear();
    auto& draws = transparent_detail_draws_[frame.frame_index];
    draws.clear();
    std::vector<const DetailPlacement*> visible;
    visible.reserve(visible_details_.size());
    for (const DetailPlacement& placement : visible_details_)
    {
        if (placement.model >= detail_meshes_.size()) return false;
        if (detail_meshes_[placement.model].mode == SurfaceMode::Transparent &&
            visible_sphere(mvp, placement.x, placement.y, placement.z, 6.f))
            visible.push_back(&placement);
    }
    const auto distance = [](const DetailPlacement* placement)
    {
        const float dx = placement->x - Device.vCameraPosition.x;
        const float dy = placement->y - Device.vCameraPosition.y;
        const float dz = placement->z - Device.vCameraPosition.z;
        return dx * dx + dy * dy + dz * dz;
    };
    if (visible.empty()) return true;

    std::vector<LevelVertex> vertices;
    std::vector<uint32_t> indices;
    for (const DetailPlacement* placement : visible)
    {
        const DetailPrototype& prototype = details_.prototypes[placement->model];
        if (vertices.size() > UINT32_MAX - prototype.vertices.size() ||
            indices.size() > UINT32_MAX - prototype.indices.size()) return false;
        draws.push_back({placement->model, static_cast<uint32_t>(indices.size()),
            static_cast<uint32_t>(prototype.indices.size())});
        sort_distances.push_back(distance(placement));
        const uint32_t first = static_cast<uint32_t>(vertices.size());
        Fmatrix rotation, scale, world;
        Fvector position;
        position.set(placement->x, placement->y, placement->z);
        rotation.rotateY(placement->yaw);
        rotation.translate_over(position);
        scale.scale(placement->scale, placement->scale, placement->scale);
        world.mul_43(rotation, scale);
        for (const LevelVertex& original : prototype.vertices)
        {
            LevelVertex vertex = original;
            Fvector local, transformed, normal;
            local.set(original.position[0], original.position[1], original.position[2]);
            world.transform_tiny(transformed, local);
            local.set(original.normal[0], original.normal[1], original.normal[2]);
            rotation.transform_dir(normal, local);
            normal.normalize_safe();
            std::copy_n(&transformed.x, 3, vertex.position);
            std::copy_n(&normal.x, 3, vertex.normal);
            vertices.push_back(vertex);
        }
        for (uint32_t index : prototype.indices) indices.push_back(first + index);
    }
    auto& batches = transparent_detail_batches_[frame.frame_index];
    if (batches.empty()) batches.resize(1);
    auto& batch = batches[0];
    const auto write = [this](BufferResource& buffer, const void* data,
        size_t bytes, VkBufferUsageFlags usage, std::string& error)
    {
        if (buffer.size() < bytes && !buffer.initialize(device_,
                std::max<VkDeviceSize>(bytes, buffer.size() ? buffer.size() * 2 : 4096),
                usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                memory_, upload_.buffer, error)) return false;
        return buffer.write(0, data, bytes, error);
    };
    std::string error;
    if (!write(batch.vertices, vertices.data(), vertices.size() * sizeof(LevelVertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, error) ||
        !write(batch.indices, indices.data(), indices.size() * sizeof(uint32_t),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, error))
    {
        Msg("! [renderer-vulkan] transparent detail upload: %s", error.c_str());
        return false;
    }
    return true;
}

bool GpuLevel::record_transparent_detail(size_t draw_index, const FrameRecordingContext& frame,
    const DeferredPass& pass, const float (&mvp)[16]) const
{
    if (frame.frame_index >= transparent_detail_batches_.size() ||
        draw_index >= transparent_detail_draws_[frame.frame_index].size() ||
        transparent_detail_batches_[frame.frame_index].empty()) return false;
    const auto& draw = transparent_detail_draws_[frame.frame_index][draw_index];
    const auto& batch = transparent_detail_batches_[frame.frame_index][0];
    const Mesh& mesh = detail_meshes_[draw.model];
    const auto& node = mesh.pass_graph.passes[0];
    {
        const char* failure = nullptr;
        if (!pass.record_transparent(frame, batch.vertices.handle(), batch.indices.handle(),
                draw.count, mvp, mesh.material, draw.first,
                node.vertex.c_str(), node.fragment.c_str(), &failure,
                float(mesh.alpha_ref) / 255.f, nullptr, mesh.terrain_scale))
        {
            Msg("! [renderer-vulkan] transparent detail shader='%s' texture='%s': %s",
                details_.prototypes[draw.model].shader.c_str(),
                details_.prototypes[draw.model].texture.c_str(), failure ? failure : "unknown");
            return false;
        }
    }
    return true;
}

bool GpuLevel::record_visual(size_t index, const FrameRecordingContext& frame,
    const DeferredPass& pass, const float (&mvp)[16], GeometryPhase phase, float lod,
    bool cull_static) const
{
    if (index >= visuals_.size()) return false;
    if (index < visual_phases_.size() &&
        !(visual_phases_[index] & (phase == GeometryPhase::Transparent ? 2 : 1)))
        return true;
    const LevelVisual& visual = visuals_[index];
    // Hierarchy bounds in older level archives are not guaranteed to enclose
    // every child. Cull leaf meshes, but always descend into child visuals.
    if (cull_static && visual.children.empty() && !visible_sphere(mvp, visual))
    {
        static std::atomic<uint64_t> culled{0};
        const uint64_t count = ++culled;
        if (count <= 4 || count % 1048576 == 0)
            Msg("[renderer-vulkan] level.frustum culled=%llu last-visual=%zu phase=%d",
                static_cast<unsigned long long>(count), index, static_cast<int>(phase));
        return true;
    }
#if defined(XR_PLATFORM_ANDROID)
    if (Device.dwPrecacheFrame)
    {
        char context[96];
        std::snprintf(context, sizeof(context), "vulkan level visual=%zu mesh=%d phase=%d",
            index, visual.mesh, static_cast<int>(phase));
        android_set_load_context(context);
    }
#endif
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
        if ((phase == GeometryPhase::Transparent) != transparent) return true;
        const auto& first = mesh.pass_graph.passes[0];
        const char* vertex = first.vertex_for_visual(false, visual.type);
        const char* fragment = first.fragment.c_str();
        std::string shader_error;
        const bool named = pass.require_game_pipeline(vertex, fragment, mesh.mode, false, false, shader_error);
        if (!named) { Msg("! Vulkan level: %s", shader_error.c_str()); return false; }
#if defined(XR_PLATFORM_ANDROID)
        if (Device.dwPrecacheFrame)
        {
            char context[192];
            std::snprintf(context, sizeof(context),
                "vulkan level visual=%zu mesh=%d phase=%d water=%d indices=%u set=%llu",
                index, mesh_index, static_cast<int>(phase), mesh.water ? 1 : 0,
                mesh.index_count, descriptor_number(mesh.material));
            android_set_load_context(context);
        }
#endif
        const char* failure = nullptr;
        const float camera[4]{Device.vCameraPosition.x, Device.vCameraPosition.y,
            Device.vCameraPosition.z, 0.f};
        float factor[4]{1.f, 1.f, 1.f, 1.f};
        if (mesh.environment_constant)
            for (unsigned channel = 0; channel < 4; ++channel)
            {
                WaveForm waveform = mesh.constant_waveforms[channel];
                factor[channel] = waveform.Calculate(Device.fTimeGlobal);
            }
        const bool recorded = mesh.water && transparent ? pass.record_water(frame, mesh.vertices.handle(), mesh.indices.handle(),
            mesh.index_count, mvp, mesh.material, 0, Device.fTimeGlobal, mesh.glass ? -.35f : .72f) :
            transparent ? pass.record_transparent(frame, mesh.vertices.handle(), mesh.indices.handle(),
            mesh.index_count, mvp, mesh.material, 0, named ? vertex : nullptr, named ? fragment : nullptr,
                &failure, float(mesh.alpha_ref) / 255.f, nullptr, mesh.terrain_scale) :
            pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                mesh.index_count, mvp, mesh.material, mesh.mode, 0,
                named ? vertex : nullptr, named ? fragment : nullptr,
                float(mesh.alpha_ref) / 255.f, nullptr, mesh.terrain_scale,
                mesh.environment ? camera : nullptr, mesh.laemb ? factor : nullptr);
        const bool completed = recorded && (mesh.pass_graph.count < 2 ||
            pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                mesh.index_count, mvp, mesh.material, mesh.mode, 0,
                mesh.pass_graph.passes[1].vertex.c_str(), mesh.pass_graph.passes[1].fragment.c_str()));
        if (!completed) report_visual_record_failure(index, mesh_index, phase,
            mesh.water, mesh.index_count, lod, failure);
        return completed;
    }
    if (visual.mesh >= 0)
    {
        if (static_cast<size_t>(visual.mesh) >= meshes_.size()) return false;
        const Mesh& base = meshes_[visual.mesh];
        const Mesh& mesh = base.fast && lod < 0.33f ? *base.fast : base;
        const bool transparent = mesh.mode == SurfaceMode::Transparent;
        const bool selected = phase == GeometryPhase::Transparent ? transparent :
            phase == GeometryPhase::OpaqueAndAlphaTest && !transparent;
        const char* failure = nullptr;
        const SlideWindow window = select_slide_window(mesh.windows, lod, mesh.index_count);
        const auto& first = mesh.pass_graph.passes[0];
        const char* vertex = first.vertex_for_visual(&mesh != &base, visual.type);
        const char* fragment = first.fragment.c_str();
        if (selected)
        {
            const float camera[4]{Device.vCameraPosition.x, Device.vCameraPosition.y,
                Device.vCameraPosition.z, 0.f};
            float factor[4]{1.f, 1.f, 1.f, 1.f};
            if (mesh.environment_constant)
                for (unsigned channel = 0; channel < 4; ++channel)
                {
                    WaveForm waveform = mesh.constant_waveforms[channel];
                    factor[channel] = waveform.Calculate(Device.fTimeGlobal);
                }
            std::string shader_error;
            const bool named = pass.require_game_pipeline(vertex, fragment, mesh.mode, false, false, shader_error);
            if (!named) { Msg("! Vulkan level: %s", shader_error.c_str()); return false; }
#if defined(XR_PLATFORM_ANDROID)
            if (Device.dwPrecacheFrame)
            {
                char context[192];
                std::snprintf(context, sizeof(context),
                    "vulkan level visual=%zu mesh=%d phase=%d water=%d first=%u count=%u set=%llu",
                    index, visual.mesh, static_cast<int>(phase), mesh.water ? 1 : 0,
                    window.offset, window.index_count,
                    descriptor_number(mesh.material));
                android_set_load_context(context);
            }
#endif
            if (!(transparent ?
                mesh.water ? pass.record_water(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, window.offset, Device.fTimeGlobal,
                    mesh.glass ? -.35f : .72f) :
                pass.record_transparent(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, window.offset,
                    named ? vertex : nullptr, named ? fragment : nullptr,
                    &failure, float(mesh.alpha_ref) / 255.f, nullptr, mesh.terrain_scale) :
                pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, mesh.mode, window.offset,
                    named ? vertex : nullptr, named ? fragment : nullptr,
                    float(mesh.alpha_ref) / 255.f, nullptr, mesh.terrain_scale,
                    mesh.environment ? camera : nullptr, mesh.laemb ? factor : nullptr)))
            {
                report_visual_record_failure(index, visual.mesh, phase, mesh.water, window.index_count, lod, failure);
                return false;
            }
            if (mesh.pass_graph.count > 1 && !pass.record_geometry(frame, mesh.vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, mesh.mode, window.offset,
                    mesh.pass_graph.passes[1].vertex.c_str(), mesh.pass_graph.passes[1].fragment.c_str()))
            {
                report_visual_record_failure(index, visual.mesh, phase, false, window.index_count, lod,
                    "material pass 1 multiply failed");
                return false;
            }
        }
    }
    for (uint32_t child : visual.children)
    {
        if (child >= visuals_.size()) return false;
        const auto& bounds = visuals_[child].bounds;
        const float dx = bounds[6] - Device.vCameraPosition.x;
        const float dy = bounds[7] - Device.vCameraPosition.y;
        const float dz = bounds[8] - Device.vCameraPosition.z;
        const float child_lod = lod_for_distance(bounds[9], dx * dx + dy * dy + dz * dz);
        if (!record_visual(child, frame, pass, mvp, phase, child_lod, cull_static)) return false;
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
    for (const auto& [key, count] : draw_failures_)
        if (count > 1)
            Msg("! [renderer-vulkan] file=level.geom repeated draw failure key='%s' occurrences=%llu",
                key.c_str(), static_cast<unsigned long long>(count));
    draw_failures_.clear();
    ++revision_;
    if (device_) wait_for_buffer_uploads(device_, pool_, upload_, pending_);
    visual_indices_.clear();
    visual_objects_.clear();
    if (textures_ && pass_)
        for (const Mesh& mesh : meshes_)
        {
            if (mesh.surface || mesh.terrain || mesh.single_detail || mesh.environment) textures_->release_terrain_material(mesh.material, *pass_);
            else if (mesh.lightmapped) textures_->release_lightmapped_material(mesh.material, *pass_);
            else textures_->release_material(mesh.material, *pass_);
            if (mesh.fast)
            {
                if (mesh.fast->surface || mesh.fast->terrain || mesh.fast->single_detail || mesh.fast->environment) textures_->release_terrain_material(mesh.fast->material, *pass_);
                else if (mesh.fast->lightmapped) textures_->release_lightmapped_material(mesh.fast->material, *pass_);
                else textures_->release_material(mesh.fast->material, *pass_);
            }
        }
    if (textures_ && pass_)
        for (const Mesh& mesh : detail_meshes_)
            if (mesh.surface || mesh.single_detail) textures_->release_terrain_material(mesh.material, *pass_);
            else textures_->release_material(mesh.material, *pass_);
    meshes_.clear();
    has_water_ = false;
    detail_meshes_.clear();
    details_ = {};
    detail_cache_.clear();
    visible_details_.clear();
    for (auto& buffers : detail_batches_) buffers.clear();
    for (auto& buffers : transparent_detail_batches_) buffers.clear();
    for (auto& draws : transparent_detail_draws_) draws.clear();
    detail_signatures_.fill(0);
    visuals_.clear();
    visual_phases_.clear();
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
