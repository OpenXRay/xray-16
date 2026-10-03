#include "xrEngine/stdafx.h"
#include "VulkanLevelRender.h"
#include "VulkanGameDevice.h"
#include "VulkanModelVisual.h"
#include "SpecialVisuals.h"
#include "ParticleVisual.h"
#include "EngineParticleSource.h"
#include "VulkanVisual.h"
#include "VulkanUIShader.h"
#include "VulkanDrawUtils.h"
#include "VulkanDebugRender.h"
#include "VulkanWallMarkArray.h"
#include "WallmarkGeometry.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Rain.h"
#include "xrEngine/thunderbolt.h"
#include "xrEngine/device.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/IGameFont.hpp"
#include "xrEngine/vis_common.h"
#include "xrCore/FMesh.hpp"
#include "xrCore/Media/Image.hpp"
#include "xrCore/PostProcess/PPInfo.hpp"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <functional>
#include <type_traits>
#include <vector>

namespace xray::render::vulkan
{
static_assert(!std::is_abstract_v<VulkanLevelRender>, "Vulkan IRender implementation must be complete");
VulkanLevelRender::VulkanLevelRender() = default;
VulkanLevelRender::VulkanLevelRender(VulkanGameDevice& device) : external_game_device_(&device) {}
void VulkanLevelRender::create()
{
    m_skinning = -1;
    m_MSAASample = -1;
}
void VulkanLevelRender::destroy()
{
    if (frame_phase_.active()) frame_phase_.reset();
    destroy_all_models();
    level_Unload();
}

void VulkanLevelRender::DumpStatistics(IGameFont& font, IPerformanceAlert*)
{
    font.OutNext("Vulkan: %u draws, %u triangles", frame_draw_calls_, frame_triangles_);
    font.OutNext("Textures: %zu, %llu bytes", textures_ ? textures_->resident_count() : 0,
        static_cast<unsigned long long>(textures_ ? textures_->resident_bytes() : 0));
    font.OutNext("Models: %zu live, %zu pooled", models_.size(), model_pool_.size());
}

xrImTextureData VulkanLevelRender::GetImGuiTextureId(pcstr name)
{
    xrImTextureData result{};
    if (!game_device_ || !name || !*name) return result;
    VkDescriptorSet descriptor{};
    VkExtent2D extent{};
    auto it = imgui_textures_.find(name);
    if (it == imgui_textures_.end())
    {
        std::string error;
        if (!textures_->ui(name, game_device_->ui_pass(), descriptor, error, &extent))
        { Msg("! [renderer-vulkan] ImGui texture %s: %s", name, error.c_str()); return result; }
        imgui_textures_.emplace(name, ImGuiTexture{descriptor, extent});
    }
    else { descriptor = it->second.descriptor; extent = it->second.extent; }
    static_assert(sizeof(descriptor) <= sizeof(result.texture));
    std::memcpy(&result.texture, &descriptor, sizeof(descriptor));
    result.size.set(float(extent.width), float(extent.height));
    return result;
}

void VulkanLevelRender::models_Prefetch()
{
    // Uploads are queued at material creation; force completion at the engine's prefetch barrier.
    ResourcesDeferredUpload();
}
bool VulkanLevelRender::occ_visible(vis_data& visual) { return occ_visible(visual.box); }
bool VulkanLevelRender::occ_visible(Fbox& bounds)
{
    u32 mask = FRUSTUM_P_ALL;
    return ViewBase.testAABB(bounds.data(), mask) != fcvNone;
}
bool VulkanLevelRender::occ_visible(sPoly&) { return true; }
void VulkanLevelRender::BeforeWorldRender() {}
void VulkanLevelRender::AfterWorldRender() {}
void VulkanLevelRender::ObtainRequiredWindowFlags(u32& flags) { flags |= SDL_WINDOW_VULKAN; }
void VulkanLevelRender::overdrawBegin() {}
void VulkanLevelRender::overdrawEnd() {}
void VulkanLevelRender::DeferredLoad(bool enabled) { deferred_load_ = enabled; }
void VulkanLevelRender::ResourcesDeferredUpload()
{
    if (textures_) R_ASSERT2(textures_->finish_uploads(), "Vulkan deferred texture upload failed");
}
void VulkanLevelRender::ResourcesDeferredUnload()
{
    if (textures_) textures_->retire_unused();
    models_Clear(false);
}
void VulkanLevelRender::ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
    const uint64_t bytes = textures_ ? textures_->resident_bytes() : 0;
    m_base = static_cast<u32>(std::min<uint64_t>(bytes, UINT32_MAX));
    c_base = textures_ ? static_cast<u32>(textures_->resident_count()) : 0;
    m_lmaps = c_lmaps = 0;
}
void VulkanLevelRender::ResourcesDestroyNecessaryTextures() { ResourcesDeferredUnload(); }
void VulkanLevelRender::ResourcesStoreNecessaryTextures() { ResourcesDeferredUpload(); }
void VulkanLevelRender::ResourcesDumpMemoryUsage()
{
    Msg("* [renderer-vulkan] textures: %zu, approximate bytes: %llu",
        textures_ ? textures_->resident_count() : 0,
        static_cast<unsigned long long>(textures_ ? textures_->resident_bytes() : 0));
}
void VulkanLevelRender::OnAssetsChanged()
{
    if (frame_phase_.active())
    {
        assets_dirty_ = true;
        return;
    }
    assets_dirty_ = false;
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan shader reload needs idle GPU frames");
    if (game_device_)
    {
        std::string shader_error;
        if (!game_device_->reload_game_shaders(shader_error))
            Msg("! [renderer-vulkan] keeping previous game pipelines after reload failure: %s",
                shader_error.c_str());
    }
    if (g_pGamePersistent && game_device_)
    {
        auto& environment = g_pGamePersistent->Environment();
        const auto visit = [](CEnvironment::EnvsMap& presets, bool create)
        {
            for (auto& [name, descriptors] : presets)
                for (CEnvDescriptor* descriptor : descriptors)
                {
                    if (!descriptor || !descriptor->m_pDescriptor) continue;
                    if (create) descriptor->on_device_create();
                    else descriptor->on_device_destroy();
                }
        };
        if (textures_)
        {
            std::string error;
            const auto before = [&]
            {
                // Old weather descriptors must be retired before old image views.
                environment.m_pRender->Clear();
                visit(environment.WeatherCycles, false);
                visit(environment.WeatherFXs, false);
            };
            const auto after = [&]
            {
                visit(environment.WeatherCycles, true);
                visit(environment.WeatherFXs, true);
            };
            if (!textures_->reload_assets(error, before, after))
                Msg("! [renderer-vulkan] keeping previous textures after reload failure: %s", error.c_str());
        }
    }
    else if (textures_)
    {
        std::string error;
        if (!textures_->reload_assets(error))
            Msg("! [renderer-vulkan] keeping previous textures after reload failure: %s", error.c_str());
    }
    models_Clear(false);
    particle_catalog_.reset();
}
void VulkanLevelRender::Screenshot(ScreenshotMode mode, pcstr name)
{
    if (!game_device_) return;
    if (mode != SM_NORMAL && (!name || !*name))
    { Msg("! [renderer-vulkan] screenshot requires a path or postfix"); return; }
    screenshot_ = std::make_unique<ScreenshotRequest>(ScreenshotRequest{mode, name ? name : ""});
    game_device_->request_screenshot();
}

