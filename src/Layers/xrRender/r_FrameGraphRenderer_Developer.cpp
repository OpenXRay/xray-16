#include "stdafx.h"

#include "Layers/xrRender/DeveloperSceneRenderer.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/r__buffer_pool.h"
#include "Layers/xrRender/r__scene.h"
#include "Layers/xrRender/r__sector.h"
#include "Layers/xrRender/BufferUtils.h"
#include "Layers/xrRender/FVisual.h"
#include "Layers/xrRender/FHierrarhyVisual.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/ShaderVariant/ShaderVariantRegistry.h"
#include "Layers/xrRender/light.h"
#include "Layers/xrRender/Light_DB.h"
#include "Layers/xrRender/FrameGraph/Blackboard.h"
#include "Layers/xrRender/FrameGraphPasses/LocalShadowPassSetup.h"
#include "Layers/xrRender/FrameGraphPasses/ShaderConstants.h"
#include "Layers/xrRender/FrameGraphPasses/VSMPassSetup.h"
#include "Layers/xrRender/RayTracing/RTAccelStructManager.h"
#include "xrCore/FMesh.hpp"
#include "xrEngine/DeveloperScene.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IRenderBackend.h"
#include "xrEngine/defines.h"

namespace xray::render::fg
{
void DeveloperSceneRenderer::BuildVertexDeclaration(VertexDeclarator& declaration)
{
    declaration.clear();
    declaration.push_back(VertexElement{0, 0, VF_FLOAT3, 0, VS_POSITION, 0});
    declaration.push_back(VertexElement{0, 12, VF_FLOAT3, 0, VS_NORMAL, 0});
    declaration.push_back(VertexElement{0, 24, VF_FLOAT2, 0, VS_TEXCOORD, 0});

    VertexElement terminator = XR_VERTEX_ELEMENT_END;
    declaration.push_back(terminator);
}

u64 DeveloperSceneRenderer::FoldHash(u64 hash, const void* data, size_t size)
{
    const u8* bytes = static_cast<const u8*>(data);
    for (size_t i = 0; i < size; ++i)
    {
        hash ^= u64(bytes[i]);
        hash *= 1099511628211ull;
    }
    return hash;
}

xr_string DeveloperSceneRenderer::SanitizeName(const xr_string& name)
{
    xr_string result;
    result.reserve(NAME_LIMIT);

    for (char symbol : name)
    {
        if (result.size() >= NAME_LIMIT)
            break;
        if ((symbol >= 'a' && symbol <= 'z') || (symbol >= '0' && symbol <= '9') || symbol == '_')
            result.push_back(symbol);
        else if (symbol >= 'A' && symbol <= 'Z')
            result.push_back(char(symbol - 'A' + 'a'));
        else if (!result.empty() && result.back() != '_')
            result.push_back('_');
    }

    while (!result.empty() && result.back() == '_')
        result.pop_back();

    if (result.empty())
        result = "room";

    return result;
}

void DeveloperSceneRenderer::ComputeMeshBounds(const xray::render::DeveloperSceneMesh& mesh, Fbox& box, Fvector& center, float& radius)
{
    box.invalidate();
    for (const xray::render::DeveloperSceneVertex& vertex : mesh.vertices)
        box.modify(vertex.position.x, vertex.position.y, vertex.position.z);

    box.getcenter(center);
    radius = box.getradius();
}

u64 DeveloperSceneRenderer::HashScene(const xr_string& sceneName, const Fbox& bounds,
    const xr_vector<xray::render::DeveloperSceneMaterial>& materials,
    const xr_vector<xray::render::DeveloperSceneMesh>& meshes)
{
    u64 hash = 14695981039346656037ull;
    hash = FoldHash(hash, sceneName.c_str(), sceneName.size());
    hash = FoldHash(hash, &bounds, sizeof(bounds));

    for (const xray::render::DeveloperSceneMaterial& material : materials)
    {
        hash = FoldHash(hash, material.name.c_str(), material.name.size());
        hash = FoldHash(hash, material.shaderName.c_str(), material.shaderName.size());
        hash = FoldHash(hash, &material.color, sizeof(material.color));
        hash = FoldHash(hash, &material.metallic, sizeof(material.metallic));
        hash = FoldHash(hash, &material.roughness, sizeof(material.roughness));
        hash = FoldHash(hash, &material.opacity, sizeof(material.opacity));
    }

    for (const xray::render::DeveloperSceneMesh& mesh : meshes)
    {
        hash = FoldHash(hash, mesh.name.c_str(), mesh.name.size());
        hash = FoldHash(hash, &mesh.material, sizeof(mesh.material));
        hash = FoldHash(hash, &mesh.collision, sizeof(mesh.collision));
        hash = FoldHash(hash, mesh.vertices.data(), mesh.vertices.size() * sizeof(xray::render::DeveloperSceneVertex));
        hash = FoldHash(hash, mesh.indices.data(), mesh.indices.size() * sizeof(u16));
    }

    return hash;
}
}

