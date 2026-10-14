#include "xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/EngineLevelModels.h"
#include "src/Layers/xrRenderVK/GpuModel.h"
#include "src/Layers/xrRenderVK/VulkanModelVisual.h"
#include "src/Layers/xrRenderVK/VulkanVisual.h"
#include "src/xrEngine/defines.h"
#include "vulkan_mock_device.h"
#include <filesystem>
#include <fstream>
#include <gli/save_dds.hpp>
#include <gli/texture.hpp>
#include <limits>
using namespace xray::render::vulkan;
namespace
{
void put32(std::vector<uint8_t> &bytes, uint32_t n)
{
    for (unsigned i = 0; i < 4; ++i)
        bytes.push_back(static_cast<uint8_t>(n >> (i * 8)));
}
void put16(std::vector<uint8_t> &bytes, uint16_t n)
{
    bytes.push_back(static_cast<uint8_t>(n));
    bytes.push_back(static_cast<uint8_t>(n >> 8));
}
void putfloat(std::vector<uint8_t> &bytes, float value)
{
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    put32(bytes, bits);
}
void chunk(std::vector<uint8_t> &dst, uint32_t id, const std::vector<uint8_t> &body)
{
    put32(dst, id);
    put32(dst, static_cast<uint32_t>(body.size()));
    dst.insert(dst.end(), body.begin(), body.end());
}
VisualRecord record(unsigned links)
{
    VisualRecord child;
    child.type = 5;
    std::vector<uint8_t> vertices, indices;
    constexpr uint32_t formats[]{0, 0x12071980, 0x240e3300, 0x481c6600, 0x5a238f80};
    put32(vertices, formats[links]);
    put32(vertices, 3);
    for (unsigned v = 0; v < 3; ++v)
    {
        if (links > 1)
            for (unsigned i = 0; i < links; ++i)
                put16(vertices, i);
        for (unsigned i = 0; i < 12; ++i)
            putfloat(vertices, i == 0 ? float(v) : 0);
        if (links > 1)
            for (unsigned i = 0; i + 1 < links; ++i)
                putfloat(vertices, links == 2 ? 0.25f : 1.0f / links);
        putfloat(vertices, 0.5f);
        putfloat(vertices, 0.75f);
        if (links == 1)
            put32(vertices, 3);
    }
    put32(indices, 3);
    put16(indices, 0);
    put16(indices, 1);
    put16(indices, 2);
    chunk(child.source, 3, vertices);
    chunk(child.source, 4, indices);
    VisualRecord root;
    root.type = 3;
    root.embedded_children.push_back(std::move(child));
    return root;
}
} // namespace
namespace
{
using Bytes = std::vector<uint8_t>;
void save(const std::filesystem::path &name, const Bytes &bytes)
{
    std::ofstream file(name, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    assert(file.good());
}
Bytes compressed(uint32_t id, Bytes body)
{
    CMemoryWriter writer;
    writer.w_chunk(id | CFS_CompressMark, body.data(), body.size());
    const auto *bytes = static_cast<const uint8_t *>(writer.pointer());
    return {bytes, bytes + writer.size()};
}
Bytes encoded(const VisualRecord &visual)
{
    Bytes header(44, 0), result;
    header[0] = 4;
    header[1] = visual.type;
    chunk(result, 1, header);
    Bytes texture{'f', 'i', 'x', 't', 'u', 'r', 'e', 0, 'd', 'e', 'f', 'a', 'u', 'l', 't', 0};
    chunk(result, 2, texture);
    result.insert(result.end(), visual.source.begin(), visual.source.end());
    if (!visual.embedded_children.empty())
    {
        Bytes children;
        unsigned id = 0;
        for (const auto &child : visual.embedded_children)
        {
            auto bytes = compressed(id++, encoded(child));
            children.insert(children.end(), bytes.begin(), bytes.end());
        }
        auto bytes = compressed(9, std::move(children));
        result.insert(result.end(), bytes.begin(), bytes.end());
    }
    return result;
}
VisualRecord plain(unsigned type)
{
    VisualRecord result;
    result.type = type;
    Bytes vb, ib, swi;
    put32(vb, 0x112);
    put32(vb, 3);
    for (unsigned v = 0; v < 3; ++v)
        for (unsigned f = 0; f < 8; ++f)
            putfloat(vb, f == 0 ? float(v) : f == 5 ? 1.f : 0.f);
    put32(ib, 3);
    put16(ib, 0);
    put16(ib, 1);
    put16(ib, 2);
    chunk(result.source, 3, vb);
    chunk(result.source, 4, ib);
    if (type == 2 || type == 4)
    {
        swi.resize(16);
        put32(swi, 1);
        put32(swi, 0);
        put16(swi, 1);
        put16(swi, 3);
        chunk(result.source, 6, swi);
    }
    return result;
}
} // namespace
int main()
{
    Core.Initialize("vulkan_asset_flow_test", nullptr, false);
    const auto directory = std::filesystem::temp_directory_path() / "openxray_vulkan_asset_flow";
    std::filesystem::create_directories(directory);
    const auto base_textures = directory / "base_textures";
    std::filesystem::create_directories(base_textures);
    gli::texture image(gli::TARGET_2D, gli::FORMAT_RGBA8_UNORM_PACK8, {4, 4, 1}, 1, 1, 1);
    std::memset(image.data(), 255, image.size());
    std::vector<char> dds;
    assert(gli::save_dds(image, dds));
    save(directory / "fixture.dds", Bytes(dds.begin(), dds.end()));
    FS.append_path("$level$", directory.string().c_str(), nullptr, false);
    FS.append_path("$game_textures$", base_textures.string().c_str(), nullptr, false);
    FS.append_path("$game_meshes$", directory.string().c_str(), nullptr, false);
    using namespace vk_mock;
    const auto device = handle<VkDevice>(1);
    const auto queue = handle<VkQueue>(2);
    const auto pool_handle = handle<VkCommandPool>(3);
    const auto rp = handle<VkRenderPass>(4);
    VkPhysicalDeviceMemoryProperties memory{};
    memory.memoryTypeCount = 1;
    memory.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    DeferredPass pass;
    std::string error;
    assert(pass.initialize(device, rp, rp, handle<VkShaderModule>(5), handle<VkShaderModule>(6), handle<VkShaderModule>(7), handle<VkShaderModule>(8),
                           handle<VkShaderModule>(9), handle<VkShaderModule>(10), handle<VkShaderModule>(11), scene_dispatch(), error));
    assert(pass.create_game_pipeline("vk\\object_opaque.vs", "vk\\object_opaque.ps", handle<VkShaderModule>(5), handle<VkShaderModule>(6), SurfaceMode::Opaque,
                                     false, error));
    for (unsigned weights = 1; weights <= 4; ++weights)
    {
        assert(pass.create_game_pipeline("vk\\skinned_" + std::to_string(weights) + ".vs", "vk\\object_opaque.ps", handle<VkShaderModule>(5),
                                         handle<VkShaderModule>(6), SurfaceMode::Opaque, false, error, true));
        assert(pass.create_game_pipeline("vk\\hud_skinned_" + std::to_string(weights) + ".vs", "vk\\hud_blended.ps", handle<VkShaderModule>(5),
                                         handle<VkShaderModule>(6), SurfaceMode::Opaque, true, error, true));
    }
    GameTextureFactory textures;
    {
        const auto thm = [](const char* detail, float scale, bool malformed = false)
        {
            Bytes data, parameters(malformed ? 7 : 8, 0), extension;
            if (!malformed) parameters[6] = 0x80; // diffuse detail
            extension.insert(extension.end(), detail, detail + std::strlen(detail) + 1);
            putfloat(extension, scale);
            chunk(data, 0x0812, parameters);
            chunk(data, 0x0815, extension);
            return data;
        };
        // A loose $level$ replacement takes priority over a game texture
        // metadata file with the same arbitrary name. Removing it restores
        // the lower-priority result; a malformed override must fail visibly.
        save(base_textures / "arb_42.thm", thm("base_detail", 1.25f));
        save(directory / "arb_42.thm", thm("mod_detail", 3.5f));
        FS.rescan_path(FS.get_path("$game_textures$")->m_Path, false);
        FS.rescan_path(FS.get_path("$level$")->m_Path, false);
        GameTextureFactory::DetailMetadata detail;
        std::string metadata_error;
        assert(textures.texture_detail_metadata("arb_42", detail, metadata_error) &&
            detail.diffuse && detail.texture == "mod_detail" && detail.scale == 3.5f);
        save(directory / "arb_42.thm", thm("mod_detail", 3.5f, true));
        FS.rescan_path(FS.get_path("$level$")->m_Path, false);
        assert(!textures.texture_detail_metadata("arb_42", detail, metadata_error) &&
            metadata_error.find("arb_42.thm") != std::string::npos &&
            metadata_error.find("TEXTUREPARAM") != std::string::npos);
        std::filesystem::remove(directory / "arb_42.thm");
        FS.rescan_path(FS.get_path("$level$")->m_Path, false);
        assert(textures.texture_detail_metadata("arb_42", detail, metadata_error) &&
            detail.diffuse && detail.texture == "base_detail" && detail.scale == 1.25f);
    }
    {
        Bytes params(8, 0), extension{'m','o','d','\\','d','e','t','a','i','l',0};
        params[6] = 0x80; // flDiffuseDetail (bit 23)
        putfloat(extension, 2.5f);
        GameTextureFactory::DetailMetadata metadata;
        assert(GameTextureFactory::decode_detail_metadata(params.data(), params.size(),
            extension.data(), extension.size(), metadata, error));
        assert(metadata.diffuse && !metadata.normal && metadata.texture == "mod\\detail" &&
            metadata.scale == 2.5f);
        params[7] = 0x04; // flBumpDetail (bit 26)
        assert(GameTextureFactory::decode_detail_metadata(params.data(), params.size(),
            extension.data(), extension.size(), metadata, error) && metadata.normal);
        assert(!GameTextureFactory::decode_detail_metadata(params.data(), 7,
            extension.data(), extension.size(), metadata, error) &&
            error.find("TEXTUREPARAM") != std::string::npos);
        assert(!GameTextureFactory::decode_detail_metadata(params.data(), params.size(),
            extension.data(), 10, metadata, error) &&
            error.find("DETAIL_EXT") != std::string::npos);
        Bytes bump;
        putfloat(bump, .08f);
        bump.insert(bump.end(), {2, 0, 0, 0, 'm', 'o', 'd', '\\', 'n', 'o', 'r', 'm', 'a', 'l', 0});
        Bytes material{2, 0, 0, 0};
        putfloat(material, .4f);
        Bytes normal{'m', 'o', 'd', '\\', 'e', 'x', 't', 0};
        GameTextureFactory::MetadataExtras extras{bump.data(), bump.size(),
            material.data(), material.size(), normal.data(), normal.size()};
        assert(GameTextureFactory::decode_detail_metadata(params.data(), params.size(),
            extension.data(), extension.size(), metadata, error, extras));
        assert(metadata.bump_mode == 2 && metadata.bump_texture == "mod\\normal" &&
            metadata.external_normal_texture == "mod\\ext" && metadata.material == 2 &&
            metadata.material_weight > .39f && metadata.material_weight < .41f);
        extras.bump_size = 8;
        assert(!GameTextureFactory::decode_detail_metadata(params.data(), params.size(),
            extension.data(), extension.size(), metadata, error, extras) &&
            error.find("BUMP") != std::string::npos);
        extras.bump_size = bump.size();
        extras.external_normal_size = normal.size() - 1;
        assert(!GameTextureFactory::decode_detail_metadata(params.data(), params.size(),
            extension.data(), extension.size(), metadata, error, extras) &&
            error.find("NORMALMAP") != std::string::npos);
    }
    VkPhysicalDeviceFeatures features{};
    VkPhysicalDeviceProperties properties{};
    assert(textures.initialize(device, queue, pool_handle, memory, false,
        features, properties, 1.0f, texture_dispatch(), sampler, destroy_sampler, idle, error));
    VkDescriptorSet wrap_set{}, clamp_set{};
    assert(textures.material("fixture", pass, wrap_set, error) &&
        textures.material("fixture", pass, clamp_set, error, true));
    assert(wrap_set && clamp_set && wrap_set != clamp_set);
    textures.release_material(wrap_set, pass);
    textures.release_material(clamp_set, pass);
    ScenePass ui_pass;
    assert(ui_pass.initialize(device, rp, handle<VkShaderModule>(5), handle<VkShaderModule>(6),
        handle<VkShaderModule>(7), handle<VkShaderModule>(8), scene_dispatch(), error, true));
    VkDescriptorSet ui_clamp{}, ui_wrap{};
    assert(textures.ui("fixture", ui_pass, ui_clamp, error) &&
        textures.ui("fixture", ui_pass, ui_wrap, error, nullptr, false));
    assert(ui_clamp && ui_wrap && ui_clamp != ui_wrap);
    assert(sampler_address.at(sampled_by.at(ui_clamp)) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    assert(sampler_address.at(sampled_by.at(ui_wrap)) == VK_SAMPLER_ADDRESS_MODE_REPEAT);
    assert(textures.reload_assets(error));
    assert(sampler_address.at(sampled_by.at(ui_clamp)) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    assert(sampler_address.at(sampled_by.at(ui_wrap)) == VK_SAMPLER_ADDRESS_MODE_REPEAT);
    assert(textures.retain_ui(ui_wrap, ui_pass));
    textures.release_ui(ui_clamp, ui_pass);
    textures.release_ui(ui_wrap, ui_pass);
    textures.release_ui(ui_wrap, ui_pass);
    std::array<std::string, 20> surface_names;
    surface_names.fill("fixture");
    SurfaceParameters surface_parameters{};
    VkDescriptorSet surface_wrap{}, surface_clamp{};
    assert(textures.surface_material(surface_names, surface_parameters, pass,
        surface_wrap, error) &&
        textures.surface_material(surface_names, surface_parameters, pass,
            surface_clamp, error, false, true));
    assert(surface_wrap && surface_clamp && surface_wrap != surface_clamp);
    assert(sampler_address.at(sampled_by.at(surface_wrap)) == VK_SAMPLER_ADDRESS_MODE_REPEAT);
    assert(sampler_address.at(sampled_by.at(surface_clamp)) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    assert(textures.reload_assets(error));
    assert(sampler_address.at(sampled_by.at(surface_wrap)) == VK_SAMPLER_ADDRESS_MODE_REPEAT);
    assert(sampler_address.at(sampled_by.at(surface_clamp)) == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    textures.release_terrain_material(surface_wrap, pass);
    textures.release_terrain_material(surface_clamp, pass);
    // All twenty surface roles retain their own DDS across a reload. The key
    // must also distinguish a changed slot and per-material surface factors.
    std::array<std::string, 20> distinct_names{};
    for (size_t role = 0; role < distinct_names.size(); ++role)
    {
        distinct_names[role] = "surface_role_" + std::to_string(role);
        save(directory / (distinct_names[role] + ".dds"), Bytes(dds.begin(), dds.end()));
    }
    FS.rescan_path(FS.get_path("$level$")->m_Path, false);
    const size_t before_surface = textures.resident_count();
    VkDescriptorSet roles_a{}, roles_again{}, changed_parameter{}, changed_slot{};
    assert(textures.surface_material(distinct_names, surface_parameters, pass, roles_a, error));
    assert(textures.surface_material(distinct_names, surface_parameters, pass, roles_again, error));
    assert(roles_a && roles_a == roles_again &&
        textures.resident_count() == before_surface + distinct_names.size());
    surface_parameters.emissive = .3f;
    assert(textures.surface_material(distinct_names, surface_parameters, pass,
        changed_parameter, error) && changed_parameter != roles_a);
    surface_parameters.emissive = 0;
    auto swapped_names = distinct_names;
    std::swap(swapped_names[11], swapped_names[12]);
    assert(textures.surface_material(swapped_names, surface_parameters, pass,
        changed_slot, error) && changed_slot != roles_a && changed_slot != changed_parameter);
    assert(textures.reload_assets(error) &&
        textures.resident_count() == before_surface + distinct_names.size());
    VkDescriptorSet after_reload{};
    assert(textures.surface_material(distinct_names, surface_parameters, pass,
        after_reload, error) && after_reload == roles_a);
    textures.release_terrain_material(roles_again, pass);
    textures.release_terrain_material(after_reload, pass);
    textures.release_terrain_material(changed_parameter, pass);
    textures.release_terrain_material(changed_slot, pass);
    assert(textures.resident_count() == before_surface + distinct_names.size());
    textures.release_terrain_material(roles_a, pass);
    textures.retire_unused();
    assert(textures.resident_count() == before_surface);
    // The mock uses an actual serialized blender definition. Runtime loading
    // must never infer alpha/depth behavior from a texture or shader name.
    {
        Bytes blender(176, 0), records, library;
        const char cls[] = "LM      ";
        for (size_t i = 0; i < 8; ++i) blender[i] = cls[7 - i];
        std::memcpy(blender.data() + 8, "default", 8);
        blender[172] = 1;
        chunk(records, 0, blender);
        chunk(library, 2, records);
        IReader reader(library.data(), library.size());
        assert(textures.load_blender_library(reader, error));
    }
    FrameRecordingContext frame{handle<VkCommandBuffer>(12), rp, handle<VkFramebuffer>(13), {960, 540}, 0, 0};
    float mvp[16]{};
    mvp[0] = mvp[5] = mvp[10] = mvp[15] = 1;
    assert(pass.create_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps", handle<VkShaderModule>(5), handle<VkShaderModule>(6), SurfaceMode::Opaque,
                                     false, error));
    for (auto profile : {LevelGameProfile::SoC, LevelGameProfile::CS, LevelGameProfile::CoP})
    {
        ShadowOfChernobylMode = profile == LevelGameProfile::SoC;
        ClearSkyMode = profile == LevelGameProfile::CS;
        CallOfPripyatMode = profile == LevelGameProfile::CoP;
        Bytes vb, ib, geom, level, shaders, visual, container, header(44, 0), table;
        put32(vb, 1);
        auto decl = [&](uint16_t offset, uint8_t type, uint8_t usage) {
            vb.insert(vb.end(), {0, 0, uint8_t(offset), uint8_t(offset >> 8), type, 0, usage, 0});
        };
        decl(0, 2, 0);
        decl(12, 2, 3);
        decl(24, 1, 5);
        vb.insert(vb.end(), {0xff, 0, 0, 0, 17, 0, 0, 0});
        put32(vb, 3);
        for (unsigned v = 0; v < 3; ++v)
            for (unsigned f = 0; f < 8; ++f)
                putfloat(vb, f == 0 ? float(v) : f == 5 ? 1.f : 0.f);
        put32(ib, 1);
        put32(ib, 3);
        put16(ib, 0);
        put16(ib, 1);
        put16(ib, 2);
        chunk(geom, 9, vb);
        chunk(geom, 10, ib);
        save(directory / "level.geom", geom);
        FS.rescan_path(FS.get_path("$level$")->m_Path, false);
        put32(shaders, 1);
        const char material[] = "default/fixture";
        shaders.insert(shaders.end(), material, material + sizeof(material));
        header[0] = 4;
        chunk(visual, 1, header);
        for (unsigned n : {0u, 0u, 3u, 0u, 0u, 3u})
            put32(container, n);
        chunk(visual, 21, container);
        table = compressed(0, visual);
        chunk(level, 1, Bytes{14, 0, 1, 0});
        chunk(level, 2, shaders);
        auto packed = compressed(3, table);
        level.insert(level.end(), packed.begin(), packed.end());
        IReader reader(level.data(), level.size());
        LevelModelData data;
        assert(load_engine_level_models(reader, data, error) && data.models.size() == 1);
        std::vector<VisualRecord> catalog;
        assert(load_engine_visual_catalog(reader, catalog, error) && catalog.size() == 1);
        GpuLevel gpu;
        assert(gpu.load(reader, device, queue, pool_handle, memory, upload_dispatch(), textures, pass, error));
        assert(gpu.visual_count() == 1 && gpu.model_count() == 1 && textures.finish_uploads());
        if (profile == LevelGameProfile::SoC)
        {
            const auto resident = textures.resident_count();
            const auto live_images = images.size();
            VkImageView weather{};
            assert(textures.environment("fixture", weather, error));
            const VkImageView original_weather = weather;
            unsigned callbacks = 0;
            const auto before = [&] { ++callbacks; textures.release_environment(weather); weather = VK_NULL_HANDLE; };
            const auto after = [&] { ++callbacks; assert(textures.environment("fixture", weather, error)); };
            save(directory / "fixture.dds", Bytes{1, 2, 3, 4});
            assert(!textures.reload_assets(error, before, after));
            assert(callbacks == 0 && weather == original_weather);
            assert(textures.resident_count() == resident && images.size() == live_images);
            save(directory / "fixture.dds", Bytes(dds.begin(), dds.end()));
            assert(textures.reload_assets(error, before, after) && error.empty());
            assert(callbacks == 2 && weather && weather != original_weather);
            assert(textures.resident_count() == resident && images.size() == live_images);
            textures.release_environment(weather);
        }
        const auto before = draws;
        assert(gpu.record(frame, pass, mvp) && draws == before + 1);
        // A mod may supply an incomplete optional geomX. Keep the valid
        // primary geometry for each profile and report the fallback at VFS load.
        Bytes partial_fast;
        chunk(partial_fast, 9, vb);
        save(directory / "level.geomX", partial_fast);
        FS.rescan_path(FS.get_path("$level$")->m_Path, false);
        LevelModelData fallback;
        assert(load_engine_level_models(reader, fallback, error) &&
            fallback.models.size() == 1 && fallback.models[0].indices.size() == 3);
        std::filesystem::remove(directory / "level.geomX");
        FS.rescan_path(FS.get_path("$level$")->m_Path, false);
        // A missing VB chunk must leave the uploaded scene available.
        Bytes missing;
        chunk(missing, 10, ib);
        save(directory / "level.geom", missing);
        FS.rescan_path(FS.get_path("$level$")->m_Path, false);
        assert(!gpu.load(reader, device, queue, pool_handle, memory, upload_dispatch(), textures, pass, error));
        assert(error.find(level_profile_name(profile)) != std::string::npos && error.find("level.geom") != std::string::npos);
        assert(gpu.visual_count() == 1 && gpu.record(frame, pass, mvp));
        gpu.destroy();
        assert(buffers.empty() && allocations.empty() && images.empty());
    }
    for (unsigned type = 0; type <= 5; ++type)
        for (unsigned weights = 1; weights <= (type >= 3 ? 4u : 1u); ++weights)
        {
            VisualRecord source = type >= 3 ? record(weights) : plain(type == 1 ? 0 : type);
            if (type == 1)
            {
                VisualRecord hierarchy;
                hierarchy.type = 1;
                hierarchy.embedded_children.push_back(source);
                source = std::move(hierarchy);
            }
            if (type == 4 || type == 5)
            {
                auto leaf = std::move(source.embedded_children[0]);
                source = std::move(leaf);
                source.type = type;
                if (type == 4)
                {
                    Bytes swi(16);
                    put32(swi, 1);
                    put32(swi, 0);
                    put16(swi, 1);
                    put16(swi, 3);
                    chunk(source.source, 6, swi);
                }
            }
            auto bytes = encoded(source);
            save(directory / "matrix.ogf", bytes);
            FS.rescan_path(FS.get_path("$level$")->m_Path, false);
            VisualRecord parsed;
            if (!load_engine_model_visual("matrix.ogf", parsed, error))
            {
                std::fprintf(stderr, "type=%u: %s\n", type, error.c_str());
                assert(false);
            }
            assert(parsed.type == type);
            auto gpu = std::make_shared<GpuModel>();
            assert(gpu->load_record(parsed, "fixture", device, queue, pool_handle, memory, upload_dispatch(), textures, pass, error));
            assert(textures.finish_uploads());
            auto instance_record = parsed;
            if (type >= 3)
                instance_record.type = 3; // skinned leaves are owned by a skeleton
            auto visual = std::make_unique<VulkanModelVisual>(instance_record, "matrix.ogf", gpu);
            if (type >= 3)
            {
                Bytes names;
                put32(names, 4);
                for (unsigned bone = 0; bone < 4; ++bone)
                {
                    const auto name = std::string("bone") + std::to_string(bone);
                    const std::string parent = bone ? "bone0" : "";
                    names.insert(names.end(), name.begin(), name.end());
                    names.push_back(0);
                    names.insert(names.end(), parent.begin(), parent.end());
                    names.push_back(0);
                    names.resize(names.size() + 60);
                }
                Bytes ogf;
                chunk(ogf, 13, names);
                SkeletonBones bones;
                assert(parse_skeleton_bones({ogf.data(), ogf.size()}, bones, error));
                auto skeleton = VulkanKinematics::create(bones, visual.get(), error);
                assert(skeleton);
                ModelGeometry geometry;
                assert(decode_model_geometry(parsed, geometry, error));
                if (type != 3)
                {
                    ModelGeometry root;
                    root.type = 3;
                    root.children.push_back(std::move(geometry));
                    geometry = std::move(root);
                }
                assert(skeleton->attach_geometry(geometry, error));
                visual->set_skeleton(std::move(skeleton));
            }
            const auto before = draws;
            for (unsigned slot = 0; slot < FrameContext::FramesInFlight; ++slot)
            {
                frame.frame_index = slot;
                if (type >= 3)
                    assert(gpu->record_animated(frame, pass, mvp, *visual->skeleton(), error));
                else
                    assert(gpu->record(frame, pass, mvp, nullptr, 0, error));
            }
            assert(draws == before + FrameContext::FramesInFlight);
            auto copy = std::make_unique<VulkanModelVisual>(*visual);
            if (type >= 3)
            {
                assert(copy->skeleton() != visual->skeleton());
                assert(gpu->record_animated(frame, pass, mvp, *copy->skeleton(), error));
                const auto before_hud = draws;
                assert(gpu->record_animated(frame, pass, mvp, *visual->skeleton(), error, GeometryPhase::Hud));
                assert(gpu->record_animated(frame, pass, mvp, *copy->skeleton(), error, GeometryPhase::Hud));
                assert(draws == before_hud + 2); // All 1–4 weight GPU HUD variants.
                visual->release_pose_buffers();
                assert(gpu->record_animated(frame, pass, mvp, *copy->skeleton(), error));
                visual->reset_instance_state();
                assert(!visual->skeleton()->GetUpdateTracksCalback());
                assert(gpu->record_animated(frame, pass, mvp, *visual->skeleton(), error, GeometryPhase::Hud));
                for (int spawn = 0; spawn < 3; ++spawn)
                {
                    auto replacement = std::make_unique<VulkanModelVisual>(*visual);
                    assert(replacement->skeleton() != visual->skeleton());
                    assert(gpu->record_animated(frame, pass, mvp, *replacement->skeleton(), error,
                        GeometryPhase::Hud));
                    replacement->release_pose_buffers();
                    replacement.reset();
                }
            }
            gpu.reset();
            visual.reset();
            assert(!buffers.empty());
            copy.reset();
            assert(buffers.empty() && allocations.empty() && images.empty() && textures.resident_count() == 0);
            // Corrupt data must include filename and type/id and preserve output.
            if (type == 0 || type == 2 || type == 4 || type == 5)
            {
                bytes.pop_back();
                IReader reader(bytes.data(), bytes.size());
                ModelGeometry previous;
                previous.type = 99;
                assert(!load_engine_model_geometry("broken.ogf", &reader, previous, error));
                assert(error.find("broken.ogf") != std::string::npos && previous.type == 99);
            }
        }
    // Nested compression is normalized through IReader, including empty chunks.
    auto nested = encoded(record(1));
    IReader compressed_reader(nested.data(), nested.size());
    VisualRecord normalized;
    assert(load_engine_model_reader(compressed_reader, "compressed.ogf", normalized, error));
    assert(normalized.embedded_children.size() == 1);
    auto oversized = compressed(1, Bytes(44, 0));
    oversized[8] = 0xff;
    oversized[9] = 0xff;
    oversized[10] = 0xff;
    oversized[11] = 0x7f;
    IReader invalid_compressed(oversized.data(), oversized.size());
    assert(!load_engine_model_reader(invalid_compressed, "bad-compression.ogf", normalized, error));
    assert(error.find("bad-compression.ogf") != std::string::npos && error.find("id=1") != std::string::npos);
    textures.destroy();
    pass.destroy();
    assert(buffers.empty() && allocations.empty() && images.empty() && submits > 0 && descriptor_frees > 0);
    std::filesystem::remove_all(directory);
}