void VulkanLevelRender::save_screenshot()
{
    if (!screenshot_) return;
    std::vector<u8> pixels;
    VkExtent2D extent{};
    VkFormat format{};
    if (!game_device_->take_screenshot(pixels, extent, format))
    { Msg("! [renderer-vulkan] screenshot readback unavailable on this surface"); screenshot_.reset(); return; }
    if (format == VK_FORMAT_B8G8R8A8_UNORM)
        for (size_t i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
    const auto request = std::move(screenshot_);
    string_path filename{};
    IWriter* writer = nullptr;
    if (request->mode == SM_NORMAL)
    {
        string64 time;
        xr_sprintf(filename, "ss_%s_%s_(%s).jpg", Core.UserName, timestamp(time),
            g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");
        writer = FS.w_open("$screenshots$", filename);
    }
    else if (request->mode == SM_FOR_GAMESAVE)
        writer = FS.w_open(request->name.c_str());
    else
    {
        strconcat(sizeof(filename), filename, request->name.c_str(), ".tga");
        writer = FS.w_open("$screenshots$", filename);
    }
    if (!writer) { Msg("! [renderer-vulkan] screenshot output could not be opened"); return; }

    if (request->mode == SM_NORMAL)
    {
        std::vector<u8> rgb(size_t(extent.width) * extent.height * 3);
        for (size_t i = 0, j = 0; i < pixels.size(); i += 4, j += 3)
            std::copy_n(pixels.data() + i, 3, rgb.data() + j);
        XRay::Media::Image img(extent.width, extent.height, rgb.data(), XRay::Media::ImageDataFormat::RGB8);
        if (!img.SaveJPEG(*writer, 100)) Msg("! [renderer-vulkan] JPEG screenshot encoding failed");
    }
    else if (request->mode == SM_FOR_GAMESAVE)
    {
        // 128x128 BC1 DDS, matching the game save preview's on-disk format.
        constexpr u32 side = 128;
        u32 header[32]{};
        header[0] = 0x20534444; header[1] = 124; header[2] = 0x81007;
        header[3] = side; header[4] = side; header[5] = side * side / 2;
        header[19] = 32; header[20] = 4; header[21] = 0x31545844;
        header[27] = 0x1000;
        writer->w(header, sizeof(header));
        std::array<u8, side * side * 3> scaled{};
        for (u32 y = 0; y < side; ++y) for (u32 x = 0; x < side; ++x)
        {
            const size_t src = (size_t(y) * extent.height / side * extent.width +
                size_t(x) * extent.width / side) * 4;
            const size_t dst = (size_t(y) * side + x) * 3;
            std::copy_n(pixels.data() + src, 3, scaled.data() + dst);
        }
        for (u32 by = 0; by < side; by += 4) for (u32 bx = 0; bx < side; bx += 4)
        {
            u8 low[3]{255, 255, 255}, high[3]{};
            for (u32 y = 0; y < 4; ++y) for (u32 x = 0; x < 4; ++x)
                for (u32 c = 0; c < 3; ++c)
                {
                    u8 v = scaled[((by + y) * side + bx + x) * 3 + c];
                    low[c] = std::min(low[c], v); high[c] = std::max(high[c], v);
                }
            const auto pack = [](const u8* v) -> u16
            { return u16((v[0] >> 3) << 11 | (v[1] >> 2) << 5 | (v[2] >> 3)); };
            u16 endpoints[2]{pack(high), pack(low)};
            if (endpoints[0] <= endpoints[1])
                endpoints[0] = endpoints[1] < 65535 ? endpoints[1] + 1 : 65535;
            u8 palette[4][3]{};
            for (u32 c = 0; c < 3; ++c)
            {
                palette[0][c] = high[c]; palette[1][c] = low[c];
                palette[2][c] = (2 * high[c] + low[c]) / 3;
                palette[3][c] = (high[c] + 2 * low[c]) / 3;
            }
            u32 selectors = 0;
            for (u32 y = 0; y < 4; ++y) for (u32 x = 0; x < 4; ++x)
            {
                const u8* p = &scaled[((by + y) * side + bx + x) * 3];
                u32 best = 0, distance = UINT32_MAX;
                for (u32 i = 0; i < 4; ++i)
                {
                    u32 d = 0;
                    for (u32 c = 0; c < 3; ++c) { int delta = int(p[c]) - palette[i][c]; d += delta * delta; }
                    if (d < distance) { distance = d; best = i; }
                }
                selectors |= best << (2 * (y * 4 + x));
            }
            writer->w(endpoints, sizeof(endpoints)); writer->w(&selectors, sizeof(selectors));
        }
    }
    else
    {
        const u32 side = extent.height;
        std::vector<u8> square(size_t(side) * side * 4);
        for (u32 y = 0; y < side; ++y) for (u32 x = 0; x < side; ++x)
        {
            const u32 source_x = u32(uint64_t(x) * extent.width / side);
            std::copy_n(pixels.data() + (size_t(y) * extent.width + source_x) * 4,
                4, square.data() + (size_t(y) * side + x) * 4);
        }
        XRay::Media::Image img(side, side, square.data(), XRay::Media::ImageDataFormat::RGBA8);
        img.SaveTGA(*writer, true);
    }
    FS.w_close(writer);
}
void VulkanLevelRender::SetPostProcessParams(const SPPInfo& ppi)
{
    gray_ = std::clamp(ppi.gray, 0.f, 1.f);
    postprocess_.effect[0] = std::max(ppi.blur, 0.f);
    postprocess_.effect[1] = ppi.duality.h;
    postprocess_.effect[2] = ppi.duality.v;
    postprocess_.effect[3] = gray_;
    postprocess_.noise[0] = std::clamp(ppi.noise.intensity, 0.f, 1.f);
    postprocess_.noise[1] = std::max(ppi.noise.grain, 1.f);
    postprocess_.noise[2] = std::max(ppi.noise.fps, .01f);
    postprocess_.tint[0] = ppi.color_base.r * 2.f;
    postprocess_.tint[1] = ppi.color_base.g * 2.f;
    postprocess_.tint[2] = ppi.color_base.b * 2.f;
    postprocess_.add[0] = ppi.color_add.r * 2.f;
    postprocess_.add[1] = ppi.color_add.g * 2.f;
    postprocess_.add[2] = ppi.color_add.b * 2.f;
    postprocess_.gray_weights[0] = ppi.color_gray.r;
    postprocess_.gray_weights[1] = ppi.color_gray.g;
    postprocess_.gray_weights[2] = ppi.color_gray.b;
    postprocess_.color_map[0] = std::clamp(ppi.cm_influence, 0.f, 1.f);
    postprocess_.color_map[1] = std::clamp(ppi.cm_interpolate, 0.f, 1.f);
    color_map_a_ = postprocess_.color_map[0] > 0.f && ppi.cm_tex1.size() ?
        ppi.cm_tex1.c_str() : "";
    color_map_b_ = !color_map_a_.empty() && ppi.cm_tex2.size() ?
        ppi.cm_tex2.c_str() : color_map_a_;
    if (color_map_a_.empty()) postprocess_.color_map[0] = 0.f;
}
void VulkanLevelRender::setGamma(float value) { gamma_ = std::clamp(value, .01f, 4.f); }
void VulkanLevelRender::setBrightness(float value) { brightness_ = std::clamp(value, 0.f, 4.f); }
void VulkanLevelRender::setContrast(float value) { contrast_ = std::clamp(value, 0.f, 4.f); }
void VulkanLevelRender::updateGamma() {}
namespace
{
bool wallmark_geometry(const Fvector (&triangle)[3], const Fvector& point,
    float size, const std::string& texture, ModelGeometry& result)
{
    if (!(size > EPS_L) || texture.empty() || !_valid(point)) return false;
    Fvector normal;
    normal.mknormal(triangle[0], triangle[1], triangle[2]);
    if (!_valid(normal) || normal.square_magnitude() < EPS_L) return false;
    Fvector axis;
    axis.set(std::abs(normal.y) > .99f ? 1.f : 0.f,
        std::abs(normal.y) > .99f ? 0.f : 1.f, 0.f);
    Fvector right, up;
    right.crossproduct(axis, normal).normalize_safe();
    up.crossproduct(normal, right).normalize_safe();
    std::array<WallmarkVertex, 3> input;
    for (size_t i = 0; i < 3; ++i)
    {
        Fvector delta;
        delta.sub(triangle[i], point);
        input[i].position = {triangle[i].x, triangle[i].y, triangle[i].z};
        input[i].u = .5f + delta.dotproduct(right) / size;
        input[i].v = .5f - delta.dotproduct(up) / size;
    }
    const auto clipped = clip_wallmark_triangle(input);
    if (clipped.empty()) return false;
    result = {};
    result.texture = texture;
    result.mode = SurfaceMode::Transparent;
    for (const auto& vertex : clipped)
    {
        ModelVertex model;
        Fvector offset;
        offset.sub(Device.vCameraPosition, point).normalize_safe();
        for (size_t axis = 0; axis < 3; ++axis)
        {
            const float component = axis == 0 ? offset.x : axis == 1 ? offset.y : offset.z;
            model.position[axis] = vertex.position[axis] + component * .003f;
        }
        model.normal[0] = normal.x;
        model.normal[1] = normal.y;
        model.normal[2] = normal.z;
        model.uv[0] = vertex.u;
        model.uv[1] = vertex.v;
        result.indices.push_back(static_cast<uint32_t>(result.vertices.size()));
        result.vertices.push_back(model);
    }
    return true;
}

std::string model_cache_name(pcstr name)
{
    std::string result = name ? name : "";
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c == '\\' ? '/' : c));
        });
    if (result.size() >= 4 && result.compare(result.size() - 4, 4, ".ogf") == 0)
        result.resize(result.size() - 4);
    return result;
}

