#include "xrEngine/stdafx.h"
#include "GpuModel.h"
#include "Include/xrRender/Kinematics.h"
#include "xrCore/Animation/Bone.hpp"

#include <algorithm>
#include <cstring>

namespace xray::render::vulkan
{
bool GpuModel::add_meshes(ModelGeometry&& geometry, const std::string& inherited_texture,
    VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory,
    const BufferUploadDispatch& upload, GameTextureFactory& textures,
    DeferredPass& pass, std::string& error)
{
    const std::string texture = geometry.texture.empty() ? inherited_texture : geometry.texture;
    auto children = std::move(geometry.children);
    geometry.children.clear();
    auto fast = std::move(geometry.fast);
    geometry.fast.reset();
    const auto upload_mesh = [&](ModelGeometry&& decoded) -> bool
    {
        if (texture.empty() || decoded.indices.empty() || decoded.indices.size() > UINT32_MAX)
        {
            error = "OGF model mesh is missing a texture or triangle indices";
            return false;
        }
        meshes_.emplace_back();
        auto &mesh = meshes_.back();
        mesh.geometry = std::move(decoded);
        MaterialDefinition definition;
        if (!textures.surface_mode(mesh.geometry.shader, texture, mesh.geometry.mode, error,
                &mesh.alpha_ref, nullptr, false, false, false, false, nullptr, nullptr, &definition))
        {
            error = "OGF shader='" + mesh.geometry.shader + "' texture='" + texture + "': " + error;
            return false;
        }
        if (definition.class_name == "LmEbB   " || definition.class_name == "LaEmB   ")
        {
            error = "OGF shader='" + mesh.geometry.shader + "' class='" +
                definition.class_name + "' requires level lightmap geometry";
            return false;
        }
        if (definition.class_name == "MODELEbB" && mesh.geometry.mode != SurfaceMode::Transparent)
        {
            error = "OGF shader='" + mesh.geometry.shader + "' class='MODELEbB' "
                "opaque environment pass has no Vulkan model pipeline";
            return false;
        }
        GameTextureFactory::DetailMetadata metadata;
        if (!textures.texture_detail_metadata(texture, metadata, error))
        {
            error = "OGF texture='" + texture + "': " + error;
            return false;
        }
        const bool animated = mesh.geometry.type == 4 || mesh.geometry.type == 5;
        mesh.environment = definition.class_name == "MODELEbB" &&
            mesh.geometry.mode == SurfaceMode::Transparent;
        // Tree surfaces are two-sided by blender contract. A user-controlled
        // shader name must never select a different raster/shader variant.
        mesh.tree_surface = definition.class_name == "D_TREE  ";
        const bool surface_roles = metadata.normal || metadata.bump_mode >= 2 ||
            !metadata.external_normal_texture.empty() || metadata.material != 1 ||
            metadata.material_weight != 0.f;
        mesh.surface = surface_roles;
        const uint16_t roles = mesh.surface ?
            ((metadata.bump_mode >= 2 ? BumpRole | BumpAuxRole | GlossRole : 0u) |
                (metadata.normal || !metadata.external_normal_texture.empty() ? NormalRole : 0u)) : 0u;
        if (!build_render_pass_graph({RenderConsumer::Model, mesh.geometry.mode, false,
                animated, mesh.environment, mesh.tree_surface, mesh.geometry.skin_weights, 1, roles},
                mesh.pass_graph, error)) return false;
        const auto& node = mesh.pass_graph.passes[0];
        const std::string& vertex = node.vertex;
        const std::string& fragment = node.fragment;
        if (!pass.request_game_pipeline(vertex, fragment, mesh.geometry.mode, false, animated, error))
        {
            error = "OGF type=" + std::to_string(mesh.geometry.type) + " shader pair: " + error;
            return false;
        }
        mesh.detail_scale = metadata.scale;
        bool surface_ready = false;
        if (mesh.surface)
        {
            std::array<std::string, 20> names;
            names.fill(texture);
            if (mesh.environment) names[2] = definition.environment_texture;
            if (metadata.bump_mode >= 2)
            { names[7] = metadata.bump_texture; names[8] = metadata.bump_texture + "#"; }
            if (!metadata.external_normal_texture.empty()) names[15] = metadata.external_normal_texture;
            SurfaceParameters surface;
            surface.bump_mode = metadata.bump_mode >= 2 ? float(metadata.bump_mode) : 0.f;
            surface.external_normal = !metadata.external_normal_texture.empty() ? 1.f : 0.f;
            surface.material = float(metadata.material);
            surface.material_weight = metadata.material_weight;
            surface.bump_height = metadata.bump_height;
            if (mesh.environment) surface.terrain_bump = metadata.scale;
            if (metadata.normal)
            {
                GameTextureFactory::DetailMetadata detail_metadata;
                if (!textures.texture_detail_metadata(metadata.texture, detail_metadata, error)) return false;
                names[mesh.environment ? 3 : 2] = metadata.texture;
                names[9] = detail_metadata.bump_mode >= 2 ? detail_metadata.bump_texture : metadata.texture + "_bump";
                names[10] = names[9] + "#";
                surface.detail_normal = metadata.diffuse ? 2.f : 1.f;
            }
            surface_ready = textures.surface_material(names, surface, pass, mesh.material, error);
        }
        if (!(mesh.surface ? surface_ready : mesh.environment ? textures.environment_material(texture, {},
                definition.environment_texture, true, pass, mesh.material, error) :
                textures.material(texture, pass, mesh.material, error,
                    (definition.class_name == "S_SET   " || definition.class_name == "PARTICLE") &&
                    !(definition.screen_flags & 1u))) ||
            !upload_buffer(device, queue, pool, memory, upload, mesh.geometry.indices.data(), mesh.geometry.indices.size() * sizeof(uint32_t),
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indices, pending_, error))
            return false;
        if (animated && mesh.geometry.skin_weights >= 1 && mesh.geometry.skin_weights <= 4)
        {
            const std::string vertex = "vk\\skinned_" + std::to_string(mesh.geometry.skin_weights) + ".vs";
            mesh.gpu_skinning = pass.has_game_pipeline(vertex, fragment);
            for (const ModelVertex& vertex : mesh.geometry.vertices)
                for (unsigned i = 0; i < mesh.geometry.skin_weights; ++i)
                    if (vertex.weights[i] > 0)
                        mesh.max_bone_index = std::max(mesh.max_bone_index, vertex.bones[i]);
            if (mesh.gpu_skinning && !upload_buffer(device, queue, pool, memory, upload,
                    mesh.geometry.vertices.data(), mesh.geometry.vertices.size() * sizeof(ModelVertex),
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.skinned_vertices, pending_, error))
                return false;
        }
        if (!animated)
        {
            std::vector<LevelVertex> vertices;
            vertices.reserve(mesh.geometry.vertices.size());
            for (const auto& source : mesh.geometry.vertices)
            {
                LevelVertex vertex;
                std::copy_n(source.position, 3, vertex.position);
                std::copy_n(source.normal, 3, vertex.normal);
                std::copy_n(source.uv, 2, vertex.uv);
                vertices.push_back(vertex);
            }
            if (vertices.empty() || !upload_buffer(device, queue, pool, memory, upload,
                    vertices.data(), vertices.size() * sizeof(LevelVertex),
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertices[0], pending_, error))
            {
                if (error.empty()) error = "static OGF has no vertices";
                return false;
            }
            // Static OGF vertices never change. Keep one GPU buffer instead
            // of three host-visible copies plus the decoded CPU array.
            std::vector<ModelVertex>().swap(mesh.geometry.vertices);
        }
        return true;
    };
    if (!geometry.vertices.empty())
    {
        if (!upload_mesh(std::move(geometry)))
            return false;
        if (fast)
        {
            const size_t base = meshes_.size() - 1;
            if (!upload_mesh(std::move(*fast)))
                return false;
            meshes_[base].fast_index = meshes_.size() - 1;
            meshes_.back().fast_variant = true;
        }
    }
    for (auto& child : children)
        if (!add_meshes(std::move(child), texture, device, queue, pool,
                memory, upload, textures, pass, error)) return false;
    return true;
}

bool GpuModel::load(const char* name, IReader* source, VkDevice device, VkQueue queue,
    VkCommandPool pool, const VkPhysicalDeviceMemoryProperties& memory,
    const BufferUploadDispatch& upload, GameTextureFactory& textures,
    DeferredPass& pass, std::string& error)
{
    ModelGeometry decoded;
    if (!load_engine_model_geometry(name, source, decoded, error)) return false;
    return load_geometry(std::move(decoded), device, queue, pool, memory, upload, textures, pass, error);
}

bool GpuModel::load_record(const VisualRecord& record, const std::string& inherited_texture,
    VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
    GameTextureFactory& textures, DeferredPass& pass, std::string& error)
{
    ModelGeometry decoded;
    if (!decode_engine_model_geometry(record, decoded, error))
        return false;
    if (decoded.texture.empty()) decoded.texture = inherited_texture;
    return load_geometry(std::move(decoded), device, queue, pool, memory, upload, textures, pass, error);
}

bool GpuModel::load_geometry(ModelGeometry&& decoded, VkDevice device, VkQueue queue,
    VkCommandPool pool, const VkPhysicalDeviceMemoryProperties& memory,
    const BufferUploadDispatch& upload, GameTextureFactory& textures,
    DeferredPass& pass, std::string& error)
{
    destroy();
    if (!device || !queue || !pool)
    {
        error = "OGF model requires a Vulkan device, queue and command pool";
        return false;
    }
    device_ = device;
    pool_ = pool;
    memory_ = memory;
    upload_ = upload;
    textures_ = &textures;
    pass_ = &pass;
    if (!add_meshes(std::move(decoded), "", device, queue, pool, memory,
            upload, textures, pass, error) || meshes_.empty())
    {
        if (error.empty()) error = "OGF model has no renderable meshes";
        destroy();
        return false;
    }
    error.clear();
    return true;
}

bool GpuModel::record(const FrameRecordingContext& frame, const DeferredPass& pass, const float (&mvp)[16], const float* pose, size_t bones, std::string& error,
    GeometryPhase phase, float lod, const IKinematics* instance, const float* normal_rows)
{
    if (!device_ || frame.frame_index >= FrameContext::FramesInFlight)
    {
        error = "Vulkan model has no acquired frame";
        return false;
    }
    for (size_t index = 0; index < meshes_.size(); ++index)
    {
        auto& base = meshes_[index];
        if (base.fast_variant)
            continue;
        // The alternative OGF geometry is the low-detail path. Its own SWI
        // is selected below; the normal mesh remains available for near LOD.
        auto& mesh = base.fast_index != SIZE_MAX && lod < 0.33f ? meshes_[base.fast_index] : base;
        const bool skinned = mesh.geometry.type == 4 || mesh.geometry.type == 5;
        const bool gpu_skinning = skinned && mesh.gpu_skinning;
        std::array<BufferResource, FrameContext::FramesInFlight>* buffers = &mesh.vertices;
        if (skinned)
        {
            if (!instance)
            {
                error = "skinned Vulkan draw has no owning skeleton instance";
                return false;
            }
            if (!gpu_skinning)
            {
                buffers = &mesh.instance_vertices[instance];
                if (!(*buffers)[frame.frame_index].handle())
                    if (!(*buffers)[frame.frame_index].initialize(device_, mesh.geometry.vertices.size() * sizeof(LevelVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memory_, upload_.buffer, error))
                        return false;
            }
        }
        const SlideWindow window = select_slide_window(mesh.geometry.windows, lod,
            mesh.geometry.indices.size());
        if (gpu_skinning)
        {
            const bool hud = phase == GeometryPhase::Hud;
            const bool transparent = mesh.geometry.mode == SurfaceMode::Transparent;
            if (!hud && (phase == GeometryPhase::Transparent) != transparent)
                continue;
            if (!pose || !bones || bones > 256 || mesh.max_bone_index >= bones)
            {
                error = "Vulkan GPU skinning references a bone outside its pose";
                return false;
            }
            auto& per_instance = mesh.poses[instance];
            auto& pose_buffer = per_instance.buffers[frame.frame_index];
            auto& descriptor = per_instance.descriptors[frame.frame_index];
            const size_t pose_bytes = bones * 16 * sizeof(float);
            if (!pose_buffer.handle())
                if (!pose_buffer.initialize(device_, pose_bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        memory_, upload_.buffer, error)) return false;
            if (pose_buffer.size() < pose_bytes)
            {
                error = "Vulkan skeletal pose grew after its descriptor was allocated";
                return false;
            }
            if (!descriptor && !pass_->pose_descriptor(pose_buffer.handle(), pose_buffer.size(), descriptor, error))
                return false;
            if (!pose_buffer.write(0, pose, pose_bytes, error)) return false;
            RenderPassGraph hud_graph;
            if (hud && !build_render_pass_graph({RenderConsumer::Model, mesh.geometry.mode, true,
                    true, mesh.environment, mesh.tree_surface, mesh.geometry.skin_weights},
                    hud_graph, error)) return false;
            const auto& node = hud ? hud_graph.passes[0] : mesh.pass_graph.passes[0];
            const std::string& vertex_name = node.vertex;
            const char* fragment_name = node.fragment.c_str();
            if (!pass.record_skinned(frame, mesh.skinned_vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, descriptor,
                    hud ? SurfaceMode::Opaque : mesh.geometry.mode, hud, window.offset,
                    vertex_name.c_str(), fragment_name, float(mesh.alpha_ref) / 255.f, normal_rows))
            {
                error = "Vulkan skeletal shader pair or render pass is unavailable: " +
                    vertex_name + " / " + fragment_name;
                return false;
            }
            continue;
        }
        if (phase == GeometryPhase::Hud)
        {
            RenderPassGraph hud_graph;
            if (!build_render_pass_graph({RenderConsumer::Model, mesh.geometry.mode, true,
                    false, mesh.environment, mesh.tree_surface}, hud_graph, error)) return false;
            BufferResource* vertex_buffer = &mesh.vertices[0];
            if (skinned)
            {
                std::vector<LevelVertex> vertices;
                if (!skin_model_mesh(mesh.geometry, pose, bones, vertices, error)) return false;
                vertex_buffer = &(*buffers)[frame.frame_index];
                if (!vertex_buffer->write(0, vertices.data(), vertices.size() * sizeof(LevelVertex), error))
                    return false;
            }
            if (!pass.record_hud(frame, vertex_buffer->handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, window.offset,
                    hud_graph.passes[0].vertex.empty() ? nullptr : hud_graph.passes[0].vertex.c_str(),
                    hud_graph.passes[0].fragment.empty() ? nullptr : hud_graph.passes[0].fragment.c_str(),
                    0.f, normal_rows))
            {
                if (error.empty()) error = "Vulkan HUD model geometry recording failed";
                return false;
            }
            continue;
        }
        const bool transparent = mesh.geometry.mode == SurfaceMode::Transparent;
        if ((phase == GeometryPhase::Transparent) != transparent)
            continue;
        const char* vertex = mesh.pass_graph.passes[0].vertex.c_str();
        const char* fragment = mesh.pass_graph.passes[0].fragment.c_str();
        // Both the immutable static buffer and CPU-skinned frame buffers use
        // LevelVertex; pose-buffer variants use a separate GPU layout.
        const bool named = pass.require_game_pipeline(vertex, fragment, mesh.geometry.mode, false, false, error);
        if (!named) return false;
        BufferResource* vertex_buffer = &mesh.vertices[0];
        if (skinned)
        {
            std::vector<LevelVertex> vertices;
            if (!skin_model_mesh(mesh.geometry, pose, bones, vertices, error)) return false;
            vertex_buffer = &(*buffers)[frame.frame_index];
            if (!vertex_buffer->write(0, vertices.data(), vertices.size() * sizeof(LevelVertex), error))
                return false;
        }
        if (!(transparent ? pass.record_transparent(frame, vertex_buffer->handle(), mesh.indices.handle(), window.index_count, mvp,
                                mesh.material, window.offset, named ? vertex : nullptr, named ? fragment : nullptr,
                                nullptr, 0.f, normal_rows) :
                            pass.record_geometry(frame, vertex_buffer->handle(), mesh.indices.handle(), window.index_count, mvp, mesh.material,
                                mesh.geometry.mode, window.offset, named ? vertex : nullptr, named ? fragment : nullptr,
                                float(mesh.alpha_ref) / 255.f, normal_rows, mesh.detail_scale)))
        {
            error = "Vulkan model geometry recording failed";
            return false;
        }
    }
    error.clear();
    return true;
}

bool GpuModel::record_animated(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16], IKinematics& skeleton, std::string& error,
    GeometryPhase phase, float lod, const float* normal_rows)
{
    static_assert(sizeof(Fmatrix) == 16 * sizeof(float));
    skeleton.CalculateBones();
    const size_t count = skeleton.LL_BoneCount();
    if (!count || count > 256)
    {
        error = "animated OGF model has an invalid bone count";
        return false;
    }
    std::vector<float> pose(count * 16);
    for (size_t bone = 0; bone < count; ++bone)
        std::memcpy(pose.data() + bone * 16,
            &skeleton.LL_GetBoneInstance(static_cast<u16>(bone)).mRenderTransform,
            16 * sizeof(float));
    return record(frame, pass, mvp, pose.data(), count, error, phase, lod, &skeleton, normal_rows);
}

void GpuModel::release_instance(const IKinematics* skeleton)
{
    for (auto& mesh : meshes_)
    {
        auto it = mesh.poses.find(skeleton);
        if (it != mesh.poses.end())
        {
            for (auto& descriptor : it->second.descriptors)
                if (pass_) pass_->release_pose_descriptor(descriptor);
            mesh.poses.erase(it);
        }
        mesh.instance_vertices.erase(skeleton);
    }
}

void GpuModel::destroy()
{
    if (device_) wait_for_buffer_uploads(device_, pool_, upload_, pending_);
    if (textures_ && pass_)
        for (auto& mesh : meshes_)
        {
            for (auto& [instance, pose] : mesh.poses)
                for (auto& descriptor : pose.descriptors)
                    pass_->release_pose_descriptor(descriptor);
            if (mesh.environment || mesh.surface) textures_->release_terrain_material(mesh.material, *pass_);
            else textures_->release_material(mesh.material, *pass_);
        }
    meshes_.clear();
    pending_.clear();
    device_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    memory_ = {};
    upload_ = {};
    textures_ = nullptr;
    pass_ = nullptr;
}
}
