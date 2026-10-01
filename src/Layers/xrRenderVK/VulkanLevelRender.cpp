#include "xrEngine/stdafx.h"
#include "VulkanLevelRender.h"
#include "VulkanGameDevice.h"
#include "VulkanVisual.h"
#include "VulkanModelVisual.h"
#include "xrEngine/device.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_object.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace xray::render::vulkan
{
namespace
{
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

bool has_spirv_entry(const void* bytes, size_t size, uint32_t stage, const char* entry)
{
    if (!bytes || size < 20 || size % 4) return false;
    std::vector<uint32_t> words(size / 4);
    std::memcpy(words.data(), bytes, size);
    if (words[0] != 0x07230203u) return false;
    for (size_t offset = 5; offset < words.size();)
    {
        const uint32_t count = words[offset] >> 16;
        const uint32_t opcode = words[offset] & 0xffffu;
        if (!count || count > words.size() - offset) return false;
        if (opcode == 15 && count >= 4 && words[offset + 1] == stage)
        {
            const char* name = reinterpret_cast<const char*>(words.data() + offset + 3);
            const size_t capacity = (count - 3) * sizeof(uint32_t);
            const void* terminator = std::memchr(name, 0, capacity);
            if (terminator && xr_strcmp(name, entry) == 0) return true;
        }
        offset += count;
    }
    return false;
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
    if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_VULKAN))
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan renderer requires SDL_WINDOW_VULKAN on the engine window");
        return;
    }

    Destroy();
    auto device = std::make_unique<VulkanGameDevice>();
    std::string error;
    if (!device->initialize(window, {width, height}, error))
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan device creation failed: %s", error.c_str());
        return;
    }

    const VkExtent2D extent = device->window().frame().extent();
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
    bind_level_device(*owned_game_device_);
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
    compiled_shaders_.clear();
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
    R_ASSERT2(device_resource_state_.device_resources_created(),
        "Vulkan renderer device resources were created out of order");
}