float distance_squared(const Fvector& left, const Fvector& right)
{
    const float x = left.x - right.x;
    const float y = left.y - right.y;
    const float z = left.z - right.z;
    const float result = x * x + y * y + z * z;
    return std::isfinite(result) ? result : 0.f;
}

Fvector visual_center(const LevelVisual* visual)
{
    Fvector center;
    if (visual)
        center.set(visual->bounds[6], visual->bounds[7], visual->bounds[8]);
    else
        center.set(0.f, 0.f, 0.f);
    return center;
}

DeferredEnvironment current_environment()
{
    DeferredEnvironment environment;
    if (g_pGamePersistent)
    {
        const CEnvDescriptorMixer& current = g_pGamePersistent->Environment().CurrentEnv;
        environment.sun_direction[0] = current.sun_dir.x;
        environment.sun_direction[1] = current.sun_dir.y;
        environment.sun_direction[2] = current.sun_dir.z;
        environment.sun_color[0] = current.sun_color.x;
        environment.sun_color[1] = current.sun_color.y;
        environment.sun_color[2] = current.sun_color.z;
        environment.ambient_color[0] = current.ambient.x;
        environment.ambient_color[1] = current.ambient.y;
        environment.ambient_color[2] = current.ambient.z;
        environment.hemi_color[0] = current.hemi_color.x;
        environment.hemi_color[1] = current.hemi_color.y;
        environment.hemi_color[2] = current.hemi_color.z;
    }
    return environment;
}

}

VulkanLevelRender::~VulkanLevelRender()
{
    Destroy();
}

void VulkanLevelRender::Create(SDL_Window* window, u32& width, u32& height,
    float& half_width, float& half_height)
{
    if (!window)
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan renderer requires the engine SDL window");
        return;
    }
    const u32 window_flags = SDL_GetWindowFlags(window);
    if (!(window_flags & SDL_WINDOW_VULKAN) || (window_flags & SDL_WINDOW_OPENGL))
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan renderer requires an SDL Vulkan window without OpenGL");
        return;
    }

    Destroy();
    auto device = external_game_device_ ? nullptr : std::make_unique<VulkanGameDevice>();
    auto* resources = external_game_device_ ? external_game_device_ : device.get();
    std::string error;
    if (!resources->initialize(window, {width, height}, error))
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan device creation failed: %s", error.c_str());
        return;
    }

    const VkExtent2D extent = resources->window().frame().extent();
    if (!extent.width || !extent.height)
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan swapchain has an empty extent");
        return;
    }
    width = extent.width;
    height = extent.height;
    half_width = static_cast<float>(width) * 0.5f;
    half_height = static_cast<float>(height) * 0.5f;

    owned_game_device_ = std::move(device);
    window_ = window;
    int drawable_width = 0, drawable_height = 0;
    SDL_Vulkan_GetDrawableSize(window, &drawable_width, &drawable_height);
    requested_drawable_ = drawable_width > 0 && drawable_height > 0 ?
        VkExtent2D{static_cast<uint32_t>(drawable_width), static_cast<uint32_t>(drawable_height)} : extent;
    bind_level_device(*resources);
    Msg("[renderer-vulkan] gameplay device initialized: Vulkan SDL window, %ux%u",
        width, height);
    context_state_.device_created();
    R_ASSERT2(device_resource_state_.device_created(),
        "Vulkan renderer device resource state was not empty at creation");
    reset_pending_ = false;
    reset_in_progress_ = false;
    app_suspended_ = false;
    recreate_surface_pending_ = false;
}

void VulkanLevelRender::Destroy()
{
    // Shutdown can interrupt a calculated frame that will never be submitted.
    frame_phase_.reset();
    // Game objects own the reference-counted light resources. Disable them
    // while the renderer is shutting down, but never delete through this
    // non-owning registry.
    for (VulkanLight* light : lights_)
        if (light) light->set_active(false);
    for (VulkanGlow* glow : glows_)
        if (glow) glow->set_active(false);
    if (device_resource_state_.can_create_factory_objects())
        OnDeviceDestroy(false);
    else
    {
        destroy_all_models();
        level_Unload();
    }
    if (game_device_)
        game_device_->use_scene_visibility(false);
    game_device_ = nullptr;
    device_ = VK_NULL_HANDLE;
    queue_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    memory_ = {};
    upload_ = {};
    textures_ = nullptr;
    pass_ = nullptr;
    wait_idle_ = nullptr;
    window_ = nullptr;
    requested_drawable_ = {};
    reset_pending_ = false;
    reset_in_progress_ = false;
    app_suspended_ = false;
    recreate_surface_pending_ = false;
    frame_phase_.reset();
    camera_state_.reset();
    context_state_.device_destroyed();
    R_ASSERT2(device_resource_state_.device_destroyed(),
        "Vulkan renderer device resources must be destroyed before the device");
    clear_target_pending_ = false;
    frame_clear_target_ = false;
    if (owned_game_device_)
    {
        owned_game_device_->destroy();
        owned_game_device_.reset();
    }
    else if (external_game_device_)
        external_game_device_->destroy();
}

void VulkanLevelRender::Reset(SDL_Window* window, u32& width, u32& height,
    float& half_width, float& half_height)
{
    R_ASSERT2(window && window == window_ && (SDL_GetWindowFlags(window) & SDL_WINDOW_VULKAN),
        "Vulkan renderer reset requires its original SDL Vulkan window");
    if (!game_device_)
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan renderer reset requires an initialized device");
        return;
    }

    if (!reset_in_progress_)
        reset_begin();

    int drawable_width = 0, drawable_height = 0;
    SDL_Vulkan_GetDrawableSize(window, &drawable_width, &drawable_height);
    if (drawable_width <= 0 || drawable_height <= 0)
    {
        reset_pending_ = true;
        reset_end();
        return;
    }

    std::string error;
    if (!game_device_->recreate_swapchain(
            {static_cast<uint32_t>(drawable_width), static_cast<uint32_t>(drawable_height)}, error,
            recreate_surface_pending_))
    {
        reset_end();
        xrDebug::Fatal(DEBUG_INFO, "Vulkan swapchain recreation failed: %s", error.c_str());
        return;
    }
    const VkExtent2D extent = game_device_->window().frame().extent();
    width = extent.width;
    height = extent.height;
    half_width = static_cast<float>(width) * 0.5f;
    half_height = static_cast<float>(height) * 0.5f;
    game_device_->clear_reset_required();
    requested_drawable_ = {static_cast<uint32_t>(drawable_width), static_cast<uint32_t>(drawable_height)};
    reset_pending_ = false;
    recreate_surface_pending_ = false;
    reset_end();
}

void VulkanLevelRender::reset_begin()
{
    R_ASSERT2(game_device_ && !reset_in_progress_ && !frame_phase_.active(),
        "Vulkan renderer reset_begin requires an idle initialized renderer");
    std::string error;
    if (!game_device_->prepare_for_reset(error))
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan renderer reset preparation failed: %s", error.c_str());
        return;
    }
    reset_in_progress_ = true;
}

void VulkanLevelRender::reset_end()
{
    R_ASSERT2(reset_in_progress_, "Vulkan renderer reset_end without reset_begin");
    reset_in_progress_ = false;
}

DeviceState VulkanLevelRender::GetDeviceState()
{
    if (!game_device_ || !window_ || !game_device_->window().device())
        return DeviceState::Lost;
    if (game_device_->device_lost())
        return DeviceState::Lost;
    if (app_suspended_)
        return DeviceState::Lost;

    int drawable_width = 0, drawable_height = 0;
    SDL_Vulkan_GetDrawableSize(window_, &drawable_width, &drawable_height);
    if (drawable_width <= 0 || drawable_height <= 0)
        return DeviceState::Lost;

    if (reset_pending_ || game_device_->reset_required() || game_device_->surface_lost() ||
        requested_drawable_.width != static_cast<uint32_t>(drawable_width) ||
        requested_drawable_.height != static_cast<uint32_t>(drawable_height))
        return DeviceState::NeedReset;
    return DeviceState::Normal;
}

