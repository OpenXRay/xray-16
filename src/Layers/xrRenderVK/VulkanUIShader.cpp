#include "xrEngine/stdafx.h"
#include "VulkanUIShader.h"
#include "ScenePass.h"
#include "xrEngine/device.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>

namespace xray::render::vulkan
{
void VulkanUIShader::Copy(IUIShader& source)
{
    const auto* other = dynamic_cast<VulkanUIShader*>(&source);
    R_ASSERT2(other && other->textures_ == textures_ && other->pass_ == pass_,
        "Vulkan UI shaders can only be copied within their owning device");
    if (other == this) return;
    destroy();
    shader_ = other->shader_;
    texture_ = other->texture_;
    blend_mode_ = other->blend_mode_;
    alpha_ref_ = other->alpha_ref_;
    video_ = other->video_;
    if (!other->sequence_.empty())
    {
        create(other->shader_.c_str(), other->texture_.c_str());
        return;
    }
    if (video_)
    {
        descriptor_ = video_->descriptor();
        extent_ = video_->extent();
        return;
    }
    if (other->descriptor_)
    {
        std::string error;
        if (!textures_->ui(texture_, *pass_, descriptor_, error, &extent_))
        {
            Msg("! [renderer-vulkan] UI shader copy '%s': %s", texture_.c_str(), error.c_str());
            use_transparent_fallback();
        }
    }
}

void VulkanUIShader::create(LPCSTR shader, LPCSTR texture)
{
    destroy();
    shader_ = shader ? shader : "";
    texture_ = texture ? texture : "";
    // Some engine-provided UI shaders (fonts, movies) have no shaders.xr
    // blender entry. They still need their real texture/video descriptor.
    if (!shader_.empty() && textures_->has_blender(shader_))
    {
        SurfaceMode surface{};
        std::string error;
        if (!textures_->surface_mode(shader_, texture_, surface, error,
                &alpha_ref_, &blend_mode_, false, true))
        {
            Msg("! [renderer-vulkan] UI material shader='%s' texture='%s': %s",
                shader_.c_str(), texture_.c_str(), error.c_str());
            use_transparent_fallback();
            return;
        }
    }
    if (texture_.empty()) { use_transparent_fallback(); return; }
    string_path video_path;
    if (FS.exist(video_path, "$game_textures$", texture_.c_str(), ".ogm"))
    {
        video_ = std::make_shared<VulkanVideoTexture>(*textures_, *pass_);
        std::string error;
        if (!video_->load(video_path, Device.dwTimeContinual, error))
        {
            Msg("! [renderer-vulkan] UI movie '%s': %s", texture_.c_str(), error.c_str());
            destroy();
            use_transparent_fallback();
            return;
        }
        descriptor_ = video_->descriptor();
        extent_ = video_->extent();
        return;
    }
    string_path sequence_path;
    if (FS.exist(sequence_path, "$game_textures$", texture_.c_str(), ".seq"))
    {
        IReader* source = FS.r_open(sequence_path);
        if (!source) { use_transparent_fallback(); return; }
        string256 line;
        source->r_string(line, sizeof(line));
        _Trim(line);
        if (xr_stricmp(line, "cycled") == 0)
        {
            sequence_cycled_ = true;
            source->r_string(line, sizeof(line));
            _Trim(line);
        }
        const int fps = std::atoi(line);
        if (fps <= 0 || fps > 1000)
        {
            Msg("! [renderer-vulkan] UI sequence '%s': invalid frame rate '%s'", texture_.c_str(), line);
            FS.r_close(source);
            use_transparent_fallback();
            return;
        }
        sequence_ms_per_frame_ = std::max(1, 1000 / fps);
        while (!source->eof() && sequence_.size() < 256)
        {
            source->r_string(line, sizeof(line));
            _Trim(line);
            if (!line[0]) continue;
            VkDescriptorSet frame = VK_NULL_HANDLE;
            VkExtent2D size{};
            std::string error;
            if (textures_->ui(line, *pass_, frame, error, &size))
                sequence_.push_back({frame, size});
            else
                Msg("! [renderer-vulkan] UI sequence '%s' frame='%s': %s", texture_.c_str(), line, error.c_str());
        }
        FS.r_close(source);
        if (sequence_.empty())
        {
            Msg("! [renderer-vulkan] UI sequence '%s' has no readable frames", texture_.c_str());
            use_transparent_fallback();
            return;
        }
        descriptor_ = sequence_.front().descriptor;
        extent_ = sequence_.front().extent;
        Msg("[renderer-vulkan] UI sequence '%s' frames=%zu fps=%d cycled=%d",
            texture_.c_str(), sequence_.size(), fps, sequence_cycled_ ? 1 : 0);
        return;
    }
    std::string error;
    if (!textures_->ui(texture_, *pass_, descriptor_, error, &extent_))
    {
        Msg("! [renderer-vulkan] UI texture '%s': %s", texture_.c_str(), error.c_str());
        use_transparent_fallback();
    }
    if (texture_.find("highlight") != std::string::npos ||
        texture_.find("loading_progress") != std::string::npos)
        Msg("[renderer-vulkan] ui.asset shader='%s' texture='%s' extent=%ux%u valid=%d",
            shader_.c_str(), texture_.c_str(), extent_.width, extent_.height,
            descriptor_ != VK_NULL_HANDLE ? 1 : 0);
}

void VulkanUIShader::use_transparent_fallback()
{
    static constexpr uint8_t transparent[]{0, 0, 0, 0};
    std::string error;
    if (!textures_->ui_pixels(transparent, 1, 1, *pass_, descriptor_, error))
        Msg("! [renderer-vulkan] UI transparent fallback '%s': %s", texture_.c_str(), error.c_str());
    else
        extent_ = {1, 1};
}

void VulkanUIShader::destroy()
{
    const bool was_sequence = !sequence_.empty();
    for (const auto& frame : sequence_)
        textures_->release_ui(frame.descriptor, *pass_);
    sequence_.clear();
    sequence_ms_per_frame_ = 0;
    sequence_cycled_ = false;
    if (descriptor_ && !video_ && !was_sequence)
        textures_->release_ui(descriptor_, *pass_);
    video_.reset();
    shader_.clear();
    texture_.clear();
    descriptor_ = VK_NULL_HANDLE;
    extent_ = {};
    blend_mode_ = 1;
    alpha_ref_ = 0;
}

bool VulkanUIShader::operator==(const IUIShader& source) const
{
    const auto* other = dynamic_cast<const VulkanUIShader*>(&source);
    return other && textures_ == other->textures_ && pass_ == other->pass_ &&
        shader_ == other->shader_ && texture_ == other->texture_;
}

bool VulkanUIShader::GetBaseTextureResolution(Fvector2& size)
{
    size.set(static_cast<float>(extent_.width), static_cast<float>(extent_.height));
    return inited() && extent_.width && extent_.height;
}

VkDescriptorSet VulkanUIShader::current_descriptor(u32 time)
{
    if (!sequence_.empty())
    {
        const size_t count = sequence_.size();
        size_t frame = (time / sequence_ms_per_frame_) % (sequence_cycled_ ? count * 2 : count);
        if (sequence_cycled_ && frame >= count) frame = count * 2 - 1 - frame;
        descriptor_ = sequence_[frame].descriptor;
        extent_ = sequence_[frame].extent;
    }
    if (video_)
    {
        video_->update(time);
        descriptor_ = video_->descriptor();
    }
    return descriptor_;
}

xrImTextureData VulkanUIShader::GetImGuiTextureId()
{
    // Both draw paths use ScenePass's sampled UI descriptor layout. ImGui
    // stores the descriptor handle in its texture ID until shader release.
    xrImTextureData result{};
    const VkDescriptorSet set = current_descriptor(Device.dwTimeContinual);
    static_assert(sizeof(set) <= sizeof(result.texture));
    std::memcpy(&result.texture, &set, sizeof(set));
    result.size.set(float(extent_.width), float(extent_.height));
    return result;
}
}
