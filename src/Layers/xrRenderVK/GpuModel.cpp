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
        const bool animated = mesh.geometry.type == 4 || mesh.geometry.type == 5;
        const bool double_sided = mesh.geometry.shader.find("double_sided") != std::string::npos || mesh.geometry.shader.find("two_sided") != std::string::npos;
        const char *family = mesh.geometry.mode == SurfaceMode::Transparent ? "object_blended"
                             : mesh.geometry.mode == SurfaceMode::AlphaTest ? "object_cutout"
                             : double_sided && !animated                    ? "object_double_sided"
                                                                            : "object_opaque";
        const std::string shader = std::string("vk\\") + family;
        const std::string vertex = animated ? "vk\\skinned_" + std::to_string(mesh.geometry.skin_weights) + ".vs" : shader + ".vs";
        const std::string fragment = animated && mesh.geometry.mode == SurfaceMode::Transparent ?
            "vk\\skinned_blended.ps" : shader + ".ps";
        if (!pass.request_game_pipeline(vertex, fragment, mesh.geometry.mode, false, animated, error))
        {
            error = "OGF type=" + std::to_string(mesh.geometry.type) + " shader pair: " + error;
            return false;
        }
        if (!textures.material(texture, pass, mesh.material, error) ||
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
        const auto bytes = mesh.geometry.vertices.size() * sizeof(LevelVertex);
        if (mesh.geometry.type != 4 && mesh.geometry.type != 5)
            for (auto& buffer : mesh.vertices)
                if (!buffer.initialize(device, bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memory, upload.buffer, error))
                    return false;
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
    GeometryPhase phase, float lod, const IKinematics* instance)
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
        const bool gpu_skinning = skinned && mesh.gpu_skinning &&
            (phase != GeometryPhase::Hud || mesh.geometry.skin_weights == 4);
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
            const std::string vertex_name = hud ? "vk\\hud_skinned_4.vs" :
                "vk\\skinned_" + std::to_string(mesh.geometry.skin_weights) + ".vs";
            const char* fragment_name = hud ? "vk\\hud_blended.ps" :
                transparent ? "vk\\skinned_blended.ps" :
                mesh.geometry.mode == SurfaceMode::AlphaTest ? "vk\\object_cutout.ps" : "vk\\object_opaque.ps";
            if (!pass.record_skinned(frame, mesh.skinned_vertices.handle(), mesh.indices.handle(),
                    window.index_count, mvp, mesh.material, descriptor,
                    hud ? SurfaceMode::Opaque : mesh.geometry.mode, hud, window.offset,
                    vertex_name.c_str(), fragment_name))
            {
                error = "Vulkan skeletal shader pair or render pass is unavailable: " +
                    vertex_name + " / " + fragment_name;
                return false;
            }
            continue;
        }
        if (phase == GeometryPhase::Hud)
        {
            std::vector<LevelVertex> vertices;
            if (skinned)
            {
                if (!skin_model_mesh(mesh.geometry, pose, bones, vertices, error)) return false;
            }
            else
            {
                vertices.reserve(mesh.geometry.vertices.size());
                for (const auto& vertex : mesh.geometry.vertices)
                {
                    LevelVertex result;
                    std::copy_n(vertex.position, 3, result.position);
                    std::copy_n(vertex.normal, 3, result.normal);
                    std::copy_n(vertex.uv, 2, result.uv);
                    vertices.push_back(result);
                }
            }
            if (!(*buffers)[frame.frame_index].write(0, vertices.data(), vertices.size() * sizeof(LevelVertex), error) ||
                !pass.record_hud(frame, (*buffers)[frame.frame_index].handle(), mesh.indices.handle(), window.index_count, mvp, mesh.material, window.offset))
            {
                if (error.empty()) error = "Vulkan HUD model geometry recording failed";
                return false;
            }
            continue;
        }
        const bool transparent = mesh.geometry.mode == SurfaceMode::Transparent;
        if ((phase == GeometryPhase::Transparent) != transparent)
            continue;
        const bool double_sided = mesh.geometry.shader.find("double_sided") != std::string::npos ||
            mesh.geometry.shader.find("two_sided") != std::string::npos;
        const char* vertex = transparent ? "vk\\object_blended.vs" :
            mesh.geometry.mode == SurfaceMode::AlphaTest ? "vk\\object_cutout.vs" :
            double_sided ? "vk\\object_double_sided.vs" : "vk\\object_opaque.vs";
        const char* fragment = transparent ? "vk\\object_blended.ps" :
            mesh.geometry.mode == SurfaceMode::AlphaTest ? "vk\\object_cutout.ps" :
            double_sided ? "vk\\object_double_sided.ps" : "vk\\object_opaque.ps";
        // The geometry was CPU-skinned into LevelVertex for this frame. The
        // Vulkan game pair therefore receives exactly the same input layout
        // as static meshes; pose-buffer variants use a separate GPU layout.
        const bool named = pass.require_game_pipeline(vertex, fragment, mesh.geometry.mode, false, false, error);
        if (!named) return false;
        std::vector<LevelVertex> vertices;
        if (skinned)
        {
            if (!skin_model_mesh(mesh.geometry, pose, bones, vertices, error)) return false;
        }
        else
        {
            vertices.reserve(mesh.geometry.vertices.size());
            for (const auto& vertex : mesh.geometry.vertices)
            {
                LevelVertex result;
                std::copy_n(vertex.position, 3, result.position);
                std::copy_n(vertex.normal, 3, result.normal);
                std::copy_n(vertex.uv, 2, result.uv);
                vertices.push_back(result);
            }
        }
        if (!(*buffers)[frame.frame_index].write(0, vertices.data(), vertices.size() * sizeof(LevelVertex), error))
            return false;
        if (!(transparent ? pass.record_transparent(frame, (*buffers)[frame.frame_index].handle(), mesh.indices.handle(), window.index_count, mvp,
                                mesh.material, window.offset, named ? vertex : nullptr, named ? fragment : nullptr) :
                            pass.record_geometry(frame, (*buffers)[frame.frame_index].handle(), mesh.indices.handle(), window.index_count, mvp, mesh.material,
                                mesh.geometry.mode, window.offset, named ? vertex : nullptr, named ? fragment : nullptr)))
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
    GeometryPhase phase, float lod)
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
    return record(frame, pass, mvp, pose.data(), count, error, phase, lod, &skeleton);
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
            textures_->release_material(mesh.material, *pass_);
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