void VulkanLevelRender::OnDeviceDestroy(bool)
{
    destroy_all_models();
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan shader teardown requires idle GPU frames");
    compiled_shaders_.clear();
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
    game_device_->begin_frame();
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

    float view_projection[16];
    static_assert(sizeof(Fmatrix) == sizeof(view_projection));
    std::memcpy(view_projection, &view_projection_matrix, sizeof(view_projection));
    for (uint32_t root : visible_roots)
    {
        const Fvector center = visual_center(level_.visual_node(root));
        game_device_->queue_level_visual(root, view_projection, false,
            distance_squared(center, camera_position));
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

    const DeferredEnvironment environment = current_environment();
    const DeferredLight light = make_environment_deferred_light(environment);
    FrameStatus status = FrameStatus::Presented;
    std::string error;
    const bool clear_target = frame_clear_target_;
    frame_clear_target_ = false;
    if (!game_device_->render(level_, mvp, light, status, error, render_world, clear_target))
    {
        Msg("! Vulkan frame recording/submission failed: %s", error.c_str());
        return;
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
    models_Clear(true);
    std::string error;
    if (!level_.load(*reader, device_, queue_, pool_, memory_, upload_, *textures_, *pass_, error))
        xrDebug::Fatal(DEBUG_INFO, "Vulkan level load failed: %s", error.c_str());
}

void VulkanLevelRender::level_Unload()
{
    R_ASSERT2(!frame_phase_.active(), "Vulkan level unload requires an idle frame");
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan queue did not become idle before level unload");
    if (game_device_) game_device_->discard_scene_draws();
    models_Clear(true);
    level_.destroy();
}

IRenderVisual* VulkanLevelRender::getVisual(int index)
{
    return index >= 0 ? level_.get_visual(static_cast<size_t>(index)) : nullptr;
}

HRESULT VulkanLevelRender::shader_compile(pcstr name, IReader* source, pcstr entry,
    pcstr target, u32 flags, void*& result)
{
    result = nullptr;
    // The Android runtime has no DXC. Host builds put compiled variants in
    // the mounted game shader directory; the original IReader is retained by
    // the resource manager, while this path creates an actual Vulkan module.
    (void)flags;
    if (!device_ || !game_device_ || !source || !source->length() || !name || !*name || !entry ||
        xr_strcmp(entry, "main") || !target ||
        !((target[0] == 'v' && target[1] == 's') ||
          (target[0] == 'p' && target[1] == 's')))
    {
        Msg("! [renderer-vulkan] shader_compile: invalid device, entry or stage for '%s'",
            name ? name : "<unnamed>");
        return E_FAIL;
    }
    const char stage[] = {target[0], target[1], '\0'};
    string_path relative, path;
    strconcat(sizeof(relative), relative, getShaderPath(), name, ".", stage, ".spv");
    FS.update_path(path, "$game_shaders$", relative);
    IReader* binary = FS.r_open(path);
    if (!binary)
    {
        Msg("! [renderer-vulkan] shader_compile: missing precompiled SPIR-V '%s'", path);
        return E_FAIL;
    }
    const uint32_t execution_model = stage[0] == 'v' ? 0u : 4u;
    if (!has_spirv_entry(binary->pointer(), binary->length(), execution_model, entry))
    {
        Msg("! [renderer-vulkan] shader_compile: '%s' has no %s entry '%s'",
            path, stage, entry);
        FS.r_close(binary);
        return E_FAIL;
    }
    const VkDevice device = game_device_->window().device();
    const auto get = game_device_->window().device_proc();
    const ShaderModuleDispatch dispatch{
        reinterpret_cast<PFN_vkCreateShaderModule>(get(device, "vkCreateShaderModule")),
        reinterpret_cast<PFN_vkDestroyShaderModule>(get(device, "vkDestroyShaderModule"))};
    auto module = std::make_unique<ShaderModule>();
    std::string error;
    const bool compiled = module->initialize_bytes(device, dispatch,
        binary->pointer(), binary->length(), error);
    FS.r_close(binary);
    if (!compiled)
    {
        Msg("! [renderer-vulkan] shader_compile: '%s': %s", path, error.c_str());
        return E_FAIL;
    }
    result = module.get();
    compiled_shaders_.push_back(std::move(module));
    return S_OK;
}

IRenderVisual* VulkanLevelRender::model_Create(pcstr name, IReader* data)
{
    R_ASSERT2(device_ && queue_ && pool_ && textures_ && pass_ && wait_idle_ &&
        device_resource_state_.can_create_factory_objects(),
        "Vulkan model creation requires an initialized gameplay device");
    if (!data && name && *name)
    {
        auto reused = model_pool_.find(name);
        if (reused != model_pool_.end())
        {
            auto instance = std::move(reused->second);
            model_pool_.erase(reused);
            IRenderVisual* visual = instance.get();
            models_.emplace(visual, std::move(instance));
            return visual;
        }
    }
    VisualRecord record;
    std::string error;
    const bool parsed = data ? parse_ogf_visual(
        {static_cast<const uint8_t*>(data->pointer()), data->length()}, record, error) :
        load_engine_model_visual(name, record, error);
    if (!parsed)
    {
        Msg("! [renderer-vulkan] OGF model '%s': %s", name ? name : "<reader>", error.c_str());
        return nullptr;
    }
    // Progressive, hierarchical, skinned and linked models have separate
    // milestones. Do not return a drawable instance with silently lost parts.
    if (record.type != 0 || !record.embedded_children.empty() || !record.linked_children.empty())
    {
        Msg("! [renderer-vulkan] OGF model '%s': unsupported static model type %u",
            name ? name : "<reader>", record.type);
        return nullptr;
    }
    auto model = std::make_unique<VulkanModelVisual>(record,
        !data && name ? name : "");
    if (!model->gpu().load(name, data, device_, queue_, pool_, memory_, upload_,
            *textures_, *pass_, error))
    {
        Msg("! [renderer-vulkan] OGF model '%s': %s", name ? name : "<reader>", error.c_str());
        return nullptr;
    }
    IRenderVisual* visual = model.get();
    models_.emplace(visual, std::move(model));
    return visual;
}

IRenderVisual* VulkanLevelRender::model_CreateChild(pcstr name, IReader* data)
{
    return model_Create(name, data);
}

void VulkanLevelRender::model_Delete(IRenderVisual*& visual, bool discard)
{
    if (!visual) return;
    auto found = models_.find(visual);
    R_ASSERT2(found != models_.end(), "Vulkan model deletion received an unknown visual");
    if (game_device_) game_device_->discard_model_draws(&found->second->gpu());
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan model deletion requires idle GPU frames");
    if (!discard && !found->second->cache_name().empty())
        model_pool_.emplace(found->second->cache_name(), std::move(found->second));
    models_.erase(found);
    visual = nullptr;
}

void VulkanLevelRender::models_Clear(bool)
{
    // The engine calls models_Clear(false) while live objects still hold
    // visuals. As in CModelPool::ClearPool, only unused instances are evicted.
    if (model_pool_.empty()) return;
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan model clear requires idle GPU frames");
    model_pool_.clear();
}

void VulkanLevelRender::destroy_all_models()
{
    if (models_.empty() && model_pool_.empty()) return;
    if (game_device_) game_device_->discard_scene_draws();
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan model teardown requires idle GPU frames");
    models_.clear();
    model_pool_.clear();
}

void VulkanLevelRender::add_Visual(u32, IRenderable* root, IRenderVisual* visual, Fmatrix& world)
{
    R_ASSERT2(game_device_, "Vulkan level visuals require a bound gameplay device");
    const int visual_index = level_.find_visual_index(visual);
    const auto model = models_.find(visual);
    R_ASSERT2(visual_index >= 0 || model != models_.end(),
        "Vulkan scene submission received a visual outside this renderer");
    const Fmatrix view_projection = current_view_projection();
    Fmatrix mvp;
    mvp.mul(view_projection, world);
    Fvector center = model != models_.end() ? model->second->getVisData().sphere.P :
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
    if (model != models_.end())
        game_device_->queue_model(model->second->gpu(), nullptr, transform, hud,
            distance_squared(center, camera_position));
    else
        game_device_->queue_level_visual(static_cast<uint32_t>(visual_index), transform, hud,
            distance_squared(center, camera_position));
}

IRender_ObjectSpecific* VulkanLevelRender::ros_create(IRenderable* parent)
{
    R_ASSERT2(parent, "Vulkan object lighting requires a renderable parent");
    auto* object = xr_new<VulkanObjectSpecific>(parent);
    object_specifics_.push_back(object);
    return object;
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