void VulkanLevelRender::OnAppLifecycleChanged(bool active)
{
    if (!game_device_ || app_suspended_ == !active)
        return;

    app_suspended_ = !active;
    if (!active)
    {
        if (!game_device_->wait_idle())
            Msg("! Vulkan: device idle wait failed while suspending the Android surface");
        return;
    }

    // Android may destroy and recreate its native window surface while the
    // engine process and VkDevice remain alive. Recreate both the VkSurfaceKHR
    // and swapchain on the next engine reset, after SDL exposes a drawable.
    recreate_surface_pending_ = true;
    reset_pending_ = true;
}

void VulkanLevelRender::SetupStates()
{
    R_ASSERT2(game_device_ && game_device_->window().device(),
        "Vulkan renderer state setup requires a created Vulkan device");
    R_ASSERT2(device_resource_state_.setup_states(),
        "Vulkan renderer SetupStates was called before device creation");
    game_device_->ui().setup_states();
}

void VulkanLevelRender::OnCameraUpdated()
{
    camera_state_.on_camera_updated(Device.mFullTransform.m,
        Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z);
}

void VulkanLevelRender::SetCacheXform(Fmatrix& view, Fmatrix& project)
{
    camera_state_.set_cache_xform(view.m, project.m);
}

IRender::RenderContext VulkanLevelRender::GetCurrentContext() const
{
    static_assert(VulkanRenderContextState::NoContext == IRender::NoContext);
    static_assert(VulkanRenderContextState::PrimaryContext == IRender::PrimaryContext);
    static_assert(VulkanRenderContextState::HelperContext == IRender::HelperContext);
    return static_cast<IRender::RenderContext>(context_state_.current());
}

void VulkanLevelRender::MakeContextCurrent(IRender::RenderContext context)
{
    R_ASSERT2(context_state_.make_current(static_cast<int>(context)),
        "Vulkan renderer received an unknown render context");
}

Fmatrix VulkanLevelRender::current_view_projection() const
{
    Fmatrix result = Device.mFullTransform;
    if (camera_state_.has_view_projection())
        camera_state_.copy_view_projection(result.m);
    return result;
}

void VulkanLevelRender::OnDeviceCreate(pcstr)
{
    R_ASSERT2(game_device_ && game_device_->window().device(),
        "Vulkan renderer resources require a created Vulkan device");
    R_ASSERT2(device_resource_state_.setup_states(),
        "Vulkan renderer resources require SetupStates first");
    if (device_resource_state_.can_create_factory_objects())
        return;

    game_device_->ui().CreateUIGeom();
    if (auto* utils = dynamic_cast<VulkanDrawUtils*>(GEnv.DU))
        utils->OnDeviceCreate();
    R_ASSERT2(device_resource_state_.device_resources_created(),
        "Vulkan renderer device resources were created out of order");
}

void VulkanLevelRender::OnDeviceDestroy(bool)
{
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan UI teardown requires idle GPU frames");
    if (auto* utils = dynamic_cast<VulkanDrawUtils*>(GEnv.DU))
        utils->OnDeviceDestroy();
#ifdef DEBUG
    if (auto* debug = dynamic_cast<VulkanDebugRender*>(GEnv.DRender))
        debug->OnDeviceDestroy();
#endif
    if (game_device_ && textures_)
        for (const auto& [name, texture] : imgui_textures_)
            textures_->release_ui(texture.descriptor, game_device_->ui_pass());
    imgui_textures_.clear();
    screenshot_.reset();
    destroy_all_models();
    if (!game_device_)
    {
        level_Unload();
        return;
    }

    // level_Unload waits for submitted work before dropping level buffers.
    level_Unload();
    game_device_->ui().DestroyUIGeom();
    R_ASSERT2(device_resource_state_.device_resources_destroyed(),
        "Vulkan renderer device resources were not active during destruction");
}

std::unique_ptr<VulkanUIShader> VulkanLevelRender::create_ui_shader()
{
    R_ASSERT2(game_device_ && device_resource_state_.can_create_factory_objects(),
        "Vulkan UI shaders require OnDeviceCreate before level loading");
    return game_device_->create_ui_shader();
}

std::unique_ptr<VulkanFontRender> VulkanLevelRender::create_font_render()
{
    R_ASSERT2(game_device_ && device_resource_state_.can_create_factory_objects(),
        "Vulkan fonts require OnDeviceCreate before level loading");
    return game_device_->create_font_render();
}

void VulkanLevelRender::Begin()
{
    R_ASSERT2(game_device_ && !frame_phase_.active(),
        "Vulkan renderer Begin requires an initialized idle frame");
    if (assets_dirty_) OnAssetsChanged();
    game_device_->begin_frame();
    frame_draw_calls_ = frame_triangles_ = 0;
    if (pass_) pass_->reset_draw_statistics();
    const auto expired = std::remove_if(wallmarks_.begin(), wallmarks_.end(),
        [](const Wallmark& mark) { return Device.fTimeGlobal >= mark.expires; });
    if (expired != wallmarks_.end())
    {
        R_ASSERT2(game_device_->wait_idle(), "Vulkan wallmark retirement requires idle GPU frames");
        wallmarks_.erase(expired, wallmarks_.end());
    }
    R_ASSERT2(frame_phase_.begin(), "Vulkan renderer frame phase could not begin");
    frame_clear_target_ = clear_target_pending_;
    clear_target_pending_ = false;
}

void VulkanLevelRender::Calculate()
{
    R_ASSERT2(game_device_ && frame_phase_.active() &&
        !frame_phase_.world_calculated() && !frame_phase_.world_rendered(),
        "Vulkan renderer Calculate requires one active frame");

    // The game queries ROS from simulation, AI, rain and HUD code. Refresh its
    // environment and local-light values before the frame's render callbacks.
    const DeferredEnvironment environment = current_environment();
    std::vector<VulkanLightSnapshot> light_snapshots;
    light_snapshots.reserve(lights_.size());
    for (const VulkanLight* light : lights_)
        if (light) light_snapshots.push_back(light->snapshot());
    game_device_->queue_lights(light_snapshots);
    for (VulkanObjectSpecific* object : object_specifics_)
        if (object)
            object->update(environment.ambient_color, environment.hemi_color,
                environment.sun_color, light_snapshots);

    std::vector<uint32_t> visible_roots;
    IGameObject* view_entity = g_pGameLevel ? g_pGameLevel->CurrentViewEntity() : nullptr;
    const size_t camera_sector = view_entity ? view_entity->Sector() :
        IRender_Sector::INVALID_SECTOR_ID;
    const Fmatrix view_projection_matrix = current_view_projection();
    Fvector camera_position;
    if (camera_state_.has_camera_position())
        camera_position.set(camera_state_.camera_position()[0], camera_state_.camera_position()[1],
            camera_state_.camera_position()[2]);
    else
        camera_position.set(Device.vCameraPosition);
    if (!level_.visible_sector_roots(camera_sector, view_projection_matrix,
            camera_position, visible_roots))
        level_.all_level_roots(visible_roots);
    level_.prepare_details(camera_position);

    float view_projection[16];
    static_assert(sizeof(Fmatrix) == sizeof(view_projection));
    std::memcpy(view_projection, &view_projection_matrix, sizeof(view_projection));
    for (uint32_t root : visible_roots)
    {
        const LevelVisual* node = level_.visual_node(root);
        const Fvector center = visual_center(node);
        const float distance = distance_squared(center, camera_position);
        game_device_->queue_level_visual(root, view_projection, false,
            distance, nullptr, lod_for_distance(node ? node->bounds[9] : 0.f, distance));
    }

    // Engine renderables are submitted after this scene-calculation phase;
    // VulkanLevelRender::add_Visual stores their transforms for End().
    R_ASSERT2(frame_phase_.calculate(), "Vulkan renderer world calculation is out of order");
}

