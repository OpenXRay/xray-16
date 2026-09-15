#include "stdafx.h"
#include "GpuParticleCollision.h"
#include "ParticleEffectDef.h"
#include "FrameGraph/BindingSetBuilder.h"
#include "SkeletonCustom.h"
#include "SkeletonX.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/xr_collide_form.h"
#include "xrEngine/cf_dynamic_mesh.h"
#include "xrCore/Animation/Bone.hpp"
#include <algorithm>
#include <limits>

namespace xray::render::fg
{
namespace
{
struct CollisionNode
{
    Fvector minimum;
    u32 escape;
    Fvector maximum;
    u32 first;
    u32 count;
    u32 pad0;
    u32 pad1;
    u32 pad2;
};

struct CollisionTriangle
{
    Fvector4 a;
    Fvector4 b;
    Fvector4 c;
};

struct CollisionShape
{
    Fvector4 row0;
    Fvector4 row1;
    Fvector4 row2;
    Fvector4 data0;
    Fvector4 data1;
    u32 type;
    u32 triangleFirst;
    u32 triangleCount;
    u32 pad;
};

struct CollisionObject
{
    Fvector4 spatialSphere;
    Fvector4 sphere;
    Fvector4 row0;
    Fvector4 row1;
    Fvector4 row2;
    u32 first;
    u32 count;
    u32 mesh;
    u32 pad;
};

struct CollisionCounts
{
    u32 nodes;
    u32 objects;
    u32 staticTriangles;
    u32 dynamicTriangles;
};

static_assert(sizeof(CollisionNode) == 48);
static_assert(sizeof(CollisionTriangle) == 48);
static_assert(sizeof(CollisionShape) == 96);
static_assert(sizeof(CollisionObject) == 96);
static_assert(sizeof(CollisionCounts) == 16);

Fvector4 Vector4(const Fvector& vector, float w = 0.f)
{
    return {vector.x, vector.y, vector.z, w};
}

void MatrixRows(const Fmatrix& matrix, Fvector4& row0, Fvector4& row1, Fvector4& row2)
{
    row0.set(matrix._11, matrix._21, matrix._31, matrix._41);
    row1.set(matrix._12, matrix._22, matrix._32, matrix._42);
    row2.set(matrix._13, matrix._23, matrix._33, matrix._43);
}

void IncludePoint(Fvector& minimum, Fvector& maximum, const Fvector4& point)
{
    minimum.x = std::min(minimum.x, point.x);
    minimum.y = std::min(minimum.y, point.y);
    minimum.z = std::min(minimum.z, point.z);
    maximum.x = std::max(maximum.x, point.x);
    maximum.y = std::max(maximum.y, point.y);
    maximum.z = std::max(maximum.z, point.z);
}

u32 BuildCollisionTree(xr_vector<CollisionNode>& nodes, xr_vector<CollisionTriangle>& triangles,
    u32 first, u32 count)
{
    const u32 index = static_cast<u32>(nodes.size());
    CollisionNode node{};
    node.minimum.set(flt_max, flt_max, flt_max);
    node.maximum.set(-flt_max, -flt_max, -flt_max);
    for (u32 i = first; i != first + count; ++i)
    {
        IncludePoint(node.minimum, node.maximum, triangles[i].a);
        IncludePoint(node.minimum, node.maximum, triangles[i].b);
        IncludePoint(node.minimum, node.maximum, triangles[i].c);
    }
    nodes.push_back(node);
    if (count <= 8)
    {
        nodes[index].first = first;
        nodes[index].count = count;
    }
    else
    {
        Fvector extent;
        extent.sub(node.maximum, node.minimum);
        const u32 axis = extent.x >= extent.y && extent.x >= extent.z ? 0 : extent.y >= extent.z ? 1 : 2;
        const u32 middle = first + count / 2;
        const auto coordinate = [axis](const CollisionTriangle& triangle)
        {
            if (axis == 0)
                return triangle.a.x + triangle.b.x + triangle.c.x;
            if (axis == 1)
                return triangle.a.y + triangle.b.y + triangle.c.y;
            return triangle.a.z + triangle.b.z + triangle.c.z;
        };
        std::nth_element(triangles.begin() + first, triangles.begin() + middle,
            triangles.begin() + first + count, [&](const CollisionTriangle& a, const CollisionTriangle& b)
            {
                return coordinate(a) < coordinate(b);
            });
        BuildCollisionTree(nodes, triangles, first, middle - first);
        BuildCollisionTree(nodes, triangles, middle, first + count - middle);
    }
    nodes[index].escape = static_cast<u32>(nodes.size());
    return index;
}

struct CollisionBuffer
{
    struct Range
    {
        size_t first;
        size_t end;
    };

    nvrhi::BufferHandle buffer;
    xr_vector<Range> dirty;

