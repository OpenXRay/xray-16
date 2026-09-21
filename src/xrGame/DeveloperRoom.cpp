#include "StdAfx.h"
#include "DeveloperRoom.h"

u32 CDeveloperRoom::AddMaterial(xray::render::DeveloperScene& scene, pcstr name,
    float red, float green, float blue, float metallic, float roughness)
{
    auto& material = scene.materials.emplace_back();
    material.name = name;
    material.color.set(red, green, blue);
    material.metallic = metallic;
    material.roughness = roughness;
    return static_cast<u32>(scene.materials.size() - 1);
}

void CDeveloperRoom::AddFace(xray::render::DeveloperSceneMesh& mesh, const Fvector& center,
    const Fvector& normal, const Fvector& tangent, const Fvector& binormal,
    float width, float height)
{
    const u16 base = static_cast<u16>(mesh.vertices.size());
    const float corners[4][2] = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } };
    for (const auto& corner : corners)
    {
        auto& vertex = mesh.vertices.emplace_back();
        vertex.position.mad(center, tangent, corner[0] * width);
        vertex.position.mad(binormal, corner[1] * height);
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.binormal = binormal;
        vertex.uv.set((corner[0] + 1.0f) * 0.5f, (corner[1] + 1.0f) * 0.5f);
    }
    const u16 indices[] = { 0, 1, 2, 0, 2, 3 };
    for (u16 index : indices)
        mesh.indices.push_back(base + index);
}

void CDeveloperRoom::AddBox(xray::render::DeveloperScene& scene, pcstr name,
    const Fvector& center, const Fvector& halfSize, u32 material, bool collision)
{
    auto& mesh = scene.meshes.emplace_back();
    mesh.name = name;
    mesh.material = material;
    mesh.collision = collision;
    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);
    Fvector origin;
    origin.set(center.x + halfSize.x, center.y, center.z);
    AddFace(mesh, origin, Fvector{ 1.0f, 0.0f, 0.0f }, Fvector{ 0.0f, 0.0f, -1.0f },
        Fvector{ 0.0f, 1.0f, 0.0f }, halfSize.z, halfSize.y);
    origin.set(center.x - halfSize.x, center.y, center.z);
    AddFace(mesh, origin, Fvector{ -1.0f, 0.0f, 0.0f }, Fvector{ 0.0f, 0.0f, 1.0f },
        Fvector{ 0.0f, 1.0f, 0.0f }, halfSize.z, halfSize.y);
    origin.set(center.x, center.y + halfSize.y, center.z);
    AddFace(mesh, origin, Fvector{ 0.0f, 1.0f, 0.0f }, Fvector{ 1.0f, 0.0f, 0.0f },
        Fvector{ 0.0f, 0.0f, -1.0f }, halfSize.x, halfSize.z);
    origin.set(center.x, center.y - halfSize.y, center.z);
    AddFace(mesh, origin, Fvector{ 0.0f, -1.0f, 0.0f }, Fvector{ 1.0f, 0.0f, 0.0f },
        Fvector{ 0.0f, 0.0f, 1.0f }, halfSize.x, halfSize.z);
    origin.set(center.x, center.y, center.z + halfSize.z);
    AddFace(mesh, origin, Fvector{ 0.0f, 0.0f, 1.0f }, Fvector{ 1.0f, 0.0f, 0.0f },
        Fvector{ 0.0f, 1.0f, 0.0f }, halfSize.x, halfSize.y);
    origin.set(center.x, center.y, center.z - halfSize.z);
    AddFace(mesh, origin, Fvector{ 0.0f, 0.0f, -1.0f }, Fvector{ -1.0f, 0.0f, 0.0f },
        Fvector{ 0.0f, 1.0f, 0.0f }, halfSize.x, halfSize.y);
}

void CDeveloperRoom::AddSphere(xray::render::DeveloperScene& scene, pcstr name,
    const Fvector& center, float radius, u32 material)
{
    constexpr u32 slices = 32;
    constexpr u32 stacks = 16;
    auto& mesh = scene.meshes.emplace_back();
    mesh.name = name;
    mesh.material = material;
    mesh.vertices.reserve((slices + 1) * (stacks + 1));
    mesh.indices.reserve(6 * slices * (stacks - 1));
    for (u32 row = 0; row <= stacks; ++row)
    {
        const float theta = PI * float(row) / float(stacks);
        for (u32 column = 0; column <= slices; ++column)
        {
            const float phi = PI_MUL_2 * float(column) / float(slices);
            auto& vertex = mesh.vertices.emplace_back();
            vertex.normal.set(_sin(theta) * _cos(phi), _cos(theta), _sin(theta) * _sin(phi));
            vertex.position.mad(center, vertex.normal, radius);
            vertex.tangent.set(-_sin(phi), 0.0f, _cos(phi));
            vertex.binormal.crossproduct(vertex.normal, vertex.tangent);
            vertex.uv.set(float(column) / float(slices), float(row) / float(stacks));
        }
    }
    for (u32 row = 0; row < stacks; ++row)
    {
        for (u32 column = 0; column < slices; ++column)
        {
            const u16 a = static_cast<u16>(row * (slices + 1) + column);
            const u16 b = static_cast<u16>(a + slices + 1);
            if (row != 0)
            {
                mesh.indices.push_back(a);
                mesh.indices.push_back(a + 1);
                mesh.indices.push_back(b);
            }
            if (row + 1 != stacks)
            {
                mesh.indices.push_back(a + 1);
                mesh.indices.push_back(b + 1);
                mesh.indices.push_back(b);
            }
        }
    }
}

