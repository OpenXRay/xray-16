#include "stdafx.h"

#include "OzzMesh.h"

#include "xrCore/FMesh.hpp"
#include "SkeletonCustom.h"
#include "SkeletonX.h"
#include "FSkinnedTypes.h"
#include "Layers/xrRender/BufferUtils.h"
#include "OzzKinematicsAnimated.h"

#include "ozz/base/maths/simd_math.h"

#include <algorithm>
#include <vector>

#include "framework/mesh.h"

namespace xray::render::fg
{
OzzMesh::OzzMesh()
{
    Type = MT_OZZ_MESH;
    shader = nullptr;
}

OzzMesh::~OzzMesh() = default;

void OzzMesh::Copy(dxRender_Visual* pFrom)
{
    Fvisual::Copy(pFrom);

    auto* src = dynamic_cast<OzzMesh*>(pFrom);
    R_ASSERT2(src, "OzzMesh::Copy: source is not OzzMesh");

    m_Parent = src->m_Parent;

    m_jointRemaps       = src->m_jointRemaps;
    m_inverseBindPoses  = src->m_inverseBindPoses;
    m_paletteStaging.clear();

    m_boneCount              = static_cast<u32>(m_jointRemaps.size());
    m_uploadedBoneOffset     = 0;
    m_uploadedFrame          = 0;
}

void OzzMesh::LoadFromOzzMesh(XRay::Animation::OzzKinematics* parent, const ozz::sample::Mesh& mesh)
{
    m_Parent = parent;

    m_jointRemaps.assign(mesh.joint_remaps.begin(), mesh.joint_remaps.end());
    m_inverseBindPoses.assign(mesh.inverse_bind_poses.begin(), mesh.inverse_bind_poses.end());
    m_paletteStaging.clear();

    m_boneCount              = static_cast<u32>(m_jointRemaps.size());
    m_uploadedBoneOffset     = 0;
    m_uploadedFrame          = 0;

    if (!mesh.xray_metadata.shader_name.empty())
        shaderName = mesh.xray_metadata.shader_name.c_str();
    if (!mesh.xray_metadata.texture_path.empty())
        textureName = mesh.xray_metadata.texture_path.c_str();

    const std::uint32_t total_vertex_count = static_cast<std::uint32_t>(mesh.vertex_count());
    const std::uint32_t total_index_count = static_cast<std::uint32_t>(mesh.triangle_index_count());
    R_ASSERT2(total_vertex_count > 0 && total_index_count > 0, "OzzMesh: empty mesh");

    using VertexType = vertHW_4W<float>;

    p_rm_Vertices = xr_new<VertexStagingBuffer>();
    p_rm_Vertices->Create(total_vertex_count * sizeof(VertexType), true);

    p_rm_Indices = xr_new<IndexStagingBuffer>();
    p_rm_Indices->Create(total_index_count * sizeof(std::uint16_t), true);

    VertexType* vdst = static_cast<VertexType*>(p_rm_Vertices->Map());
    Fbox bbox;
    bbox.invalidate();

    std::uint32_t out_vertex = 0;
    for (const ozz::sample::Mesh::Part& part : mesh.parts)
    {
        const int part_vertex_count = part.vertex_count();
        const int influences = part.influences_count();
        for (int v = 0; v < part_vertex_count; ++v)
        {
            Fvector position;
            position.x = -part.positions[v * 3 + 0];
            position.y =  part.positions[v * 3 + 1];
            position.z = -part.positions[v * 3 + 2];

            Fvector normal{ 0.f, 0.f, 1.f };
            if (!part.normals.empty())
            {
                normal.x = -part.normals[v * 3 + 0];
                normal.y =  part.normals[v * 3 + 1];
                normal.z = -part.normals[v * 3 + 2];
            }

            Fvector tangent{ 1.f, 0.f, 0.f };
            float handedness = 1.f;
            if (!part.tangents.empty())
            {
                tangent.x = -part.tangents[v * 4 + 0];
                tangent.y =  part.tangents[v * 4 + 1];
                tangent.z = -part.tangents[v * 4 + 2];
                handedness = part.tangents[v * 4 + 3] >= 0.f ? 1.f : -1.f;
            }

            Fvector binormal;
            binormal.crossproduct(normal, tangent);
            binormal.mul(handedness);

            Fvector2 uv{ 0.f, 0.f };
            if (!part.uvs.empty())
            {
                uv.x = part.uvs[v * 2 + 0];
                uv.y = part.uvs[v * 2 + 1];
            }

            int idx[4] = { 0, 0, 0, 0 };
            float w[3] = { 0.f, 0.f, 0.f };
            const int copy_idx = std::min(4, influences);
            for (int i = 0; i < copy_idx; ++i)
                idx[i] = static_cast<int>(part.joint_indices[v * influences + i]);
            const int copy_w = std::min(3, std::max(0, influences - 1));
            for (int i = 0; i < copy_w; ++i)
                w[i] = part.joint_weights[v * (influences - 1) + i];
            if (influences == 1)
                w[0] = 1.f;
            else if (influences == 2)
                w[0] = part.joint_weights[v];

            VertexType& dst = vdst[out_vertex];
            dst.set(position, normal, tangent, binormal, uv,
                    idx[0] * 3, idx[1] * 3, idx[2] * 3, idx[3] * 3,
                    w[0], w[1], w[2]);

            bbox.modify(position);
            ++out_vertex;
        }
    }
    R_ASSERT(out_vertex == total_vertex_count);
    p_rm_Vertices->Unmap(true);

    std::uint16_t* idst = static_cast<std::uint16_t*>(p_rm_Indices->Map());
    for (std::uint32_t i = 0; i < total_index_count; ++i)
        idst[i] = mesh.triangle_indices[i];
    p_rm_Indices->Unmap(true);

    vCount = total_vertex_count;
    vBase = 0;
    vStride = sizeof(VertexType);

    iCount = total_index_count;
    iBase = 0;
    dwPrimitives = total_index_count / 3;

    rm_geom.create(get_decl<VertexType>(), *p_rm_Vertices, *p_rm_Indices);

    Fvector center;
    center.add(bbox.vMin, bbox.vMax);
    center.mul(0.5f);
    Fvector extent;
    extent.sub(bbox.vMax, bbox.vMin);
    extent.mul(0.5f);
    const float radius = extent.magnitude();

    vis.box.set(bbox.vMin, bbox.vMax);
    vis.sphere.P = center;
    vis.sphere.R = radius;
}
}