    void Mark(size_t first, size_t end)
    {
        if (first == end)
            return;
        if (!dirty.empty() && dirty.back().end >= first)
            dirty.back().end = std::max(dirty.back().end, end);
        else
            dirty.push_back({first, end});
    }

    template <typename T>
    void Upload(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, const char* name,
        const xr_vector<T>& values, u64& bindingVersion, bool full = false)
    {
        const T empty{};
        const T* data = values.empty() ? &empty : values.data();
        const size_t bytes = std::max(size_t(1), values.size()) * sizeof(T);
        if (!buffer || buffer->getDesc().byteSize < bytes)
        {
            nvrhi::BufferDesc description;
            description.byteSize = std::max<u64>(bytes, buffer ? buffer->getDesc().byteSize * 2 : bytes);
            description.structStride = sizeof(T);
            description.debugName = name;
            description.initialState = nvrhi::ResourceStates::ShaderResource;
            description.keepInitialState = true;
            buffer = device->createBuffer(description);
            R_ASSERT2(buffer, "GPU particle collision buffer allocation failed");
            ++bindingVersion;
            full = true;
        }
        if (full)
            commandList->writeBuffer(buffer, data, bytes);
        else
            for (const auto& range : dirty)
            {
                const size_t end = std::min(range.end, values.size());
                if (range.first < end)
                    commandList->writeBuffer(buffer, data + range.first,
                        (end - range.first) * sizeof(T), range.first * sizeof(T));
            }
        dirty.clear();
    }
};

template <typename T>
void StoreCollisionValue(xr_vector<T>& values, CollisionBuffer& buffer, size_t index, const T& value)
{
    if (index == values.size())
        values.push_back(value);
    else if (memcmp(&values[index], &value, sizeof(T)) == 0)
        return;
    else
        values[index] = value;
    buffer.Mark(index, index + 1);
}

struct CollisionTriangleRange
{
    u64 revision;
    u32 first;
    u32 count;
};
}

struct GpuParticleCollision::State
{
    nvrhi::IDevice* device{};
    const IGame_Level* level{};
    const CDB::MODEL* model{};
    const CDB::TRI* sourceTriangles{};
    const Fvector* sourceVertices{};
    u32 sourceTriangleCount{};
    u32 sourceVertexCount{};
    xr_vector<CollisionNode> nodes;
    xr_vector<CollisionTriangle> staticTriangles;
    xr_vector<CollisionTriangle> dynamicTriangles;
    xr_vector<CollisionShape> shapes;
    xr_vector<CollisionObject> objects;
    xr_vector<CollisionTriangleRange> triangleRanges;
    xr_vector<CollisionNode> blocks;
    xr_vector<CollisionCounts> counts{1};
    CollisionBuffer nodeBuffer;
    CollisionBuffer staticBuffer;
    CollisionBuffer dynamicBuffer;
    CollisionBuffer shapeBuffer;
    CollisionBuffer objectBuffer;
    CollisionBuffer countBuffer;
    CollisionBuffer blockBuffer;
    u64 bindingVersion{};

    bool UpdateStatic()
    {
        auto* currentLevel = g_pGameLevel;
        auto* currentModel = currentLevel && currentLevel->bReady ? currentLevel->ObjectSpace.GetStaticModel() : nullptr;
        const CDB::TRI* triangles = currentModel ? currentModel->get_tris() : nullptr;
        const Fvector* vertices = currentModel ? currentModel->get_verts() : nullptr;
        const u32 triangleCount = currentModel ? currentModel->get_tris_count() : 0;
        const u32 vertexCount = currentModel ? currentModel->get_verts_count() : 0;
        if (currentLevel == level && currentModel == model && triangles == sourceTriangles
            && vertices == sourceVertices && triangleCount == sourceTriangleCount && vertexCount == sourceVertexCount)
            return false;
        level = currentLevel;
        model = currentModel;
        sourceTriangles = triangles;
        sourceVertices = vertices;
        sourceTriangleCount = triangleCount;
        sourceVertexCount = vertexCount;
        nodes.clear();
        staticTriangles.clear();
        if (!triangles || !vertices || triangleCount == 0 || vertexCount == 0)
            return true;
        staticTriangles.reserve(triangleCount);
        for (u32 i = 0; i != triangleCount; ++i)
        {
            const auto& triangle = triangles[i];
            R_ASSERT(triangle.verts[0] < vertexCount && triangle.verts[1] < vertexCount && triangle.verts[2] < vertexCount);
            staticTriangles.push_back({Vector4(vertices[triangle.verts[0]]),
                Vector4(vertices[triangle.verts[1]]), Vector4(vertices[triangle.verts[2]])});
        }
        nodes.reserve(triangleCount / 2 + 1);
        BuildCollisionTree(nodes, staticTriangles, 0, triangleCount);
        return true;
    }