Fvector CDeveloperRoom::ActorPosition()
{
    return Fvector{ 0.0f, 0.25f, -8.0f };
}

void CDeveloperRoom::Build(xray::render::DeveloperScene& scene)
{
    scene.name = "dev_material_room";
    scene.bounds.set(-14.0f, -2.0f, -14.0f, 14.0f, 10.0f, 14.0f);
    scene.materials.clear();
    scene.meshes.clear();
    const u32 floor = AddMaterial(scene, "floor", 0.32f, 0.34f, 0.36f, 0.0f, 0.8f);
    const u32 tile = AddMaterial(scene, "floor_tile", 0.55f, 0.56f, 0.58f, 0.0f, 0.65f);
    const u32 white = AddMaterial(scene, "wall_neutral", 0.7f, 0.7f, 0.7f, 0.0f, 0.8f);
    const u32 red = AddMaterial(scene, "wall_red", 0.55f, 0.045f, 0.035f, 0.0f, 0.8f);
    const u32 green = AddMaterial(scene, "wall_green", 0.045f, 0.4f, 0.08f, 0.0f, 0.8f);
    const u32 plinth = AddMaterial(scene, "plinth", 0.13f, 0.14f, 0.15f, 0.0f, 0.6f);
    AddBox(scene, "floor", Fvector{ 0.0f, -0.25f, 0.0f }, Fvector{ 12.0f, 0.25f, 12.0f }, floor);
    AddBox(scene, "wall_left", Fvector{ -12.0f, 3.0f, 0.0f }, Fvector{ 0.25f, 3.0f, 12.25f }, red);
    AddBox(scene, "wall_right", Fvector{ 12.0f, 3.0f, 0.0f }, Fvector{ 0.25f, 3.0f, 12.25f }, green);
    AddBox(scene, "wall_back", Fvector{ 0.0f, 3.0f, 12.0f }, Fvector{ 12.0f, 3.0f, 0.25f }, white);
    AddBox(scene, "wall_entry", Fvector{ 0.0f, 3.0f, -12.0f }, Fvector{ 12.0f, 3.0f, 0.25f }, white);
    AddBox(scene, "roof_back", Fvector{ 0.0f, 6.0f, 9.0f }, Fvector{ 12.0f, 0.2f, 3.0f }, white);
    for (u32 row = 0; row < 6; ++row)
    {
        for (u32 column = 0; column < 6; ++column)
        {
            if ((row + column) % 2 != 0)
                continue;
            string64 name;
            xr_sprintf(name, "floor_tile_%u_%u", row, column);
            AddBox(scene, name, Fvector{ -10.0f + 4.0f * float(column), 0.002f, -10.0f + 4.0f * float(row) },
                Fvector{ 1.98f, 0.002f, 1.98f }, tile, false);
        }
    }
    const float roughness[] = { 0.08f, 0.2f, 0.35f, 0.5f, 0.7f, 0.9f };
    for (u32 row = 0; row < 2; ++row)
    {
        for (u32 column = 0; column < 6; ++column)
        {
            string64 name;
            xr_sprintf(name, "%s_%u", row == 0 ? "dielectric" : "metal", column);
            const u32 sample = AddMaterial(scene, name, 0.7f, 0.48f, 0.2f, float(row), roughness[column]);
            const float x = -7.5f + float(column) * 3.0f;
            const float z = 2.0f + float(row) * 4.0f;
            AddSphere(scene, name, Fvector{ x, 1.6f, z }, 0.8f, sample);
            xr_sprintf(name, "plinth_%u_%u", row, column);
            AddBox(scene, name, Fvector{ x, 0.4f, z }, Fvector{ 0.95f, 0.4f, 0.95f }, plinth);
        }
    }
    for (u32 step = 0; step < 4; ++step)
    {
        string64 name;
        xr_sprintf(name, "step_%u", step);
        const float height = 0.18f * float(step + 1);
        AddBox(scene, name, Fvector{ -9.0f, height * 0.5f, -7.0f + float(step) },
            Fvector{ 1.4f, height * 0.5f, 0.5f }, white);
    }
    Msg("[dev_level] event=room name=%s meshes=%u materials=%u bounds=%.1f,%.1f,%.1f:%.1f,%.1f,%.1f",
        scene.name.c_str(), static_cast<u32>(scene.meshes.size()), static_cast<u32>(scene.materials.size()),
        scene.bounds.vMin.x, scene.bounds.vMin.y, scene.bounds.vMin.z,
        scene.bounds.vMax.x, scene.bounds.vMax.y, scene.bounds.vMax.z);
    for (u32 index = 0; index < scene.materials.size(); ++index)
    {
        const auto& material = scene.materials[index];
        Msg("[dev_level] event=material id=%u name=%s color=%.3f,%.3f,%.3f metallic=%.3f roughness=%.3f",
            index, material.name.c_str(), material.color.x, material.color.y, material.color.z,
            material.metallic, material.roughness);
    }
}