namespace xray::render
{

bool FrameGraphRenderer::PrepareDeveloperMaterials(const DeveloperScene& scene)
{
    const xr_string sceneKey = fg::DeveloperSceneRenderer::SanitizeName(scene.name);
    const xr_string shaderName = "$developer\\" + sceneKey;

    for (u32 index = 0; index < u32(scene.materials.size()); ++index)
    {
        const DeveloperSceneMaterial& source = scene.materials[index];

        if (!source.shaderName.empty()
            && ShaderVariantRegistry::Instance().GetVariantIndex(source.shaderName.c_str()) == 0)
        {
            Msg("! [dev_level] event=load stage=materials result=fail material=%u name='%s' shader='%s' reason=missing_shader_variant",
                index, source.name.c_str(), source.shaderName.c_str());
            return false;
        }

        string64 materialSuffix;
        xr_sprintf(materialSuffix, "\\mat_%u", index);

        DeveloperMaterialRecord record;
        record.key = "$developer\\" + sceneKey + materialSuffix;
        record.shaderName = source.shaderName.empty() ? shaderName : source.shaderName;
        record.textureName = record.key;
        record.color = source.color;
        record.metallic = source.metallic;
        record.roughness = source.roughness;
        record.opacity = source.opacity;

        const u32 materialID = m_materialCache->RegisterDeveloperMaterial(record.key.c_str(),
            record.shaderName.c_str(), record.textureName.c_str(), record.color, record.metallic, record.roughness,
            record.opacity);
        if (materialID == UINT32_MAX)
        {
            Msg("! [dev_level] event=load stage=materials result=fail material=%u name='%s'",
                index, source.name.c_str());
            return false;
        }

        m_developerMaterials.push_back(record);
    }

    m_developerMaterialEpoch = m_materialCache->GetVisualMaterialEpoch();

    Msg("* [dev_level] event=load stage=materials result=ok count=%u default_shader='%s'",
        u32(m_developerMaterials.size()), shaderName.c_str());

    return true;
}

bool FrameGraphRenderer::BuildDeveloperGeometry(const DeveloperScene& scene, u32 formatID,
    xr_vector<dxRender_Visual*>& visuals, Fbox& bounds)
{
    fg::GPUCullingManager* gpuCulling = GetGPUCullingManager();

    const u32 vertexPoolBase = u32(fg::BufferPool.nVB.size());
    const u32 indexPoolBase = u32(fg::BufferPool.nIB.size());

    fg::BufferPool.nVB.resize(vertexPoolBase + u32(scene.meshes.size()));
    fg::BufferPool.nIB.resize(indexPoolBase + u32(scene.meshes.size()));
    fg::BufferPool.Visuals.reserve(fg::BufferPool.Visuals.size() + scene.meshes.size() + 1);
    visuals.clear();
    visuals.reserve(scene.meshes.size());

    bounds.invalidate();

    const fg::VertexElement* declaration = fg::BufferPool.nDC[formatID].begin();

    for (u32 meshIndex = 0; meshIndex < u32(scene.meshes.size()); ++meshIndex)
    {
        const DeveloperSceneMesh& source = scene.meshes[meshIndex];

        const u32 vertexCount = u32(source.vertices.size());
        const u32 indexCount = u32(source.indices.size());

        if (source.material >= scene.materials.size())
        {
            Msg("! [dev_level] event=load stage=geometry result=fail mesh=%u reason=material_index material=%u materials=%u",
                meshIndex, source.material, u32(scene.materials.size()));
            return false;
        }

        if (vertexCount == 0 || indexCount < 3 || (indexCount % 3) != 0 || vertexCount > fg::DeveloperSceneRenderer::MAX_POOL_VERTICES)
        {
            Msg("! [dev_level] event=load stage=geometry result=fail mesh=%u reason=range vertices=%u indices=%u limit=%u",
                meshIndex, vertexCount, indexCount, fg::DeveloperSceneRenderer::MAX_POOL_VERTICES);
            return false;
        }

        for (u32 index = 0; index < indexCount; ++index)
        {
            if (source.indices[index] >= vertexCount)
            {
                Msg("! [dev_level] event=load stage=geometry result=fail mesh=%u reason=index_overflow at=%u value=%u vertices=%u",
                    meshIndex, index, u32(source.indices[index]), vertexCount);
                return false;
            }
        }

        fg::VertexStagingBuffer& vertexBuffer = fg::BufferPool.nVB[vertexPoolBase + meshIndex];
        fg::IndexStagingBuffer& indexBuffer = fg::BufferPool.nIB[indexPoolBase + meshIndex];

        vertexBuffer.Create(size_t(vertexCount) * fg::DeveloperSceneRenderer::VERTEX_STRIDE);
        u8* vertexHost = static_cast<u8*>(vertexBuffer.Map());
        if (!vertexHost)
        {
            Msg("! [dev_level] event=load stage=geometry result=fail mesh=%u reason=vertex_map", meshIndex);
            return false;
        }

        for (u32 vertex = 0; vertex < vertexCount; ++vertex)
        {
            const DeveloperSceneVertex& input = source.vertices[vertex];
            fg::DeveloperSceneRenderer::VertexData& output = reinterpret_cast<fg::DeveloperSceneRenderer::VertexData*>(vertexHost)[vertex];

            output.position[0] = input.position.x;
            output.position[1] = input.position.y;
            output.position[2] = input.position.z;

            output.normal[0] = input.normal.x;
            output.normal[1] = input.normal.y;
            output.normal[2] = input.normal.z;

            output.uv[0] = input.uv.x;
            output.uv[1] = input.uv.y;
        }

        const u32 vertexPoolID = gpuCulling->RegisterVBPool(vertexHost, vertexCount, fg::DeveloperSceneRenderer::VERTEX_STRIDE, declaration, false);
        vertexBuffer.Unmap(true);

        if (vertexPoolID == UINT32_MAX)
        {
            Msg("! [dev_level] event=load stage=geometry result=fail mesh=%u reason=vertex_pool", meshIndex);
            return false;
        }

        indexBuffer.Create(size_t(indexCount) * sizeof(u16));
        u8* indexHost = static_cast<u8*>(indexBuffer.Map());
        if (!indexHost)
        {
            Msg("! [dev_level] event=load stage=geometry result=fail mesh=%u reason=index_map", meshIndex);
            return false;
        }

        CopyMemory(indexHost, source.indices.data(), size_t(indexCount) * sizeof(u16));

        const u32 indexPoolID = gpuCulling->RegisterIBPool(reinterpret_cast<const u16*>(indexHost), indexCount, false);
        indexBuffer.Unmap(true);

        if (indexPoolID == UINT32_MAX)
        {
            Msg("! [dev_level] event=load stage=geometry result=fail mesh=%u reason=index_pool", meshIndex);
            return false;
        }

        Fbox meshBounds;
        Fvector meshCenter;
        float meshRadius = 0.0f;
        fg::DeveloperSceneRenderer::ComputeMeshBounds(source, meshBounds, meshCenter, meshRadius);

        fg::Fvisual* visual = xr_new<fg::Fvisual>();
        visual->Type = MT_NORMAL;
        visual->vis.box.set(meshBounds);
        visual->vis.sphere.set(meshCenter, meshRadius);
        visual->shaderName = m_developerMaterials[source.material].shaderName.c_str();
        visual->textureName = m_developerMaterials[source.material].textureName.c_str();
        visual->shader_id = UINT32_MAX;
        visual->lifetimeID = dxRender_Visual::AllocateLifetimeID();

        visual->p_rm_Vertices = &vertexBuffer;
        visual->p_rm_Vertices->AddRef();
        visual->vBase = 0;
        visual->vCount = vertexCount;
        visual->vStride = fg::DeveloperSceneRenderer::VERTEX_STRIDE;

        visual->p_rm_Indices = &indexBuffer;
        visual->p_rm_Indices->AddRef();
        visual->iBase = 0;
        visual->iCount = indexCount;
        visual->dwPrimitives = indexCount / 3;

        visual->vbPoolID = vertexPoolID;
        visual->ibPoolID = indexPoolID;
        visual->useAlternativeGeom = false;

        visual->rm_geom.create(declaration, *visual->p_rm_Vertices, *visual->p_rm_Indices);

        bounds.merge(meshBounds);
        fg::BufferPool.Visuals.push_back(visual);
        visuals.push_back(visual);
    }

    return true;
}

bool FrameGraphRenderer::VerifyDeveloperAllocations(const xr_vector<dxRender_Visual*>& visuals, u32& validCount, u32& invalidCount)
{
    fg::GPUCullingManager* gpuCulling = GetGPUCullingManager();
    validCount = 0;
    invalidCount = 0;

    for (dxRender_Visual* visual : visuals)
    {
        fg::Fvisual* mesh = static_cast<fg::Fvisual*>(visual);
        const fg::MeshAllocation allocation = gpuCulling->GetMeshAllocation(mesh->vbPoolID, mesh->vBase, mesh->vCount,
            mesh->ibPoolID, mesh->iBase, mesh->iCount, mesh->useAlternativeGeom);

        if (allocation.valid)
            ++validCount;
        else
        {
            ++invalidCount;
            Msg("! [dev_level] event=load stage=allocation result=invalid vec=%u+%u idx=%u+%u",
                mesh->vBase, mesh->vCount, mesh->iBase, mesh->iCount);
        }
    }

    Msg("* [dev_level] event=load stage=allocation result=%s valid=%u invalid=%u",
        invalidCount == 0 ? "ok" : "fail", validCount, invalidCount);

    return invalidCount == 0;
}

bool FrameGraphRenderer::BakeDeveloperClusterDAG(const DeveloperScene& scene, u64& sceneStamp, xr_string& cachePath)
{
    fg::GPUCullingManager* gpuCulling = GetGPUCullingManager();

    xr_vector<fg::ClusterBakeRange> ranges;
    CollectClusterBakeRanges(ranges);
    if (ranges.empty())
    {
        Msg("! [dev_level] event=load stage=cluster_bake result=fail reason=no_ranges visuals=%u",
            u32(fg::BufferPool.Visuals.size()));
        return false;
    }

    sceneStamp = fg::DeveloperSceneRenderer::HashScene(fg::DeveloperSceneRenderer::SanitizeName(scene.name), scene.bounds, scene.materials, scene.meshes);

    string_path relative = "";
    xr_sprintf(relative, "cluster_cache_fg%sdev_%s.vclf", DELIMITER, fg::DeveloperSceneRenderer::SanitizeName(scene.name).c_str());

    string_path absolute = "";
    FS.update_path(absolute, "$app_data_root$", relative);
    cachePath = absolute;

    gpuCulling->BakeClusterDAG(ranges, cachePath.c_str(), sceneStamp);

    const fg::ClusterDAG& dag = gpuCulling->GetClusterDAG();
    if (dag.Empty())
    {
        Msg("! [dev_level] event=load stage=cluster_bake result=fail reason=empty_dag ranges=%u stamp=%llu",
            u32(ranges.size()), (unsigned long long)sceneStamp);
        return false;
    }

    Msg("* [dev_level] event=load stage=cluster_bake result=ok ranges=%u meshes=%u clusters=%u pages=%u stamp=%llu cache='%s'",
        u32(ranges.size()), dag.Stats().bakedMeshes, dag.Stats().clusters, u32(dag.Pages().size()),
        (unsigned long long)sceneStamp, cachePath.c_str());

    return true;
}

void FrameGraphRenderer::DiscardDeveloperStaging(const xr_vector<dxRender_Visual*>& visuals)
{
    fg::GPUCullingManager* gpuCulling = GetGPUCullingManager();

    for (dxRender_Visual* visual : visuals)
    {
        fg::Fvisual* mesh = static_cast<fg::Fvisual*>(visual);
        const fg::MeshAllocation allocation = gpuCulling->GetMeshAllocation(mesh->vbPoolID, mesh->vBase, mesh->vCount,
            mesh->ibPoolID, mesh->iBase, mesh->iCount, mesh->useAlternativeGeom);
        if (!allocation.valid)
            continue;

        if (mesh->p_rm_Vertices)
        {
            Resources->DiscardGeometryBuffer(mesh->p_rm_Vertices->GetBufferHandle());
            mesh->p_rm_Vertices->DiscardDeviceBuffer();
        }
        if (mesh->p_rm_Indices)
        {
            Resources->DiscardGeometryBuffer(mesh->p_rm_Indices->GetBufferHandle());
            mesh->p_rm_Indices->DiscardDeviceBuffer();
        }
    }
}

void FrameGraphRenderer::SetupDeveloperSun()
{
    fg::Lights.Unload();

    Fvector direction;
    direction.set(fg::passes::SunDirVisual());
    direction.normalize_safe();

    Fvector right;
    right.set(1, 0, 0);

    fg::light* sun = fg::Lights.Create();
    sun->flags.bStatic = true;
    sun->set_type(IRender_Light::DIRECT);
    sun->set_shadow(true);
    sun->set_rotation(direction, right);
    fg::Lights.sun = sun;

    fg::Lights.LoadHemi();

    Msg("* [dev_level] event=load stage=lights result=ok sun=(%.3f %.3f %.3f) shadow=1",
        direction.x, direction.y, direction.z);
}

bool FrameGraphRenderer::CommitDeveloperSector(const xr_vector<dxRender_Visual*>& visuals)
{
    fg::FHierrarhyVisual* root = xr_new<fg::FHierrarhyVisual>();
    root->Type = MT_HIERRARHY;
    root->bDontDelete = TRUE;
    root->lifetimeID = dxRender_Visual::AllocateLifetimeID();
    root->children.reserve(visuals.size());

    Fbox bounds;
    bounds.invalidate();

    for (dxRender_Visual* visual : visuals)
    {
        root->children.push_back(visual);
        bounds.merge(visual->vis.box);
    }

    Fvector center;
    bounds.getcenter(center);
    const float radius = bounds.getradius();

    root->vis.box.set(bounds);
    root->vis.sphere.set(center, radius);

    const u32 rootID = u32(fg::BufferPool.Visuals.size());
    fg::BufferPool.Visuals.push_back(root);

    xr_vector<fg::CSector::level_sector_data_t> sectors;
    sectors.resize(1);
    sectors[0].root_id = rootID;

    xr_vector<fg::CPortal::level_portal_data_t> portals;
    fg::Scene.load(sectors, portals);
    fg::Scene.largest_sector_id = 0;
    m_last_sector_id = IRender_Sector::INVALID_SECTOR_ID;

    if (fg::Scene.Sectors.empty() || !fg::Scene.Sectors[0]->root())
    {
        Msg("! [dev_level] event=load stage=sector result=fail reason=root_visual root_id=%u visuals=%u",
            rootID, u32(fg::BufferPool.Visuals.size()));
        return false;
    }

    Msg("* [dev_level] event=load stage=sector result=ok sectors=%u root_id=%u children=%u radius=%.3f",
        u32(fg::Scene.Sectors.size()), rootID, u32(visuals.size()), radius);

    return true;
}

void FrameGraphRenderer::ReRegisterDeveloperMaterials()
{
    if (!m_materialCache || m_developerMaterials.empty())
        return;

    if (m_materialCache->GetVisualMaterialEpoch() == m_developerMaterialEpoch)
        return;

    Msg("* [dev_level] event=materials action=reregister epoch=%u->%u count=%u",
        m_developerMaterialEpoch, m_materialCache->GetVisualMaterialEpoch(), u32(m_developerMaterials.size()));

    for (const DeveloperMaterialRecord& record : m_developerMaterials)
    {
        const u32 materialID = m_materialCache->RegisterDeveloperMaterial(record.key.c_str(), record.shaderName.c_str(),
            record.textureName.c_str(), record.color, record.metallic, record.roughness, record.opacity);
        R_ASSERT2(materialID != UINT32_MAX, "Failed to restore a developer material after renderer reset");
    }

    m_developerMaterialEpoch = m_materialCache->GetVisualMaterialEpoch();
}

void FrameGraphRenderer::CleanupDeveloperLoad()
{
    m_developerSceneLoaded = false;
    m_developerMaterialEpoch = 0;
    m_developerMeshCount = 0;
    m_developerVertexCount = 0;
    m_developerIndexCount = 0;

    if (m_materialCache)
        m_materialCache->ReleaseDeveloperMaterials();
    m_developerMaterials.clear();
}

bool FrameGraphRenderer::level_LoadDeveloper(const DeveloperScene& scene)
{
    ZoneScoped;
    m_lightingState.ResetRecovery();

    if (GEnv.isDedicatedServer)
    {
        Msg("! [dev_level] event=load result=fail reason=dedicated_server");
        return false;
    }

    if (b_loaded)
    {
        Msg("! [dev_level] event=load result=fail reason=already_loaded - unload the level first");
        return false;
    }

    if (!g_pGameLevel)
    {
        Msg("! [dev_level] event=load result=fail reason=no_game_level");
        return false;
    }

    if (!m_device || !m_geometryCollector || !m_materialCache || !GetGPUCullingManager())
    {
        Msg("! [dev_level] event=load result=fail reason=renderer_not_initialized");
        return false;
    }

    if (scene.materials.empty() || scene.meshes.empty())
    {
        Msg("! [dev_level] event=load result=fail reason=empty_scene materials=%u meshes=%u",
            u32(scene.materials.size()), u32(scene.meshes.size()));
        return false;
    }

    u32 totalVertices = 0;
    u32 totalIndices = 0;
    for (const DeveloperSceneMesh& mesh : scene.meshes)
    {
        totalVertices += u32(mesh.vertices.size());
        totalIndices += u32(mesh.indices.size());
    }

    if (totalVertices == 0 || totalIndices == 0)
    {
        Msg("! [dev_level] event=load result=fail reason=no_geometry vertices=%u indices=%u", totalVertices, totalIndices);
        return false;
    }

    Msg("* [dev_level] event=load stage=begin name='%s' materials=%u meshes=%u vertices=%u indices=%u",
        scene.name.c_str(), u32(scene.materials.size()), u32(scene.meshes.size()), totalVertices, totalIndices);

    g_pGamePersistent->LoadBegin();
    Resources->DeferredLoad(TRUE);

    if (m_geometryCollector)
        m_geometryCollector->ClearStatic();
    if (m_blackboard)
    {
        fg::passes::InvalidateVSMCache(m_blackboard->get_or_add<fg::passes::VSMState>());
        fg::passes::ResetLocalShadowPool(m_blackboard->get_or_add<fg::passes::LocalShadowState>());
    }
    fg::passes::ResetSunDirVisual();

    if (!PrepareDeveloperMaterials(scene))
    {
        Msg("! [dev_level] event=load result=fail stage=materials");
        g_pGamePersistent->LoadEnd();
        b_loaded = TRUE;
        level_Unload();
        return false;
    }

    fg::VertexDeclarator declaration;
    fg::DeveloperSceneRenderer::BuildVertexDeclaration(declaration);

    const u32 formatID = u32(fg::BufferPool.nDC.size());
    fg::BufferPool.nDC.push_back(declaration);

    g_pGamePersistent->LoadTitle("st_loading_geometry");

    fg::GPUCullingManager* gpuCulling = GetGPUCullingManager();
    gpuCulling->BeginLevelLoad(totalVertices, totalIndices);

    xr_vector<dxRender_Visual*> visuals;
    Fbox bounds;
    const bool geometryBuilt = BuildDeveloperGeometry(scene, formatID, visuals, bounds);

    u32 validAllocations = 0;
    u32 invalidAllocations = 0;
    const bool allocationsValid = geometryBuilt
        && VerifyDeveloperAllocations(visuals, validAllocations, invalidAllocations);

    u64 sceneStamp = 0;
    xr_string cachePath;
    const bool baked = allocationsValid && BakeDeveloperClusterDAG(scene, sceneStamp, cachePath);

    if (!geometryBuilt || !allocationsValid || !baked)
    {
        Msg("! [dev_level] event=load result=fail stage=%s meshes_built=%u",
            !geometryBuilt ? "geometry" : (!allocationsValid ? "allocation" : "cluster_bake"),
            u32(visuals.size()));
        g_pGamePersistent->LoadEnd();
        b_loaded = TRUE;
        level_Unload();
        return false;
    }

    gpuCulling->EndLevelLoad();
    if (GEnv.Backend)
        GEnv.Backend->WaitForIdle();

    DiscardDeveloperStaging(visuals);

    g_pGamePersistent->LoadTitle("st_loading_sectors_portals");

    const bool sectorCommitted = CommitDeveloperSector(visuals);
    if (!sectorCommitted)
    {
        Msg("! [dev_level] event=load result=fail stage=sector mesh_count=%u", u32(visuals.size()));
        g_pGamePersistent->LoadEnd();
        b_loaded = TRUE;
        level_Unload();
        return false;
    }

    g_pGamePersistent->LoadTitle("st_loading_lights");
    SetupDeveloperSun();

    if (m_blackboard)
        fg::passes::WarmLocalShadowPool(m_device, m_blackboard->get_or_add<fg::passes::LocalShadowState>());

    WarmParticles();
    WarmGameplayPipelines();

    m_developerMeshCount = u32(visuals.size());
    m_developerVertexCount = totalVertices;
    m_developerIndexCount = totalIndices;
    m_developerSceneLoaded = true;

    g_pGamePersistent->LoadEnd();
    b_loaded = TRUE;

    Msg("* [dev_level] event=load result=ok name='%s' materials=%u meshes=%u vertices=%u indices=%u allocations=%u cluster_stamp=%llu cluster_cache='%s'",
        scene.name.c_str(), u32(m_developerMaterials.size()), m_developerMeshCount, m_developerVertexCount,
        m_developerIndexCount, validAllocations, (unsigned long long)sceneStamp, cachePath.c_str());

    return true;
}

void FrameGraphRenderer::DumpDeveloperSceneDiagnostics()
{
    if (!m_developerSceneLoaded)
    {
        Msg("[dev_level] event=render state=idle loaded=0");
        return;
    }

    const GeometryCollector::Stats& collectorStats = m_geometryCollector->GetStats();

    fg::GPUCullingManager* gpuCulling = GetGPUCullingManager();
    const fg::GPUCullingManager::CullingStats& cullStats = gpuCulling->GetCullingStats();
    const fg::GeometryMemoryStats& memoryStats = gpuCulling->GetGeometryMemoryStats();
    const fg::ClusterDAG& dag = gpuCulling->GetClusterDAG();
    const fg::ClusterResidencyStats& residencyStats = dag.ResidencyStats();
    const fg::ClusterBakeStats& bakeStats = dag.Stats();

    const bool statsReadbackScheduled = psDeviceFlags.test(rsStatistic);

    Msg("[dev_level] event=render frame=%u loaded=1 meshes=%u verts=%u indices=%u materials=%u "
        "batches=%u static_batches=%u static_build_complete=%d collector_triangles=%u collector_verts=%u "
        "static_draws=%u static_instances=%u static_objects=%u transparent_objects=%u dynamic_objects=%u skinned_objects=%u "
        "lights_collected=%u lights_culled=%u renderables=%u sectors=%u",
        Device.dwFrame, m_developerMeshCount, m_developerVertexCount, m_developerIndexCount, u32(m_developerMaterials.size()),
        collectorStats.numBatches, u32(m_geometryCollector->GetStaticBatches().size()),
        m_geometryCollector->IsStaticBuildComplete() ? 1 : 0,
        collectorStats.numTriangles, collectorStats.numVertices,
        u32(gpuCulling->GetStaticDrawArgsData().size()), u32(gpuCulling->GetStaticInstanceData().size()),
        gpuCulling->GetStaticObjectCount(), gpuCulling->GetTransparentObjectCount(), gpuCulling->GetDynamicObjectCount(),
        gpuCulling->GetSkinnedObjectCount(),
        u32(m_collectedLights.size()), u32(m_culledLights.size()), u32(m_lstRenderables.size()),
        u32(fg::Scene.Sectors.size()));

    Msg("[dev_level] event=lighting frame=%u requested=%u effective=%u fallback=%u names='%s|%s|%s' "
        "recorded=%d recorded_samples=%u conflicting_requests=%d history_used=%d scene_revision=%llu "
        "pt_samples=%u pt_valid=%d",
        Device.dwFrame, u32(m_lightingState.requested), u32(m_lightingState.effective), u32(m_lightingState.fallback),
        fg::LightingModeName(m_lightingState.requested), fg::LightingModeName(m_lightingState.effective),
        fg::LightingFallbackName(m_lightingState.fallback),
        m_lightingState.recorded ? 1 : 0, m_lightingState.recordedSamples,
        0, m_lightingState.historyUsed ? 1 : 0,
        (unsigned long long)m_lightingState.sceneRevision,
        m_mainView.pathTracer.history.samples, m_mainView.pathTracer.history.valid ? 1 : 0);

    Msg("[dev_level] event=geometry frame=%u cull_stats_source=readback cull_stats_scheduled=%d "
        "cluster_visible=%u cluster_candidates=%u cluster_tris_drawn=%u cluster_overflow=%u "
        "cluster_meshes=%u clusters=%u pages=%u bake_failed=%u root_pages=%u fine_pages=%u pinned_groups=%u "
        "residency_used_bytes=%llu residency_arena_bytes=%llu geometry_cut=%u "
        "mega_ready=%d mega_uploaded=%d rt_source_ready=%d rt_verts=%u rt_indices=%u rt_scene_ready=%d",
        Device.dwFrame, statsReadbackScheduled ? 1 : 0,
        cullStats.clusterVisible, cullStats.clusterCandidates, cullStats.clusterTrianglesDrawn, cullStats.clusterOverflow,
        bakeStats.bakedMeshes, bakeStats.clusters, u32(dag.Pages().size()), bakeStats.bakeFailed,
        residencyStats.rootPages, residencyStats.finePages, residencyStats.pinnedGroups,
        (unsigned long long)memoryStats.residencyUsedBytes,
        (unsigned long long)memoryStats.residencyArenaBytes,
        gpuCulling->GetGeometryCutRevision(),
        gpuCulling->AreMegaBuffersReady() ? 1 : 0, gpuCulling->IsMegaDataUploaded() ? 1 : 0,
        gpuCulling->IsRTSourceReady() ? 1 : 0, gpuCulling->GetRTVertexCount(), gpuCulling->GetRTIndexCount(),
        (m_rtAccelMgr && m_rtAccelMgr->IsReady()) ? 1 : 0);
}
}