    void UpdateDynamic()
    {
        u32 objectCount = 0;
        u32 shapeCount = 0;
        u32 triangleCount = 0;
        u32 rangeCount = 0;
        if (!g_pGameLevel || !g_pGameLevel->bReady || !g_pGamePersistent)
        {
            ClearDynamic();
            return;
        }
        auto& list = g_pGameLevel->Objects;
        for (u32 index = 0; index != list.o_count(); ++index)
        {
            IGameObject* object = list.o_get_by_iterator(index);
            auto& spatial = object->GetSpatialData();
            if (!(spatial.type & STYPE_COLLIDEABLE) || !spatial.node_ptr
                || spatial.space != &g_pGamePersistent->SpatialSpace)
                continue;
            ICollisionForm* form = object->GetCForm();
            if (!form || form->Type() != cftObject)
                continue;
            auto* skeleton = dynamic_cast<CCF_Skeleton*>(form);
            R_ASSERT2(skeleton, "GPU particle collision encountered an unknown object collision form");
            skeleton->RefreshCollisionGeometry();
            CollisionObject gpuObject{};
            gpuObject.first = shapeCount;
            gpuObject.mesh = dynamic_cast<CCF_DynamicMesh*>(form) != nullptr;
            gpuObject.spatialSphere = Vector4(spatial.sphere.P, spatial.sphere.R);
            Fvector center;
            object->XFORM().transform_tiny(center, form->getSphere().P);
            gpuObject.sphere = Vector4(center, form->getSphere().R);
            if (gpuObject.mesh)
            {
                Fmatrix inverse;
                inverse.invert(object->XFORM());
                MatrixRows(inverse, gpuObject.row0, gpuObject.row1, gpuObject.row2);
            }
            auto* kinematics = gpuObject.mesh ? dynamic_cast<CKinematics*>(object->Visual()) : nullptr;
            R_ASSERT(!gpuObject.mesh || kinematics);
            for (const auto& element : skeleton->_GetElements())
            {
                if (!element.valid())
                    continue;
                CollisionShape shape{};
                switch (element.type)
                {
                case SBoneShape::stBox:
                    shape.type = 1;
                    MatrixRows(element.b_IM, shape.row0, shape.row1, shape.row2);
                    shape.data0 = Vector4(element.b_hsize);
                    break;
                case SBoneShape::stSphere:
                    shape.type = 2;
                    shape.data0 = Vector4(element.s_sphere.P, element.s_sphere.R);
                    break;
                case SBoneShape::stCylinder:
                    shape.type = 3;
                    shape.data0 = Vector4(element.c_cylinder.m_center, element.c_cylinder.m_radius);
                    shape.data1 = Vector4(element.c_cylinder.m_direction, element.c_cylinder.m_height);
                    break;
                default:
                    NODEFAULT;
                }
                shape.triangleFirst = triangleCount;
                if (kinematics)
                {
                    for (auto* child : kinematics->children)
                    {
                        auto* mesh = dynamic_cast<CSkeletonX*>(child);
                        R_ASSERT(mesh);
                        const auto& cached = mesh->ExportCollisionTriangles(element.elem_id);
                        R_ASSERT(cached.vertices.size() % 3 == 0);
                        const u32 count = static_cast<u32>(cached.vertices.size() / 3);
                        const CollisionTriangleRange range{cached.revision, triangleCount, count};
                        const bool changed = rangeCount == triangleRanges.size()
                            || triangleRanges[rangeCount].revision != range.revision
                            || triangleRanges[rangeCount].first != range.first
                            || triangleRanges[rangeCount].count != range.count;
                        if (rangeCount == triangleRanges.size())
                            triangleRanges.push_back(range);
                        else
                            triangleRanges[rangeCount] = range;
                        ++rangeCount;
                        if (changed)
                        {
                            if (dynamicTriangles.size() < size_t(triangleCount) + count)
                                dynamicTriangles.resize(size_t(triangleCount) + count);
                            for (u32 triangle = 0; triangle != count; ++triangle)
                            {
                                const auto* vertices = cached.vertices.data() + size_t(triangle) * 3;
                                dynamicTriangles[triangleCount + triangle] = {Vector4(vertices[0]),
                                    Vector4(vertices[1]), Vector4(vertices[2])};
                            }
                            dynamicBuffer.Mark(triangleCount, size_t(triangleCount) + count);
                        }
                        triangleCount += count;
                    }
                    shape.triangleCount = triangleCount - shape.triangleFirst;
                }
                StoreCollisionValue(shapes, shapeBuffer, shapeCount++, shape);
            }
            gpuObject.count = shapeCount - gpuObject.first;
            if (gpuObject.count)
                StoreCollisionValue(objects, objectBuffer, objectCount++, gpuObject);
        }
        objects.resize(objectCount);
        shapes.resize(shapeCount);
        dynamicTriangles.resize(triangleCount);
        triangleRanges.resize(rangeCount);
        u32 blockCount = 0;
        for (u32 first = 0; first < objectCount; first += 16)
        {
            CollisionNode block{};
            block.first = first;
            block.count = std::min(16u, objectCount - first);
            block.minimum.set(flt_max, flt_max, flt_max);
            block.maximum.set(-flt_max, -flt_max, -flt_max);
            for (u32 offset = 0; offset != block.count; ++offset)
            {
                const auto& sphere = objects[first + offset].spatialSphere;
                const float margin = (std::max({std::abs(sphere.x), std::abs(sphere.y),
                    std::abs(sphere.z), std::abs(sphere.w)}) + 1.f)
                    * (8.f * std::numeric_limits<float>::epsilon());
                const float radius = std::abs(sphere.w) + margin;
                IncludePoint(block.minimum, block.maximum,
                    {sphere.x - radius, sphere.y - radius, sphere.z - radius, 0.f});
                IncludePoint(block.minimum, block.maximum,
                    {sphere.x + radius, sphere.y + radius, sphere.z + radius, 0.f});
            }
            StoreCollisionValue(blocks, blockBuffer, blockCount++, block);
        }
        blocks.resize(blockCount);
    }

