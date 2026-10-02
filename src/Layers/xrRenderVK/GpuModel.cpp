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
    if (!geometry.vertices.empty())
    {
        if (texture.empty() || geometry.indices.empty() ||
            geometry.indices.size() > UINT32_MAX)
        {
            error = "OGF model mesh is missing a texture or triangle indices";
            return false;
        }
        meshes_.emplace_back();
        auto& mesh = meshes_.back();
        mesh.geometry = std::move(geometry);
        if (!textures.material(texture, pass, mesh.material, error) ||
            !upload_buffer(device, queue, pool, memory, upload,
                mesh.geometry.indices.data(), mesh.geometry.indices.size() * sizeof(uint32_t),
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indices, pending_, error)) return false;
        const auto bytes = mesh.geometry.vertices.size() * sizeof(LevelVertex);
        for (auto& buffer : mesh.vertices)
            if (!buffer.initialize(device, bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    memory, upload.buffer, error)) return false;
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
    if (!decode_model_geometry(record, decoded, error)) return false;
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

bool GpuModel::record(const FrameRecordingContext& frame, const DeferredPass& pass,
    const float (&mvp)[16], const float* pose, size_t bones, std::string& error,
    GeometryPhase phase, float lod)
{
    if (!device_ || frame.frame_index >= FrameContext::FramesInFlight)
    {
        error = "Vulkan model has no acquired frame";
        return false;
    }
    for (auto& mesh : meshes_)
    {
        const SlideWindow window = select_slide_window(mesh.geometry.windows, lod,
            mesh.geometry.indices.size());
        if (phase == GeometryPhase::Hud)
        {
            std::vector<LevelVertex> vertices;
            if (mesh.geometry.type == 4 || mesh.geometry.type == 5)
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
            if (!mesh.vertices[frame.frame_index].write(0, vertices.data(),
                    vertices.size() * sizeof(LevelVertex), error) ||
                !pass.record_hud(frame, mesh.vertices[frame.frame_index].handle(),
                    mesh.indices.handle(), window.index_count,
                    mvp, mesh.material, window.offset))
            {
                if (error.empty()) error = "Vulkan HUD model geometry recording failed";
                return false;
            }
            continue;
        }
        const bool transparent = mesh.geometry.mode == SurfaceMode::Transparent;
        if ((phase == GeometryPhase::Transparent) != transparent)
            continue;
        std::vector<LevelVertex> vertices;
        if (mesh.geometry.type == 4 || mesh.geometry.type == 5)
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
        if (!mesh.vertices[frame.frame_index].write(0, vertices.data(),
                vertices.size() * sizeof(LevelVertex), error)) return false;
        if (!(transparent ?
                pass.record_transparent(frame, mesh.vertices[frame.frame_index].handle(),
                    mesh.indices.handle(), window.index_count, mvp, mesh.material, window.offset) :
                pass.record_geometry(frame, mesh.vertices[frame.frame_index].handle(),
                    mesh.indices.handle(), window.index_count,
                    mvp, mesh.material, mesh.geometry.mode, window.offset)))
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
    return record(frame, pass, mvp, pose.data(), count, error, phase, lod);
}

void GpuModel::destroy()
{
    if (device_) wait_for_buffer_uploads(device_, pool_, upload_, pending_);
    if (textures_ && pass_)
        for (const Mesh& mesh : meshes_)
            textures_->release_material(mesh.material, *pass_);
    meshes_.clear();
    pending_.clear();
    device_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    upload_ = {};
    textures_ = nullptr;
    pass_ = nullptr;
}
}