void VulkanLevelRender::Render()
{
    R_ASSERT2(game_device_ && frame_phase_.active() &&
        frame_phase_.world_calculated() && !frame_phase_.world_rendered(),
        "Vulkan renderer Render requires a calculated world in the active frame");
    // Geometry is recorded into the deferred G-buffer at End(), after all
    // engine callbacks have queued their transforms and before the UI pass.
    if (g_pGamePersistent && g_pGameLevel)
    {
        auto& environment = g_pGamePersistent->Environment();
        environment.RenderSky();
        environment.RenderClouds();
        if (environment.eff_Rain) environment.eff_Rain->Render();
        environment.RenderFlares();
        if (environment.eff_Thunderbolt) environment.eff_Thunderbolt->Render();
    }
    for (const VulkanGlow* glow : glows_)
    {
        if (!glow || !glow->active() || glow->radius() <= EPS_L) continue;
        const VkDescriptorSet texture = game_device_->glow_texture(glow->texture());
        if (!texture) continue;
        const auto& location = glow->position();
        Fvector center, right, up;
        center.set(location[0], location[1], location[2]);
        right.mul(Device.vCameraRight, glow->radius());
        up.mul(Device.vCameraTop, glow->radius());
        const auto& tint = glow->color();
        const u32 packed = color_rgba_f(tint[0], tint[1], tint[2], tint[3]);
        auto& ui = game_device_->ui();
        ui.SetTextureDescriptor(texture);
        ui.CacheSetXformWorld(Fidentity);
        ui.CacheSetCullMode(IUIRender::cmNONE);
        ui.StartPrimitive(4, IUIRender::ptTriStrip, IUIRender::pttLIT);
        const float uv[4][2]{{0, 0}, {0, 1}, {1, 0}, {1, 1}};
        const Fvector corners[4]{
            Fvector().add(center, right).sub(up), Fvector().add(center, right).add(up),
            Fvector().sub(center, right).sub(up), Fvector().sub(center, right).add(up)};
        for (size_t i = 0; i < 4; ++i)
            ui.PushPoint(corners[i].x, corners[i].y, corners[i].z, packed, uv[i][0], uv[i][1]);
        ui.FlushPrimitive();
    }
    R_ASSERT2(frame_phase_.render(), "Vulkan renderer world rendering is out of order");
}

void VulkanLevelRender::RenderMenu()
{
    R_ASSERT2(game_device_ && frame_phase_.active(),
        "Vulkan renderer RenderMenu requires an active frame");
    R_ASSERT2(frame_phase_.render_menu(), "Vulkan renderer menu callbacks are out of order");
    if (!g_pGamePersistent)
        return;

    // Match the engine's menu callback order. Both callbacks enqueue ordinary
    // UI batches; DeferredFrame records those batches after deferred lighting.
    g_pGamePersistent->OnRenderPPUI_main();
    g_pGamePersistent->OnRenderPPUI_PP();
}

void VulkanLevelRender::Clear()
{
    R_ASSERT2(game_device_, "Vulkan renderer Clear requires an initialized device");
    // GBufferTargets begins each frame with clear load operations for albedo,
    // normal and depth, so this request is already part of the deferred pass.
    // Match D3D's optional backbuffer clear using the engine's existing flag.
    if (psDeviceFlags.test(rsClearBB))
        ClearTarget();
}

void VulkanLevelRender::ClearTarget()
{
    R_ASSERT2(game_device_, "Vulkan renderer ClearTarget requires an initialized device");
    if (frame_phase_.active())
        frame_clear_target_ = true;
    else
        clear_target_pending_ = true;
}

void VulkanLevelRender::End()
{
    R_ASSERT2(game_device_ && frame_phase_.active(),
        "Vulkan renderer End requires a begun frame");
    R_ASSERT2(frame_phase_.can_end(),
        "Vulkan renderer End was reached before the calculated world was rendered");
    const bool render_world = frame_phase_.world_rendered();
    R_ASSERT2(frame_phase_.end(), "Vulkan renderer frame end is out of order");

    float mvp[16];
    static_assert(sizeof(Fmatrix) == sizeof(mvp));
    const Fmatrix view_projection = current_view_projection();
    std::memcpy(mvp, &view_projection, sizeof(mvp));

    if (render_world)
        for (auto& mark : wallmarks_)
        {
            Fmatrix transform = view_projection;
            if (mark.transform) transform.mul(view_projection, *mark.transform);
            float matrix[16];
            std::memcpy(matrix, &transform, sizeof(matrix));
            game_device_->queue_model(*mark.model, mark.skeleton, matrix, false,
                distance_squared(mark.center, Device.vCameraPosition), mark.model.get());
        }

    const DeferredEnvironment environment = current_environment();
    DeferredLight light = make_environment_deferred_light(environment);
    postprocess_.grade[0] = gamma_;
    postprocess_.grade[1] = brightness_;
    postprocess_.grade[2] = contrast_;
    postprocess_.noise[3] = Device.fTimeGlobal;
    game_device_->set_postprocess(postprocess_, color_map_a_, color_map_b_);
    FrameStatus status = FrameStatus::Presented;
    std::string error;
    const bool clear_target = frame_clear_target_;
    frame_clear_target_ = false;
    if (!game_device_->render(level_, mvp, light, status, error, render_world, clear_target))
    {
        Msg("! Vulkan frame recording/submission failed: %s", error.c_str());
        return;
    }
    if (status == FrameStatus::Presented) save_screenshot();
    if (pass_)
    {
        frame_draw_calls_ = pass_->draw_calls() + game_device_->last_ui_draw_calls();
        frame_triangles_ = pass_->triangles() + game_device_->last_ui_triangles();
    }
    if (status == FrameStatus::RecreateRequired)
        reset_pending_ = true;
}

void VulkanLevelRender::bind_level_device(VulkanGameDevice& resources)
{
    auto& window = resources.window();
    bind_level_device(window.device(), window.queue(), window.frame().command_pool(),
        window.physical().memory, resources.buffer_upload(), resources.textures(),
        resources.deferred(), resources.device_wait_idle_proc());
    game_device_ = &resources;
    resources.use_scene_visibility(true);
}

void VulkanLevelRender::bind_level_device(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
    GameTextureFactory& textures, DeferredPass& pass, PFN_vkDeviceWaitIdle wait_idle)
{
    level_Unload();
    if (game_device_)
        game_device_->use_scene_visibility(false);
    game_device_ = nullptr;
    device_ = device;
    queue_ = queue;
    pool_ = pool;
    memory_ = memory;
    upload_ = upload;
    textures_ = &textures;
    pass_ = &pass;
    wait_idle_ = wait_idle;
}

void VulkanLevelRender::level_Load(IReader* reader)
{
    R_ASSERT2(!frame_phase_.active(), "Vulkan level replacement requires an idle frame");
    R_ASSERT2(reader && device_ && queue_ && pool_ && textures_ && pass_ && wait_idle_ &&
        device_resource_state_.can_load_level(),
        "Vulkan level load requires OnDeviceCreate and an initialized gameplay device");
    // Replacing a level must wait for every submitted draw before its buffers
    // and visual identities are released. Uploads use the same graphics queue.
    R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan queue did not become idle before level load");
    if (game_device_) game_device_->discard_scene_draws();
    wallmarks_.clear();
    models_Clear(true);
    model_bases_.clear();
    model_gpu_cache_.clear();
    std::string error;
    if (!level_.load(*reader, device_, queue_, pool_, memory_, upload_, *textures_, *pass_, error))
        xrDebug::Fatal(DEBUG_INFO, "Vulkan level load failed: %s", error.c_str());
}

void VulkanLevelRender::level_Unload()
{
    R_ASSERT2(!frame_phase_.active(), "Vulkan level unload requires an idle frame");
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan queue did not become idle before level unload");
    if (game_device_)
    {
        game_device_->discard_scene_draws();
        game_device_->release_level_glows();
    }
    wallmarks_.clear();
    models_Clear(true);
    model_bases_.clear();
    model_gpu_cache_.clear();
    level_.destroy();
}

IRenderVisual* VulkanLevelRender::getVisual(int index)
{
    return index >= 0 ? level_.get_visual(static_cast<size_t>(index)) : nullptr;
}

HRESULT VulkanLevelRender::shader_compile(pcstr name, IReader* source, pcstr entry, pcstr target, u32 flags, void*& result)
{
    result = nullptr;
    // The Android runtime has no DXC. Host builds put compiled variants in
    // the mounted game shader directory; the original IReader is retained by
    // the resource manager, while this path creates an actual Vulkan module.
    if (!device_ || !game_device_ || !source || !source->length() || !name || !*name || !entry || xr_strcmp(entry, "main") || !target ||
        !((target[0] == 'v' && target[1] == 's') || (target[0] == 'p' && target[1] == 's')))
    {
        Msg("! [renderer-vulkan] shader_compile: invalid device, entry or stage for '%s'", name ? name : "<unnamed>");
        return E_FAIL;
    }
    std::string error;
    GameShaderResource *resource = nullptr;
    const auto stage = target[0] == 'v' ? GameShaderStage::Vertex : GameShaderStage::Pixel;
    const std::string relative = std::string(getShaderPath()) + name;
    const bool compiled = game_device_->shader_resources().shader(relative, stage, entry, flags, resource, error);
    if (!compiled)
    {
        Msg("! [renderer-vulkan] shader_compile: '%s': %s", name, error.c_str());
        return E_FAIL;
    }
    result = resource;
    return S_OK;
}

