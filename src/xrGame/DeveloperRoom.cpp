#include "StdAfx.h"
#include "DeveloperRoom.h"

u32 CDeveloperRoom::AddMaterial(xray::render::DeveloperScene& scene, pcstr name,
    float red, float green, float blue, float metallic, float roughness,
    pcstr shaderName, float opacity)
{
    auto& material = scene.materials.emplace_back();
    material.name = name;
    material.color.set(red, green, blue);
    material.metallic = metallic;
    material.roughness = roughness;
    material.shaderName = shaderName ? shaderName : "";
    material.opacity = opacity;
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

void CDeveloperRoom::AddGlassDisplay(xray::render::DeveloperScene& scene, pcstr name,
    const Fvector& center, const Fvector& halfSize, u32 glass, u32 frame)
{
    string64 mesh;
    xr_sprintf(mesh, "%s_pane", name);
    AddBox(scene, mesh, center, halfSize, glass);
    xr_sprintf(mesh, "%s_base", name);
    AddBox(scene, mesh, Fvector{ center.x, 0.175f, center.z },
        Fvector{ halfSize.x + 0.05f, 0.175f, 0.08f }, frame);
    xr_sprintf(mesh, "%s_jamb_left", name);
    AddBox(scene, mesh, Fvector{ center.x - halfSize.x - 0.05f, center.y, center.z },
        Fvector{ 0.05f, halfSize.y + 0.1f, 0.06f }, frame);
    xr_sprintf(mesh, "%s_jamb_right", name);
    AddBox(scene, mesh, Fvector{ center.x + halfSize.x + 0.05f, center.y, center.z },
        Fvector{ 0.05f, halfSize.y + 0.1f, 0.06f }, frame);
    xr_sprintf(mesh, "%s_rail_top", name);
    AddBox(scene, mesh, Fvector{ center.x, center.y + halfSize.y + 0.05f, center.z },
        Fvector{ halfSize.x + 0.1f, 0.05f, 0.06f }, frame);
}

void CDeveloperRoom::AddPanelDisplay(xray::render::DeveloperScene& scene, pcstr name,
    const Fvector& center, const Fvector& halfSize, u32 panel, u32 support)
{
    string64 mesh;
    xr_sprintf(mesh, "%s_panel", name);
    AddBox(scene, mesh, center, halfSize, panel);
    xr_sprintf(mesh, "%s_base", name);
    AddBox(scene, mesh, Fvector{ center.x, 0.08f, center.z }, Fvector{ 0.5f, 0.08f, 0.3f }, support);
    xr_sprintf(mesh, "%s_post", name);
    const float postTop = center.y - halfSize.y + 0.1f;
    AddBox(scene, mesh, Fvector{ center.x, postTop * 0.5f, center.z },
        Fvector{ 0.22f, postTop * 0.5f, 0.06f }, support);
}

void CDeveloperRoom::AddPlinthSample(xray::render::DeveloperScene& scene, pcstr name,
    const Fvector& center, float radius, float height, u32 material, u32 plinth)
{
    string64 mesh;
    xr_sprintf(mesh, "%s_sphere", name);
    AddSphere(scene, mesh, center, radius, material);
    xr_sprintf(mesh, "%s_plinth", name);
    AddBox(scene, mesh, Fvector{ center.x, height * 0.5f, center.z },
        Fvector{ radius + 0.1f, height * 0.5f, radius + 0.1f }, plinth);
}

void CDeveloperRoom::AddSampleRecord(pcstr group, const xray::render::DeveloperSceneMaterial& material,
    const Fvector& position)
{
    Msg("[dev_level] event=sample group=%s name=%s pos=%.2f,%.2f,%.2f shader=%s opacity=%.2f roughness=%.2f",
        group, material.name.c_str(), position.x, position.y, position.z,
        material.shaderName.empty() ? "opaque" : material.shaderName.c_str(),
        material.opacity, material.roughness);
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
    const u32 glass_clear = AddMaterial(scene, "glass_clear", 0.88f, 0.89f, 0.90f, 0.0f, 0.05f,
        "models\\transparent", 0.12f);
    const u32 glass_green = AddMaterial(scene, "glass_green", 0.52f, 0.85f, 0.58f, 0.0f, 0.08f,
        "models\\transparent", 0.25f);
    const u32 glass_blue = AddMaterial(scene, "glass_blue", 0.48f, 0.66f, 0.92f, 0.0f, 0.12f,
        "models\\transparent", 0.3f);
    const u32 glass_rough = AddMaterial(scene, "glass_rough", 0.82f, 0.83f, 0.85f, 0.0f, 0.55f,
        "models\\transparent", 0.4f);
    const u32 glassMaterials[] = { glass_clear, glass_green, glass_blue, glass_rough };
    const float glassX[] = { -6.0f, -2.0f, 2.0f, 6.0f };
    for (u32 index = 0; index < 4; ++index)
    {
        const Fvector position{ glassX[index], 1.5f, -1.0f };
        const auto& sample = scene.materials[glassMaterials[index]];
        AddGlassDisplay(scene, sample.name.c_str(), position, Fvector{ 0.9f, 1.15f, 0.045f },
            glassMaterials[index], plinth);
        AddSampleRecord("glass", sample, position);
    }
    const u32 ref_silver = AddMaterial(scene, "reference_polished_silver", 0.95f, 0.93f, 0.88f, 1.0f, 0.025f);
    const u32 ref_copper = AddMaterial(scene, "reference_copper", 0.955f, 0.637f, 0.538f, 1.0f, 0.18f);
    const u32 ref_black = AddMaterial(scene, "reference_glossy_black", 0.02f, 0.02f, 0.02f, 0.0f, 0.07f);
    const u32 ref_white = AddMaterial(scene, "reference_matte_white", 0.8f, 0.8f, 0.8f, 0.0f, 0.92f);
    const u32 referenceMaterials[] = { ref_silver, ref_copper, ref_black, ref_white };
    const float referenceZ[] = { -1.0f, 2.0f, 5.0f, 8.0f };
    for (u32 index = 0; index < 4; ++index)
    {
        const Fvector position{ -10.0f, 1.5f, referenceZ[index] };
        const auto& sample = scene.materials[referenceMaterials[index]];
        AddPlinthSample(scene, sample.name.c_str(), position, 0.6f, 0.9f, referenceMaterials[index], plinth);
        AddSampleRecord("reference", sample, position);
    }
    const u32 emissive_warm = AddMaterial(scene, "emissive_warm", 0.95f, 0.55f, 0.2f, 0.0f, 0.5f,
        "models\\selflight");
    const u32 emissive_cool = AddMaterial(scene, "emissive_cool", 0.2f, 0.5f, 1.0f, 0.0f, 0.5f,
        "models\\selflight");
    const u32 emissive_dim = AddMaterial(scene, "emissive_dim", 0.85f, 0.85f, 0.85f, 0.0f, 0.5f,
        "models\\selflight_det");
    const u32 emissiveMaterials[] = { emissive_warm, emissive_cool, emissive_dim };
    const pcstr witnessNames[] = { "witness_warm", "witness_cool", "witness_dim" };
    const float emissiveX[] = { -6.0f, 0.0f, 6.0f };
    for (u32 index = 0; index < 3; ++index)
    {
        const Fvector position{ emissiveX[index], 2.4f, 10.8f };
        const auto& sample = scene.materials[emissiveMaterials[index]];
        AddPanelDisplay(scene, sample.name.c_str(), position, Fvector{ 0.9f, 0.75f, 0.08f },
            emissiveMaterials[index], plinth);
        AddSampleRecord("emissive", sample, position);
        const Fvector witness{ emissiveX[index], 0.5f, 9.3f };
        AddBox(scene, witnessNames[index], witness, Fvector{ 0.5f, 0.5f, 0.5f }, ref_white);
        AddSampleRecord("witness", scene.materials[ref_white], witness);
    }
    Msg("[dev_level] event=room name=%s meshes=%u materials=%u bounds=%.1f,%.1f,%.1f:%.1f,%.1f,%.1f",
        scene.name.c_str(), static_cast<u32>(scene.meshes.size()), static_cast<u32>(scene.materials.size()),
        scene.bounds.vMin.x, scene.bounds.vMin.y, scene.bounds.vMin.z,
        scene.bounds.vMax.x, scene.bounds.vMax.y, scene.bounds.vMax.z);
    for (u32 index = 0; index < scene.materials.size(); ++index)
    {
        const auto& material = scene.materials[index];
        Msg("[dev_level] event=material id=%u name=%s color=%.3f,%.3f,%.3f metallic=%.3f roughness=%.3f shader=%s opacity=%.3f",
            index, material.name.c_str(), material.color.x, material.color.y, material.color.z,
            material.metallic, material.roughness,
            material.shaderName.empty() ? "opaque" : material.shaderName.c_str(), material.opacity);
    }
}