    void ClearDynamic()
    {
        objects.clear();
        shapes.clear();
        dynamicTriangles.clear();
        triangleRanges.clear();
        blocks.clear();
    }
};

GpuParticleCollision::GpuParticleCollision() : state(std::make_unique<State>()) {}
GpuParticleCollision::~GpuParticleCollision() = default;

void GpuParticleCollision::Update(nvrhi::IDevice* device, nvrhi::ICommandList* commandList, u32 definitionFlags)
{
    R_ASSERT(device && commandList);
    if (state->device && state->device != device)
        Reset();
    state->device = device;
    const bool collision = (definitionFlags & PS::CPEDef::dfCollision) != 0;
    if (!collision && state->countBuffer.buffer)
        return;
    const bool staticChanged = collision && state->UpdateStatic();
    if (collision && (definitionFlags & PS::CPEDef::dfCollisionDyn) != 0)
        state->UpdateDynamic();
    else
        state->ClearDynamic();
    CollisionCounts counts{};
    counts.nodes = static_cast<u32>(state->nodes.size());
    counts.objects = static_cast<u32>(state->objects.size());
    counts.staticTriangles = static_cast<u32>(state->staticTriangles.size());
    counts.dynamicTriangles = static_cast<u32>(state->dynamicTriangles.size());
    StoreCollisionValue(state->counts, state->countBuffer, 0, counts);
    if (staticChanged || !state->nodeBuffer.buffer)
    {
        state->nodeBuffer.Upload(device, commandList, "GpuParticleCollisionNodes", state->nodes,
            state->bindingVersion, true);
        state->staticBuffer.Upload(device, commandList, "GpuParticleCollisionStaticTriangles", state->staticTriangles,
            state->bindingVersion, true);
    }
    state->dynamicBuffer.Upload(device, commandList, "GpuParticleCollisionDynamicTriangles", state->dynamicTriangles,
        state->bindingVersion);
    state->shapeBuffer.Upload(device, commandList, "GpuParticleCollisionShapes", state->shapes, state->bindingVersion);
    state->objectBuffer.Upload(device, commandList, "GpuParticleCollisionObjects", state->objects, state->bindingVersion);
    state->blockBuffer.Upload(device, commandList, "GpuParticleCollisionBlocks", state->blocks, state->bindingVersion);
    state->countBuffer.Upload(device, commandList, "GpuParticleCollisionCounts", state->counts, state->bindingVersion);
}

void GpuParticleCollision::Bind(framegraph::BindingSetBuilder& builder) const
{
    R_ASSERT(state->countBuffer.buffer);
    builder.BufferSRV("g_GpuCollisionCounts", state->countBuffer.buffer)
        .BufferSRV("g_GpuCollisionNodes", state->nodeBuffer.buffer)
        .BufferSRV("g_GpuCollisionStaticTriangles", state->staticBuffer.buffer)
        .BufferSRV("g_GpuCollisionDynamicTriangles", state->dynamicBuffer.buffer)
        .BufferSRV("g_GpuCollisionShapes", state->shapeBuffer.buffer)
        .BufferSRV("g_GpuCollisionObjects", state->objectBuffer.buffer)
        .BufferSRV("g_GpuCollisionBlocks", state->blockBuffer.buffer);
}

u64 GpuParticleCollision::GetBindingVersion() const
{
    return state->bindingVersion;
}

void GpuParticleCollision::Reset()
{
    const u64 bindingVersion = state->bindingVersion + 1;
    state = std::make_unique<State>();
    state->bindingVersion = bindingVersion;
}
}