IRenderVisual* VulkanLevelRender::model_Create(pcstr name, IReader* data)
{
    R_ASSERT2(device_ && queue_ && pool_ && textures_ && pass_ && wait_idle_ && device_resource_state_.can_create_factory_objects(),
        "Vulkan model creation requires an initialized gameplay device");
    const std::string cache_name = model_cache_name(name);
    if (!cache_name.empty())
    {
        auto reused = model_pool_.find(cache_name);
        if (reused != model_pool_.end())
        {
            auto instance = std::move(reused->second);
            model_pool_.erase(reused);
            instance->reset_instance_state();
            IRenderVisual* visual = instance.get();
            models_.emplace(visual, std::move(instance));
            return visual;
        }
    }
    std::string error;
    std::unique_ptr<VulkanModelVisual> model;
    if (cache_name.empty())
        model = load_model_base(name, data, "", error);
    else if (VulkanModelVisual* base = get_model_base(name, data, cache_name, error))
        model = std::make_unique<VulkanModelVisual>(*base);
    if (!model)
    {
        Msg("! [renderer-vulkan] OGF model '%s': %s", name ? name : "<reader>", error.c_str());
        return nullptr;
    }
    IRenderVisual* visual = model.get();
    models_.emplace(visual, std::move(model));
    return visual;
}

IRenderVisual* VulkanLevelRender::model_CreateParticles(pcstr name)
{
    R_ASSERT2(device_ && textures_ && pass_ && wait_idle_ &&
        device_resource_state_.can_create_factory_objects(),
        "Vulkan particles require an initialized gameplay device");
    if (!name || !*name) return nullptr;
    if (!particle_catalog_)
    {
        string_path path;
        FS.update_path(path, _game_data_, "particles.xr");
        IReader* source = FS.r_open(path);
        if (!source)
        {
            Msg("! [renderer-vulkan] particle library not found: %s", path);
            return nullptr;
        }
        auto catalog = std::make_shared<ParticleCatalog>();
        std::string error;
        const auto normalized = particle_library_bytes(*source);
        const bool parsed = parse_particle_catalog(
            {normalized.data(), normalized.size()}, *catalog, error);
        FS.r_close(source);
        if (!parsed)
        {
            Msg("! [renderer-vulkan] particle library: %s", error.c_str());
            return nullptr;
        }
        particle_catalog_ = std::move(catalog);
    }
    std::string error;
    if (const auto* def = particle_catalog_->effect(name))
    {
        auto visual = std::make_unique<VulkanParticleEffect>(particle_catalog_, *def);
        if (!visual->initialize(device_, memory_, upload_.buffer, *textures_, *pass_, error))
        {
            Msg("! [renderer-vulkan] particle effect '%s': %s", name, error.c_str());
            return nullptr;
        }
        IRenderVisual* result = visual.get();
        particle_effects_.emplace(result, std::move(visual));
        return result;
    }
    if (const auto* def = particle_catalog_->group(name))
    {
        auto visual = std::make_unique<VulkanParticleGroup>(particle_catalog_, *def);
        if (!visual->initialize(device_, memory_, upload_.buffer, *textures_, *pass_, error))
        {
            Msg("! [renderer-vulkan] particle group '%s': %s", name, error.c_str());
            return nullptr;
        }
        IRenderVisual* result = visual.get();
        particle_groups_.emplace(result, std::move(visual));
        return result;
    }
    Msg("! [renderer-vulkan] particle effect or group not found: %s", name);
    return nullptr;
}

std::unique_ptr<VulkanModelVisual> VulkanLevelRender::load_model_base(pcstr name, IReader *data, const std::string &cache_name, std::string &error,
                                                                      bool skeletal_child)
{
    VisualRecord record;
    const bool parsed = data ? load_engine_model_reader(*data, name, record, error) : load_engine_model_visual(name, record, error);
    if (!parsed)
        return nullptr;
    auto result = create_model_tree(record, "", cache_name, error, skeletal_child);
    if (!result)
        error = std::string(name && *name ? name : "<reader>.ogf") + " / type=" + std::to_string(record.type) +
                " shader_id=" + std::to_string(record.shader_id) + " create: " + error;
    return result;
}

VulkanModelVisual *VulkanLevelRender::get_model_base(pcstr name, IReader *data, const std::string &cache_name, std::string &error, bool skeletal_child)
{
    auto found = model_bases_.find(cache_name);
    if (found != model_bases_.end())
        return found->second.get();
    auto base = load_model_base(name, data, cache_name, error, skeletal_child);
    if (!base)
        return nullptr;
    VulkanModelVisual *visual = base.get();
    model_bases_.emplace(cache_name, std::move(base));
    return visual;
}

std::unique_ptr<VulkanModelVisual> VulkanLevelRender::create_model_tree(const VisualRecord& record,
    const std::string& inherited_texture, const std::string& cache_name, std::string& error,
    bool skeletal_child)
{
    const bool skeleton = record.type == 3 || record.type == 10;
    const bool skinned = record.type == 4 || record.type == 5;
    const bool tree = record.type == 7 || record.type == 11;
    if (record.type != 0 && record.type != 1 && record.type != 2 && record.type != 6 && !tree && !skeleton &&
        !(skeletal_child && skinned))
    {
        error = "unsupported OGF model type " + std::to_string(record.type);
        return nullptr;
    }
    if ((record.type == 0 || record.type == 2 || tree) &&
        (!record.embedded_children.empty() || !record.linked_children.empty()))
    {
        error = "OGF mesh unexpectedly contains child references";
        return nullptr;
    }
    if (skinned && (!record.embedded_children.empty() || !record.linked_children.empty()))
    {
        error = "OGF skinned child unexpectedly contains nested visuals";
        return nullptr;
    }
    if ((record.type == 1 || record.type == 6) &&
        (record.embedded_children.empty() == record.linked_children.empty()))
    {
        error = "OGF hierarchy needs exactly one nonempty child table";
        return nullptr;
    }
    std::shared_ptr<GpuModel> gpu;
    if (record.type == 0 || record.type == 2 || tree || skinned)
    {
        if (!cache_name.empty()) gpu = model_gpu_cache_[cache_name].lock();
        if (!gpu)
        {
            gpu = std::make_shared<GpuModel>();
            if (!gpu->load_record(record, inherited_texture, device_, queue_, pool_, memory_,
                    upload_, *textures_, *pass_, error)) return nullptr;
            if (!cache_name.empty()) model_gpu_cache_[cache_name] = gpu;
        }
    }
    else if (!record.linked_children.empty())
    {
        for (uint32_t child : record.linked_children)
            if (!level_.get_visual(child))
            {
                error = "OGF hierarchy refers to a missing level visual";
                return nullptr;
            }
    }
    auto model = std::make_unique<VulkanModelVisual>(record, cache_name, std::move(gpu),
        record.linked_children.empty() ? nullptr : &level_);
    if (record.type == 6)
    {
        IReader source(const_cast<uint8_t*>(record.source.data()), record.source.size());
        IReader* raw = source.open_chunk(OGF_LODDEF2);
        LodFacets facets;
        if (!raw || !decode_lod_facets({static_cast<const uint8_t*>(raw->pointer()),
                raw->length()}, 0, facets, error))
        {
            if (raw) raw->close();
            if (error.empty()) error = "OGF LOD has no valid facets";
            return nullptr;
        }
        raw->close();
        std::array<std::shared_ptr<GpuModel>, 8> lod_gpu;
        const auto& texture = !record.texture.empty() ? record.texture :
            !record.embedded_children.empty() ? record.embedded_children.front().texture : inherited_texture;
        for (size_t face = 0; face < lod_gpu.size(); ++face)
        {
            ModelGeometry geometry;
            geometry.type = 0;
            geometry.texture = texture;
            geometry.mode = classify_surface_material(record.shader, texture);
            geometry.indices = std::move(facets.meshes[face].indices);
            for (const LevelVertex& vertex : facets.meshes[face].vertices)
            {
                ModelVertex converted;
                std::copy_n(vertex.position, 3, converted.position);
                std::copy_n(vertex.normal, 3, converted.normal);
                std::copy_n(vertex.uv, 2, converted.uv);
                geometry.vertices.push_back(converted);
            }
            lod_gpu[face] = std::make_shared<GpuModel>();
            if (!lod_gpu[face]->load_decoded(std::move(geometry), device_, queue_, pool_,
                    memory_, upload_, *textures_, *pass_, error)) return nullptr;
        }
        model->set_lod(std::move(lod_gpu), facets.normals);
    }
    if (skeleton)
    {
        if (record.embedded_children.empty() || !record.linked_children.empty())
        {
            error = "Vulkan skeleton needs embedded child meshes";
            return nullptr;
        }
        SkeletonBones bones;
        if (!parse_skeleton_bones({record.source.data(), record.source.size()}, bones, error))
            return nullptr;
        auto instance = VulkanKinematics::create(bones, model.get(), error);
        if (!instance) return nullptr;
        if (record.type == 3)
        {
            std::vector<std::string> bone_names;
            bone_names.reserve(bones.bones.size());
            for (const auto& bone : bones.bones)
                bone_names.push_back(bone.name);
            std::vector<MotionSlot> motions;
            if (!parse_skeleton_motions({ record.source.data(), record.source.size() }, bone_names, cache_name, load_engine_motion_files, motions, error))
                return nullptr;
            instance->set_motions(std::move(motions));
        }
        model->set_skeleton(std::move(instance));
    }
    const std::string texture = record.texture.empty() ? inherited_texture : record.texture;
    for (const auto& child : record.embedded_children)
    {
        auto instance = create_model_tree(child, texture, "", error, skeletal_child || skeleton);
        if (!instance) return nullptr;
        model->add_child(std::move(instance));
    }
    if (skeleton)
    {
        ModelGeometry geometry;
        if (!decode_model_geometry(record, geometry, error) || !model->skeleton()->attach_geometry(geometry, error))
            return nullptr;
    }
    return model;
}

