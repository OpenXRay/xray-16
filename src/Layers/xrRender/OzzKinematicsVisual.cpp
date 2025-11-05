#include "stdafx.h"

#include "OzzKinematicsVisual.h"

#include "BufferUtils.h"
#include "FVisual.h"
#include "xrEngine/Render.h"

#include "xrAnimation/OzzConversion.h"
#include "xrCore/FMesh.hpp"
#include "xrCore/xrMemory.h"

#include "xrCommon/xr_string.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

namespace xray::render::RENDER_NAMESPACE
{
using inherited = FHierrarhyVisual;
using XRay::Animation::ConvertOzzMatrixToXRay;

namespace
{
struct OzzGpuVertex
{
    Fvector position;
    Fvector normal;
    Fvector binormal;
    Fvector4 tangent;
    Fvector2 uv;
};

static inline Fvector ConvertOzzVectorToXRayBasis(Fvector value)
{
    value.x = -value.x;
    value.z = -value.z;
    return value;
}

static inline Fvector4 ConvertOzzTangentToXRayBasis(Fvector4 value)
{
    value.x = -value.x;
    value.z = -value.z;
    return value;
}

constexpr VertexElement OzzVertexDecl[] =
{
    { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
    { 0, 24, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
    { 0, 36, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,  0 },
    { 0, 52, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    D3DDECL_END()
};
} // namespace

class COzzSkinnedSurface final : public dxRender_Visual
{
public:
    COzzSkinnedSurface(COzzKinematicsVisual& owner, const ozz::sample::Mesh& mesh);
    ~COzzSkinnedSurface() override = default;

    void Render(CBackend& cmd_list, float, bool) override;
    void Copy(dxRender_Visual* from) override;
    void Release() override;

private:
    struct Influence
    {
        u16 bone_index = 0;
        u16 remap_index = 0;
        float weight = 0.f;
    };

    struct SourceVertex
    {
        Fvector position;
        Fvector normal;
        Fvector4 tangent;
        Fvector2 uv;
        std::array<Influence, 4> influences{};
        u8 influence_count = 0;
    };

private:
    void InitializeGeometry(const ozz::sample::Mesh& mesh);
    void UpdateGeometry();

private:
    COzzKinematicsVisual& owner_;
    xr_vector<SourceVertex> source_vertices_;
    xr_vector<u16> indices_;
    xr_vector<u16> joint_remaps_;
    xr_unique_ptr<VertexStreamBuffer> vertex_buffer_;
    xr_unique_ptr<IndexStagingBuffer> index_buffer_;
    ref_geom geom_;
    u32 vertex_count_ = 0;
    u32 primitive_count_ = 0;
};

static inline Fvector2 ReadUV(const ozz::vector<float>& uvs, int index)
{
    if (uvs.empty())
        return Fvector2().set(0.f, 0.f);
    const int offset = index * ozz::sample::Mesh::Part::kUVsCpnts;
    return Fvector2().set(uvs[offset + 0], uvs[offset + 1]);
}

static inline Fvector ReadVector3(const ozz::vector<float>& data, int index, int components)
{
    if (data.empty())
        return Fvector().set(0.f, 0.f, 0.f);
    const int offset = index * components;
    return Fvector().set(data[offset + 0], data[offset + 1], data[offset + 2]);
}

static inline Fvector4 ReadVector4(const ozz::vector<float>& data, int index, int components)
{
    if (data.empty())
        return Fvector4().set(0.f, 0.f, 0.f, 1.f);
    const int offset = index * components;
    return Fvector4().set(data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3]);
}

COzzSkinnedSurface::COzzSkinnedSurface(COzzKinematicsVisual& owner, const ozz::sample::Mesh& mesh)
    : owner_(owner)
{
    Type = mesh.xray_metadata.ogf_type;
    InitializeGeometry(mesh);
#ifdef DEBUG
    dbg_name = mesh.xray_metadata.texture_path.c_str();
    Msg("Yohji debug - init COzzSkinnedSurface %s", dbg_name.c_str());
#endif
}

void COzzSkinnedSurface::InitializeGeometry(const ozz::sample::Mesh& mesh)
{
    vertex_count_ = static_cast<u32>(mesh.vertex_count());
    primitive_count_ = static_cast<u32>(mesh.triangle_index_count() / 3);

    joint_remaps_.assign(mesh.joint_remaps.begin(), mesh.joint_remaps.end());
    if (joint_remaps_.empty())
        joint_remaps_.push_back(0);

    source_vertices_.resize(vertex_count_);

    const char* shader_id = mesh.xray_metadata.shader_name.empty() ? "default_object" : mesh.xray_metadata.shader_name.c_str();
    const char* texture_id = mesh.xray_metadata.texture_path.empty() ? shader_id : mesh.xray_metadata.texture_path.c_str();
    shader.create(shader_id, texture_id);

    int vertex_base = 0;
    for (const auto& part : mesh.parts)
    {
        const int influences = std::min(part.influences_count(), 4);
        const int vertex_count = part.vertex_count();

        for (int local = 0; local < vertex_count; ++local)
        {
            const int vertex_index = vertex_base + local;
            SourceVertex& dst = source_vertices_[vertex_index];

            dst.position = ConvertOzzVectorToXRayBasis(
                ReadVector3(part.positions, local, ozz::sample::Mesh::Part::kPositionsCpnts));
            dst.normal = ConvertOzzVectorToXRayBasis(
                ReadVector3(part.normals, local, ozz::sample::Mesh::Part::kNormalsCpnts));
            dst.tangent = ConvertOzzTangentToXRayBasis(
                ReadVector4(part.tangents, local, ozz::sample::Mesh::Part::kTangentsCpnts));
            dst.uv = ReadUV(part.uvs, local);

            dst.influence_count = static_cast<u8>(influences == 0 ? 1 : influences);

            float accumulated_weight = 0.f;
            for (int influence = 0; influence < dst.influence_count; ++influence)
            {
                Influence data{};
                const int joint_stride = part.influences_count();
                const int weight_stride = std::max(0, joint_stride - 1);

                if (joint_stride == 0)
                {
                    data.bone_index = 0;
                    data.remap_index = 0;
                    data.weight = 1.f;
                }
                else
                {
                    const int joint_index = part.joint_indices[local * joint_stride + influence];
                    data.remap_index = static_cast<u16>(joint_index);
                    const u16 bone_index = (joint_index < static_cast<int>(mesh.joint_remaps.size())) ? mesh.joint_remaps[joint_index] : 0;
                    data.bone_index = bone_index;

                    if (influence < weight_stride)
                    {
                        data.weight = part.joint_weights[local * weight_stride + influence];
                        accumulated_weight += data.weight;
                    }
                    else
                    {
                        data.weight = std::max(0.f, 1.f - accumulated_weight);
                    }
                }

                dst.influences[influence] = data;
            }

            for (int influence = dst.influence_count; influence < 4; ++influence)
                dst.influences[influence] = Influence{};
        }

        vertex_base += vertex_count;
    }

    indices_.assign(mesh.triangle_indices.begin(), mesh.triangle_indices.end());

    vertex_buffer_ = xr_make_unique<VertexStreamBuffer>();
    vertex_buffer_->Create(static_cast<size_t>(vertex_count_) * sizeof(OzzGpuVertex));

    index_buffer_ = xr_make_unique<IndexStagingBuffer>();
    index_buffer_->Create(static_cast<size_t>(indices_.size()) * sizeof(u16), false, true);
    if (!indices_.empty())
    {
        auto* dst = static_cast<u16*>(index_buffer_->Map());
        std::memcpy(dst, indices_.data(), indices_.size() * sizeof(u16));
        index_buffer_->Unmap(true);
    }

    geom_.create(OzzVertexDecl, *vertex_buffer_, *index_buffer_);

    vis.box.invalidate();
    for (const SourceVertex& vertex : source_vertices_)
    {
        vis.box.modify(vertex.position);
    }
    vis.box.getsphere(vis.sphere.P, vis.sphere.R);
}

void COzzSkinnedSurface::UpdateGeometry()
{
    if (!owner_.IsKinematicsReady())
    {
        return;
    }

    const xr_vector<Fmatrix>& palette = owner_.SkinningPalette();
    OzzKinematics* kinematics = owner_.Kinematics();

    if (source_vertices_.empty() || palette.empty())
        return;

    xr_vector<Fmatrix> remapped_palette(joint_remaps_.size());
    for (size_t idx = 0; idx < joint_remaps_.size(); ++idx)
    {
        const u16 bone_index = joint_remaps_[idx];
        if (bone_index >= palette.size())
        {
            remapped_palette[idx] = Fidentity;
            continue;
        }

        remapped_palette[idx].set(palette[bone_index]);
    }

    if (!vertex_buffer_ || !vertex_buffer_->IsValid() || vertex_count_ == 0)
    {
        return;
    }

    const size_t buffer_size = static_cast<size_t>(vertex_count_) * sizeof(OzzGpuVertex);
    auto* gpu_vertices = static_cast<OzzGpuVertex*>(vertex_buffer_->Map(0, buffer_size, true));
    for (u32 vertex = 0; vertex < vertex_count_; ++vertex)
    {
        const SourceVertex& src = source_vertices_[vertex];
        Fvector skinned_pos = { 0.f, 0.f, 0.f };
        Fvector skinned_normal = { 0.f, 0.f, 0.f };
        Fvector skinned_tangent = { 0.f, 0.f, 0.f };

        for (u8 influence = 0; influence < src.influence_count; ++influence)
        {
            const Influence& data = src.influences[influence];
            if (data.weight <= 0.f)
                continue;

            const Fmatrix& skin = (data.remap_index < remapped_palette.size())
                ? remapped_palette[data.remap_index]
                : Fidentity;

            Fvector contribution;
            skin.transform_tiny(contribution, src.position);
            skinned_pos.mad(contribution, data.weight);

            Fvector tmp_normal = src.normal;
            skin.transform_dir(tmp_normal);
            skinned_normal.mad(tmp_normal, data.weight);

            Fvector tmp_tangent;
            tmp_tangent.set(src.tangent.x, src.tangent.y, src.tangent.z);
            skin.transform_dir(tmp_tangent);
            skinned_tangent.mad(tmp_tangent, data.weight);
        }

        if (skinned_normal.square_magnitude() > EPS_S)
            skinned_normal.normalize();
        if (skinned_tangent.square_magnitude() > EPS_S)
            skinned_tangent.normalize();

        Fvector tangent_dir = skinned_tangent;
        float tangent_sign = src.tangent.w;
        if (tangent_dir.square_magnitude() > EPS_S)
            tangent_dir.normalize();
        else
            tangent_dir.set(0.f, 0.f, 0.f);

        Fvector binormal;
        binormal.crossproduct(skinned_normal, tangent_dir);
        binormal.mul(tangent_sign);

        OzzGpuVertex& dst = gpu_vertices[vertex];
        dst.position = skinned_pos;
        dst.normal = skinned_normal;
        dst.binormal = binormal;
        dst.tangent.set(tangent_dir.x, tangent_dir.y, tangent_dir.z, tangent_sign);
        dst.uv = src.uv;
    }

    vertex_buffer_->Unmap();
}

void COzzSkinnedSurface::Render(CBackend& cmd_list, float, bool)
{
    UpdateGeometry();

    cmd_list.set_Geometry(geom_);
    cmd_list.set_xform_world(cmd_list.xforms.m_w);
    cmd_list.Render(D3DPT_TRIANGLELIST, 0, 0, vertex_count_, 0, primitive_count_);
    cmd_list.stat.r.s_dynamic.add(vertex_count_);
}

void COzzSkinnedSurface::Copy(dxRender_Visual*)
{
    R_ASSERT(false && "COzzSkinnedSurface::Copy should not be invoked directly");
}

void COzzSkinnedSurface::Release()
{
    geom_.destroy();
    vertex_buffer_.reset();
    index_buffer_.reset();
}

COzzKinematicsVisual::COzzKinematicsVisual()
{
    Type = MT_OZZ_ANIMATED;  // Default, will be overwritten from bundle
    initialized_ = false;
    kinematics_ = xr_make_unique<OzzKinematics>();
}

COzzKinematicsVisual::~COzzKinematicsVisual()
{
    if (kinematics_)
    {
        kinematics_->SetUpdateCallback(nullptr);
        kinematics_->SetUpdateCallbackParam(nullptr);
    }

    DestroySurfaces();
}

void COzzKinematicsVisual::DebugDumpPalette(const xr_vector<Fmatrix>& palette) const
{
    if (!AcquirePaletteDumpTicket())
        return;

    if (!kinematics_)
        return;

    const u16 bone_count = kinematics_->LL_BoneCount();
    if (bone_count == 0)
        return;

    xr_vector<Fmatrix> world_palette;
    world_palette.resize(bone_count);
    for (u16 idx = 0; idx < bone_count; ++idx)
        world_palette[idx] = kinematics_->LL_GetTransform(idx);

#ifdef DEBUG
    const char* label = dbg_name.size() ? dbg_name.c_str() : "<ozz_visual>";
#else
    const char* label = "<ozz_visual>";
#endif

    DumpPaletteLog("ozz", label, world_palette);
    DumpPaletteLog("ozz_render", label, palette);
}

void COzzKinematicsVisual::DestroySurfaces()
{
    for (auto* surface : surfaces_)
    {
        if (!surface)
            continue;

        surface->Release();
        xr_delete(surface);
    }
    surfaces_.clear();
}

bool COzzKinematicsVisual::InitializeFromPayload(bool spawn_children)
{
    if (skeleton_payload_.empty())
        return false;

    initialized_ = false;
    is_animated_ = RequiresAnimation();

    // Create appropriate kinematics type
    if (is_animated_)
    {
        OzzKinematicsAnimated* animated = xr_new<OzzKinematicsAnimated>();
        animated_kinematics_ = animated;
        kinematics_.reset(animated);  // Takes ownership
    }
    else
    {
        kinematics_ = xr_make_unique<OzzKinematics>();
        animated_kinematics_ = nullptr;
    }

    kinematics_->SetVisualOwner(this);

    ozz::span<const std::byte> skeleton_span(reinterpret_cast<const std::byte*>(skeleton_payload_.data()), skeleton_payload_.size());

    // Initialize with or without motion refs
    if (is_animated_)
    {
        auto* animated = static_cast<OzzKinematicsAnimated*>(kinematics_.get());

        if (!embedded_animation_payload_.empty())
        {
            Msg("[COzzKinematicsVisual] Setting embedded animation data: %zu bytes", embedded_animation_payload_.size());
            animated->SetEmbeddedAnimationData(embedded_animation_payload_);
        }

        if (!animated->InitializeFromOzzBuffer(skeleton_span, motion_references_))
        {
            Msg("[OzzKinematicsVisual] Failed to initialize animated kinematics from cached payload");
            return false;
        }
    }
    else
    {
        if (!kinematics_->InitializeFromOzzBuffer(skeleton_span))
        {
            Msg("[OzzKinematicsVisual] Failed to initialize kinematics from cached payload");
            return false;
        }
    }

    if (!kinematics_->LoadUserDataFromBuffer(user_data_payload_) && !user_data_payload_.empty())
        Msg("[OzzKinematicsVisual] Failed to initialize user data from bundle payload");

    if (!kinematics_->ApplyExtendedBoneMetadata(bone_metadata_))
        Msg("[OzzKinematicsVisual] Bone metadata application failed; falling back to defaults");

    if (meshes_.empty() && !mesh_payload_.empty())
    {
        ozz::io::MemoryStream mesh_stream;
        if (!mesh_stream.Write(mesh_payload_.data(), mesh_payload_.size()))
        {
            Msg("[OzzKinematicsVisual] Failed to seed mesh stream from cached payload");
            return false;
        }
        mesh_stream.Seek(0, ozz::io::Stream::kSet);

        ozz::io::IArchive archive(&mesh_stream);
        meshes_.clear();
        while (archive.TestTag<ozz::sample::Mesh>())
        {
            meshes_.emplace_back();
            archive >> meshes_.back();
        }
    }

    //DestroySurfaces();
    children.clear();
    surfaces_.reserve(meshes_.size());
    for (const auto& mesh : meshes_)
    {
        auto* surface = xr_new<COzzSkinnedSurface>(*this, mesh);
        children.push_back(surface);
        surfaces_.push_back(surface);
        if (spawn_children)
            surface->Spawn();
    }
    bDontDelete = TRUE;

    kinematics_->SetUpdateCallback(&COzzKinematicsVisual::HandleKinematicsUpdated);
    kinematics_->SetUpdateCallbackParam(this);

    last_animation_update_frame_ = u32(-1);

    initialized_ = true;

    kinematics_->CalculateBones(TRUE);
    OnPoseUpdated();
    EnsureSkinningPalette();

    return true;
}

void COzzKinematicsVisual::Load(const char* N, IReader* data, u32)
{
    R_ASSERT2(false, "COzzKinematicsVisual::Load should not be invoked directly. Use LoadFromBundle().");
}

bool COzzKinematicsVisual::LoadFromBundle(const char* name, const std::filesystem::path& path)
{
#ifdef DEBUG
    dbg_name = name;
#endif

    XRay::Animation::OzzxBundle bundle;
    xr_string error_msg = "Failed to read .ozzx bundle: ";
    error_msg += path.string().c_str();
    R_ASSERT2(XRay::Animation::ReadOzzxBundle(path, bundle), error_msg.c_str());

    R_ASSERT2(!bundle.skeleton.empty(), "Ozz bundle missing skeleton payload");

    // Set visual type from bundle
    Type = bundle.model_type;

    skeleton_payload_.assign(bundle.skeleton.begin(), bundle.skeleton.end());
    mesh_payload_.assign(bundle.mesh.begin(), bundle.mesh.end());
    motion_references_ = bundle.motion_refs;
#ifdef DEBUG
    Msg("[OzzKinematicsVisual] '%s' motion refs: %zu", name, motion_references_.size());
    for (const auto& ref : motion_references_)
        Msg("[OzzKinematicsVisual]   ref '%s'", ref.c_str());
#endif
    bone_metadata_ = bundle.bone_metadata;
    user_data_payload_.assign(bundle.user_data.begin(), bundle.user_data.end());
    embedded_animation_payload_ = bundle.embedded_animation_data;

    Msg("[COzzKinematicsVisual::LoadFromBundle] Bundle '%s' loaded:", name);
    Msg("  - Skeleton: %zu bytes", skeleton_payload_.size());
    Msg("  - Mesh: %zu bytes", mesh_payload_.size());
    Msg("  - Motion refs: %zu", motion_references_.size());
    Msg("  - Embedded animations: %zu bytes", embedded_animation_payload_.size());
    Msg("  - User data: %zu bytes", user_data_payload_.size());

    meshes_.clear();

    xr_string init_error = "Failed to initialize OzzKinematics from bundle: ";
    init_error += path.string().c_str();
    R_ASSERT2(InitializeFromPayload(), init_error.c_str());

#ifdef DEBUG
    Msg("Yohji debug - init COzzKinematicsVisual %s", dbg_name.c_str());
#endif

    return true;
}

void COzzKinematicsVisual::Copy(dxRender_Visual* pFrom)
{
    dxRender_Visual::Copy(pFrom);

    auto* other = dynamic_cast<COzzKinematicsVisual*>(pFrom);
    if (!other)
    {
        Msg("[COzzKinematicsVisual::Copy] Source is not COzzKinematicsVisual, initializing empty");
        kinematics_ = xr_make_unique<OzzKinematics>();
        animated_kinematics_ = nullptr;
        initialized_ = false;
        return;
    }

    Msg("[COzzKinematicsVisual::Copy] Starting copy from source visual");

    // Copy payload data (needed for re-initialization)
    skeleton_payload_ = other->skeleton_payload_;
    mesh_payload_ = other->mesh_payload_;
    motion_references_ = other->motion_references_;
    bone_metadata_ = other->bone_metadata_;
    user_data_payload_ = other->user_data_payload_;
    embedded_animation_payload_ = other->embedded_animation_payload_;

    // Copy mesh data (shallow copy - meshes are immutable)
    meshes_ = other->meshes_;

    // Copy flags
    initialized_ = false;
    is_animated_ = other->is_animated_;

    // Following ozz-animation multithread sample pattern:
    // - Skeleton is shared (loaded once, passed as const& to jobs)
    // - Each instance has its own runtime state (locals, context, bone instances)
    //
    // Since ozz::animation::Skeleton is non-copyable, we re-initialize from
    // the cached skeleton payload, then share the motion data.

    // Create appropriate kinematics type
    if (is_animated_)
    {
        OzzKinematicsAnimated* animated = xr_new<OzzKinematicsAnimated>();
        animated_kinematics_ = animated;
        kinematics_.reset(animated);
    }
    else
    {
        kinematics_ = xr_make_unique<OzzKinematics>();
        animated_kinematics_ = nullptr;
    }

    kinematics_->SetVisualOwner(this);

    // Re-initialize skeleton from cached payload (creates new instance data)
    ozz::span<const std::byte> skeleton_span(
        reinterpret_cast<const std::byte*>(skeleton_payload_.data()),
        skeleton_payload_.size());

    if (is_animated_)
    {
        auto* animated = static_cast<OzzKinematicsAnimated*>(kinematics_.get());

        // Initialize skeleton first (creates per-instance buffers)
        if (!animated->InitializeFromOzzBuffer(skeleton_span, motion_references_))
        {
            Msg("[COzzKinematicsVisual::Copy] Failed to initialize animated kinematics from payload");
            return;
        }

        // Now copy motion data (shares motion handles, increments refcounts)
        if (other->animated_kinematics_)
        {
            animated->Copy(other->animated_kinematics_);
        }
    }
    else
    {
        if (!kinematics_->InitializeFromOzzBuffer(skeleton_span))
        {
            Msg("[COzzKinematicsVisual::Copy] Failed to initialize kinematics from payload");
            return;
        }
    }

    // Apply metadata and user data
    if (!kinematics_->LoadUserDataFromBuffer(user_data_payload_) && !user_data_payload_.empty())
        Msg("[COzzKinematicsVisual::Copy] Failed to load user data from payload");

    if (!kinematics_->ApplyExtendedBoneMetadata(bone_metadata_))
        Msg("[COzzKinematicsVisual::Copy] Bone metadata application failed");

    // Re-create surfaces (per-instance state)
    DestroySurfaces();
    children.clear();
    surfaces_.reserve(meshes_.size());
    for (const auto& mesh : meshes_)
    {
        auto* surface = xr_new<COzzSkinnedSurface>(*this, mesh);
        children.push_back(surface);
        surfaces_.push_back(surface);
    }
    bDontDelete = TRUE;

    // Set up callbacks
    kinematics_->SetUpdateCallback(&COzzKinematicsVisual::HandleKinematicsUpdated);
    kinematics_->SetUpdateCallbackParam(this);

    last_animation_update_frame_ = u32(-1);
    initialized_ = true;

    // Initialize bone palette
    kinematics_->CalculateBones(TRUE);
    OnPoseUpdated();
    EnsureSkinningPalette();

    Msg("[COzzKinematicsVisual::Copy] Copy complete - skeleton re-initialized, motion data shared");
}

void COzzKinematicsVisual::Spawn()
{
    inherited::Spawn();

    if (!initialized_ && !skeleton_payload_.empty())
    {
        if (!InitializeFromPayload(true))
            Msg("[OzzKinematicsVisual] Failed to restore visual state during Spawn()");
    }
}

void COzzKinematicsVisual::Depart()
{
    initialized_ = false;
    if (kinematics_)
    {
        kinematics_->SetUpdateCallback(nullptr);
        kinematics_->SetUpdateCallbackParam(nullptr);
    }

    DestroySurfaces();
    children.clear();

    inherited::Depart();
}

void COzzKinematicsVisual::Release()
{
    if (kinematics_)
    {
        kinematics_->SetUpdateCallback(nullptr);
        kinematics_->SetUpdateCallbackParam(nullptr);
    }
    kinematics_.reset();
    animated_kinematics_ = nullptr;
    skeleton_payload_.clear();
    mesh_payload_.clear();
    motion_references_.clear();
    user_data_payload_.clear();
    embedded_animation_payload_.clear();
    DestroySurfaces();
    children.clear();
    bone_palette_.clear();
    last_animation_update_frame_ = u32(-1);
    initialized_ = false;
    is_animated_ = false;
    inherited::Release();
}

void COzzKinematicsVisual::UpdateBounds()
{
    if (!kinematics_)
        return;

    const Fbox& box = kinematics_->GetBox();
    vis.box = box;

    if (box.is_valid())
    {
        Fvector center;
        float radius = 0.f;
        vis.box.getsphere(center, radius);
        vis.sphere.P = center;
        vis.sphere.R = radius;
    }
    else
    {
        vis.sphere.P.set(0.f, 0.f, 0.f);
        vis.sphere.R = 0.f;
    }
}

void COzzKinematicsVisual::UpdateSkinningPalette()
{
}

void COzzKinematicsVisual::EnsureSkinningPalette()
{
    if (!initialized_ || !kinematics_ || !kinematics_->IsInitialized())
    {
        bone_palette_.clear();
        return;
    }

    const u32 frame_id = Device.dwFrame;
    if (frame_id != last_animation_update_frame_ && UpdateAnimation(Device.fTimeDelta))
    {
        last_animation_update_frame_ = frame_id;
    }

    if (!kinematics_->HasBones())
    {
        bone_palette_.clear();
        return;
    }

    kinematics_->CalculateBones(TRUE);
    kinematics_->BuildSkinningPalette(bone_palette_, true);
}

const xr_vector<Fmatrix>& COzzKinematicsVisual::SkinningPalette()
{
    EnsureSkinningPalette();
    return bone_palette_;
}

void COzzKinematicsVisual::OnPoseUpdated()
{
    UpdateBounds();
    UpdateSkinningPalette();
}

void COzzKinematicsVisual::HandleKinematicsUpdated(IKinematics* kin)
{
    auto* visual = static_cast<COzzKinematicsVisual*>(kin ? kin->GetUpdateCallbackParam() : nullptr);
    if (!visual)
        return;
    visual->OnPoseUpdated();
}

IKinematics* COzzKinematicsVisual::dcast_PKinematics()
{
    return kinematics_.get();
}

IKinematicsAnimated* COzzKinematicsVisual::dcast_PKinematicsAnimated()
{
    return animated_kinematics_;
}

OzzKinematicsAnimated* COzzKinematicsVisual::AnimatedKinematics()
{
    return animated_kinematics_;
}

const OzzKinematicsAnimated* COzzKinematicsVisual::AnimatedKinematics() const
{
    return animated_kinematics_;
}

bool COzzKinematicsVisual::IsKinematicsReady() const
{
    return initialized_ && kinematics_ && kinematics_->IsInitialized();
}

bool COzzKinematicsVisual::RequiresAnimation() const
{
    return Type == MT_OZZ_ANIMATED;
}

bool COzzKinematicsVisual::LoadAnimationFromFile(const std::filesystem::path& path)
{
    if (!animated_kinematics_)
        return false;

    if (!animated_kinematics_->LoadAnimationFromFile(path))
        return false;

    last_animation_update_frame_ = u32(-1);
    return true;
}

void COzzKinematicsVisual::StopAnimation()
{
    if (animated_kinematics_)
    {
        animated_kinematics_->StopAnimation();
        last_animation_update_frame_ = u32(-1);
    }
}

bool COzzKinematicsVisual::UpdateAnimation(float dt)
{
    if (!animated_kinematics_)
        return false;
    return animated_kinematics_->AdvanceAnimation(dt);
}

bool COzzKinematicsVisual::PlayMotion(const xr_string& motion_name)
{
    if (!animated_kinematics_)
        return false;

    // Use standard PlayCycle method to play the motion
    MotionID motion_id = animated_kinematics_->ID_Cycle_Safe(motion_name.c_str());
    if (!motion_id.valid())
        return false;

    CBlend* blend = animated_kinematics_->PlayCycle(motion_id);
    if (!blend)
        return false;

    last_animation_update_frame_ = u32(-1);
    return true;
}

xr_vector<xr_string> COzzKinematicsVisual::GetAvailableMotions()
{
    xr_vector<xr_string> result;
    if (!animated_kinematics_)
        return result;

    // Get motion names from all loaded motion slots
    // Note: This would require exposing motion names through the shared_motions interface
    // For now, return empty vector as we're removing legacy support
    // TODO: Implement proper motion enumeration through the shared motion system

    return result;
}
} // namespace xray::render::RENDER_NAMESPACE