IRenderVisual* VulkanLevelRender::model_CreateChild(pcstr name, IReader* data)
{
    R_ASSERT2(device_ && queue_ && pool_ && textures_ && pass_ && wait_idle_ && device_resource_state_.can_create_factory_objects(),
              "Vulkan child model creation requires an initialized gameplay device");
    const std::string filename = model_cache_name(name);
    const std::string cache_name = filename.empty() ? "" : "child:" + filename;
    std::string error;
    std::unique_ptr<VulkanModelVisual> child;
    if (cache_name.empty())
        child = load_model_base(name, data, "", error, true);
    else if (VulkanModelVisual *base = get_model_base(name, data, cache_name, error, true))
    {
        child = std::make_unique<VulkanModelVisual>(*base);
        // Children belong to their parent; deleting one must not put it in
        // the pool of standalone model instances.
        child->set_cache_name("");
    }
    if (!child)
    {
        Msg("! [renderer-vulkan] OGF child '%s': %s", name ? name : "<reader>", error.c_str());
        return nullptr;
    }
    IRenderVisual* visual = child.get();
    models_.emplace(visual, std::move(child));
    return visual;
}

IRenderVisual* VulkanLevelRender::model_Duplicate(IRenderVisual* visual)
{
    auto found = models_.find(visual);
    const VulkanModelVisual* source = found == models_.end() ? nullptr : found->second.get();
    if (!source)
        for (const auto& entry : models_)
            if ((source = entry.second->find(visual))) break;
    R_ASSERT2(source, "Vulkan duplication requires a live model instance");
    auto copy = std::make_unique<VulkanModelVisual>(*source);
    IRenderVisual* result = copy.get();
    models_.emplace(result, std::move(copy));
    return result;
}

void VulkanLevelRender::model_Delete(IRenderVisual*& visual, bool discard)
{
    if (!visual)
        return;
    if (auto particle = particle_effects_.find(visual); particle != particle_effects_.end())
    {
        if (game_device_) game_device_->discard_model_draws(visual);
        if (device_ && wait_idle_)
            R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan particle deletion requires idle GPU frames");
        particle_effects_.erase(particle);
        visual = nullptr;
        return;
    }
    if (auto particle = particle_groups_.find(visual); particle != particle_groups_.end())
    {
        if (game_device_) game_device_->discard_model_draws(visual);
        if (device_ && wait_idle_)
            R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan particle deletion requires idle GPU frames");
        particle_groups_.erase(particle);
        visual = nullptr;
        return;
    }
    auto found = models_.find(visual);
    R_ASSERT2(found != models_.end(), "Vulkan model deletion received an unknown visual");
    if (game_device_)
        game_device_->discard_model_draws(visual);
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan model deletion requires idle GPU frames");
    std::vector<IKinematics*> doomed_skeletons;
    std::function<void(const VulkanModelVisual&)> collect_skeletons =
        [&](const VulkanModelVisual& node)
    {
        if (node.skeleton()) doomed_skeletons.push_back(node.skeleton());
        for (const auto& child : node.children()) collect_skeletons(*child);
    };
    collect_skeletons(*found->second);
    wallmarks_.erase(std::remove_if(wallmarks_.begin(), wallmarks_.end(),
        [&](const Wallmark& mark)
    {
        return std::find(doomed_skeletons.begin(), doomed_skeletons.end(), mark.skeleton) !=
            doomed_skeletons.end();
    }), wallmarks_.end());
    found->second->release_pose_buffers();
    if (!discard && !found->second->cache_name().empty())
        model_pool_.emplace(found->second->cache_name(), std::move(found->second));
    models_.erase(found);
    visual = nullptr;
}

void VulkanLevelRender::models_Clear(bool)
{
    // The engine calls models_Clear(false) while live objects still hold
    // visuals. As in CModelPool::ClearPool, only unused instances are evicted.
    if (model_pool_.empty())
        return;
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan model clear requires idle GPU frames");
    model_pool_.clear();
    for (auto it = model_gpu_cache_.begin(); it != model_gpu_cache_.end();)
        if (it->second.expired())
            it = model_gpu_cache_.erase(it);
        else
            ++it;
}

void VulkanLevelRender::destroy_all_models()
{
    if (models_.empty() && model_pool_.empty() && model_bases_.empty() &&
        particle_effects_.empty() && particle_groups_.empty())
    {
        particle_catalog_.reset();
        return;
    }
    if (game_device_)
        game_device_->discard_scene_draws();
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan model teardown requires idle GPU frames");
    models_.clear();
    particle_effects_.clear();
    particle_groups_.clear();
    particle_catalog_.reset();
    model_pool_.clear();
    model_bases_.clear();
    model_gpu_cache_.clear();
}

void VulkanLevelRender::add_Visual(u32, IRenderable* root, IRenderVisual* visual, Fmatrix& world)
{
    R_ASSERT2(game_device_, "Vulkan level visuals require a bound gameplay device");
    if (particle_effects_.count(visual) || particle_groups_.count(visual))
    {
        const Fmatrix view_projection = current_view_projection();
        Fvector camera_position;
        if (camera_state_.has_camera_position())
            camera_position.set(camera_state_.camera_position()[0], camera_state_.camera_position()[1],
                camera_state_.camera_position()[2]);
        else camera_position.set(Device.vCameraPosition);
        const Fvector center = visual->getVisData().sphere.P;
        const bool hud = root && root->renderable_HUD();
        float transform[16];
        std::memcpy(transform, &view_projection, sizeof(transform));
        game_device_->queue_particle(visual, transform, Device.vCameraRight, Device.vCameraTop,
            hud, distance_squared(center, camera_position));
        return;
    }
    const int visual_index = level_.find_visual_index(visual);
    const VulkanModelVisual* model = nullptr;
    const void* model_owner = nullptr;
    for (const auto& entry : models_)
        if ((model = entry.second->find(visual)))
        {
            model_owner = entry.first;
            break;
        }
    R_ASSERT2(visual_index >= 0 || model, "Vulkan scene submission received a visual outside this renderer");
    const Fmatrix view_projection = current_view_projection();
    Fmatrix mvp;
    mvp.mul(view_projection, world);
    Fvector center = model ? model->visibility().sphere.P :
        visual_center(level_.visual_node(static_cast<size_t>(visual_index)));
    Fvector world_center;
    world.transform_tiny(world_center, center);
    center = world_center;
    static_assert(sizeof(Fmatrix) == 16 * sizeof(float));
    float transform[16];
    std::memcpy(transform, &mvp, sizeof(transform));
    const bool hud = root && root->renderable_HUD();
    Fvector camera_position;
    if (camera_state_.has_camera_position())
        camera_position.set(camera_state_.camera_position()[0], camera_state_.camera_position()[1],
            camera_state_.camera_position()[2]);
    else
        camera_position.set(Device.vCameraPosition);
    const float sort_distance = distance_squared(center, camera_position);
    const float scale = std::max({world.i.magnitude(), world.j.magnitude(), world.k.magnitude()});
    const float radius = (model ? model->visibility().sphere.R :
        level_.visual_node(static_cast<size_t>(visual_index))->bounds[9]) * scale;
    const float lod = lod_for_distance(radius, sort_distance);
    if (model)
    {
        std::function<void(const VulkanModelVisual&, IKinematics*)> submit =
            [&](const VulkanModelVisual& node, IKinematics* skeleton)
        {
            if (node.skeleton()) skeleton = node.skeleton();
            if (node.has_lod() && lod < .33f)
            {
                std::array<std::array<float, 3>, 8> normals = node.lod_normals();
                for (auto& normal : normals)
                {
                    Fvector direction, transformed;
                    direction.set(normal[0], normal[1], normal[2]);
                    world.transform_dir(transformed, direction);
                    normal = {transformed.x, transformed.y, transformed.z};
                }
                Fvector center;
                world.transform_tiny(center, node.visibility().sphere.P);
                const uint8_t facet = select_lod_facet(normals,
                    {center.x - camera_position.x, center.y - camera_position.y,
                        center.z - camera_position.z});
                game_device_->queue_model(node.lod_gpu(facet), skeleton,
                    transform, hud, sort_distance, model_owner, lod);
                return;
            }
            if (node.has_gpu())
                game_device_->queue_model(node.gpu(), skeleton,
                    transform, hud, sort_distance, model_owner, lod);
            for (const auto& child : node.children()) submit(*child, skeleton);
            if (!node.linked().empty())
            {
                R_ASSERT2(node.linked_valid(), "Vulkan model has level references from an unloaded level");
                for (uint32_t index : node.linked())
                    game_device_->queue_level_visual(index, transform, hud, sort_distance, model_owner, lod);
            }
        };
        submit(*model, nullptr);
    }
    else
        game_device_->queue_level_visual(static_cast<uint32_t>(visual_index), transform, hud,
            sort_distance, nullptr, lod);
}

IRender_ObjectSpecific* VulkanLevelRender::ros_create(IRenderable* parent)
{
    R_ASSERT2(parent, "Vulkan object lighting requires a renderable parent");
    auto* object = xr_new<VulkanObjectSpecific>(parent);
    object_specifics_.push_back(object);
    return object;
}

void VulkanLevelRender::create_wallmark(ModelGeometry&& geometry, const Fvector& center,
    IKinematics* skeleton, const Fmatrix* transform)
{
    if (!device_ || !game_device_) return;
    auto model = std::make_unique<GpuModel>();
    std::string error;
    if (!model->load_decoded(std::move(geometry), device_, queue_, pool_, memory_,
            upload_, *textures_, *pass_, error))
    {
        Msg("! [renderer-vulkan] wallmark: %s", error.c_str());
        return;
    }
    wallmarks_.push_back({std::move(model), skeleton, transform, center,
        Device.fTimeGlobal + 50.f});
}

void VulkanLevelRender::add_StaticWallmark(const wm_shader& shader, const Fvector& point,
    float size, CDB::TRI* triangle, Fvector* vertices)
{
    if (!triangle || !vertices || triangle->suppress_wm) return;
    const auto* material = dynamic_cast<const VulkanUIShader*>(&*shader);
    R_ASSERT2(material, "Vulkan wallmark requires a Vulkan material");
    Fvector corners[3] = {vertices[triangle->verts[0]],
        vertices[triangle->verts[1]], vertices[triangle->verts[2]]};
    ModelGeometry geometry;
    if (wallmark_geometry(corners, point, size, material->texture_name(), geometry))
        create_wallmark(std::move(geometry), point);
}

void VulkanLevelRender::add_StaticWallmark(IWallMarkArray* array, const Fvector& point,
    float size, CDB::TRI* triangle, Fvector* vertices)
{
    auto* materials = dynamic_cast<VulkanWallMarkArray*>(array);
    R_ASSERT2(materials, "Vulkan wallmark requires a Vulkan material array");
    if (!triangle || !vertices || triangle->suppress_wm) return;
    const std::string* texture = materials->select_texture();
    if (!texture) return;
    Fvector corners[3] = {vertices[triangle->verts[0]],
        vertices[triangle->verts[1]], vertices[triangle->verts[2]]};
    ModelGeometry geometry;
    if (wallmark_geometry(corners, point, size, *texture, geometry))
        create_wallmark(std::move(geometry), point);
}

void VulkanLevelRender::add_SkeletonWallmark(const Fmatrix* transform, IKinematics* skeleton,
    IWallMarkArray* array, const Fvector& start, const Fvector& direction, float size)
{
    auto* materials = dynamic_cast<VulkanWallMarkArray*>(array);
    R_ASSERT2(materials && transform && skeleton,
        "Vulkan skeletal wallmark requires a live skeleton and material array");
    const std::string* texture = materials->select_texture();
    if (!texture || !(size > EPS_L)) return;
    skeleton->CalculateBones();
    IKinematics::pick_result picked{};
    u16 bone = BI_NONE;
    float nearest = size * 3.f;
    for (u16 id = 0; id < skeleton->LL_BoneCount(); ++id)
    {
        IKinematics::pick_result hit{};
        if (skeleton->PickBone(*transform, hit, nearest, start, direction, id))
        {
            nearest = hit.dist;
            picked = hit;
            bone = id;
        }
    }
    if (bone == BI_NONE) return;
    Fvector contact;
    contact.mad(start, direction, nearest);
    ModelGeometry geometry;
    if (!wallmark_geometry(picked.tri, contact, size, *texture, geometry)) return;
    // Store the decal in bind space; the model's ordinary skinning path then
    // carries it with the hit bone as its pose changes.
    Fmatrix inverse_world, inverse_bone;
    inverse_world.invert(*transform);
    inverse_bone.invert(skeleton->LL_GetBoneInstance(bone).mRenderTransform);
    for (auto& vertex : geometry.vertices)
    {
        Fvector world, local, bind;
        world.set(vertex.position[0], vertex.position[1], vertex.position[2]);
        inverse_world.transform_tiny(local, world);
        inverse_bone.transform_tiny(bind, local);
        vertex.position[0] = bind.x;
        vertex.position[1] = bind.y;
        vertex.position[2] = bind.z;
        vertex.bones[0] = bone;
    }
    geometry.type = 4;
    create_wallmark(std::move(geometry), contact, skeleton, transform);
}

void VulkanLevelRender::clear_static_wallmarks()
{
    if (game_device_)
        R_ASSERT2(game_device_->wait_idle(), "Vulkan wallmark clear requires idle GPU frames");
    wallmarks_.erase(std::remove_if(wallmarks_.begin(), wallmarks_.end(),
        [](const Wallmark& mark) { return !mark.skeleton; }), wallmarks_.end());
}

void VulkanLevelRender::ros_destroy(IRender_ObjectSpecific*& object)
{
    if (!object)
        return;
    auto* vulkan_object = dynamic_cast<VulkanObjectSpecific*>(object);
    R_ASSERT2(vulkan_object, "Vulkan renderer received foreign object-specific data");
    if (!vulkan_object)
        return;
    object_specifics_.erase(std::remove(object_specifics_.begin(), object_specifics_.end(), vulkan_object),
        object_specifics_.end());
    xr_delete(vulkan_object);
    object = nullptr;
}

IRender_Light* VulkanLevelRender::light_create()
{
    auto* light = xr_new<VulkanLight>();
    lights_.push_back(light);
    return light;
}

void VulkanLevelRender::light_destroy(IRender_Light* light)
{
    // Called by IRender_Light's base destructor after the derived subobject is
    // gone: compare identities only and do not dereference the pointer.
    lights_.erase(std::remove_if(lights_.begin(), lights_.end(),
        [light](const VulkanLight* candidate) { return static_cast<const IRender_Light*>(candidate) == light; }),
        lights_.end());
}

IRender_Glow* VulkanLevelRender::glow_create()
{
    auto* glow = xr_new<VulkanGlow>();
    glows_.push_back(glow);
    return glow;
}

void VulkanLevelRender::glow_destroy(IRender_Glow* glow)
{
    // Like lights, glows are xr_resources and call back from their base
    // destructor. Avoid accessing the object during that callback.
    glows_.erase(std::remove_if(glows_.begin(), glows_.end(),
        [glow](const VulkanGlow* candidate) { return static_cast<const IRender_Glow*>(candidate) == glow; }),
        glows_.end());
}
}
