#include "GameShaderResources.h"
#include <algorithm>
#include <cctype>

namespace xray::render::vulkan
{
bool GameShaderResources::canonical_name(const std::string &name, GameShaderStage stage, std::string &result, std::string &error)
{
    result.clear();
    if (stage != GameShaderStage::Vertex && stage != GameShaderStage::Pixel)
    {
        error = "unsupported legacy shader stage: " + name;
        return false;
    }
    std::string normalized = name;
    std::replace(normalized.begin(), normalized.end(), '/', '\\');
    if (normalized.empty() || normalized.front() == '\\' || normalized.find(':') != std::string::npos || normalized.find('\0') != std::string::npos ||
        normalized.find("..") != std::string::npos || normalized.find("\\\\") != std::string::npos || normalized.back() == '\\')
    {
        error = "invalid legacy shader name: " + name;
        return false;
    }
    const auto ends = [&](const char *suffix) {
        const std::string s(suffix);
        return normalized.size() >= s.size() && normalized.compare(normalized.size() - s.size(), s.size(), s) == 0;
    };
    if (ends(".spv"))
        normalized.resize(normalized.size() - 4);
    const bool vs = ends(".vs"), ps = ends(".ps");
    if ((vs && stage != GameShaderStage::Vertex) || (ps && stage != GameShaderStage::Pixel))
    {
        error = "legacy SVS/SPS stage mismatch: " + name;
        return false;
    }
    if (!vs && !ps)
        normalized += stage == GameShaderStage::Vertex ? ".vs" : ".ps";
    result = std::move(normalized);
    error.clear();
    return true;
}

void GameShaderResources::configure(VkDevice device, const ShaderModuleDispatch &dispatch, Source source)
{
    destroy();
    device_ = device;
    dispatch_ = dispatch;
    source_ = std::move(source);
}

bool GameShaderResources::shader(const std::string &name, GameShaderStage stage, const char *entry, uint32_t flags, GameShaderResource *&result,
                                 std::string &error)
{
    result = nullptr;
    std::string key;
    if (!canonical_name(name, stage, key, error))
        return false;
    if (!device_ || !source_ || !entry || std::string(entry) != "main" || flags)
    {
        error = "invalid precompiled legacy shader parameters (device/entry/flags): " + key;
        return false;
    }
    const auto found = resources_.find(key);
    if (found != resources_.end())
    {
        result = found->second.get();
        error.clear();
        return true;
    }
    std::vector<uint8_t> bytes;
    if (!source_(key + ".spv", bytes, error))
    {
        error = "legacy shader resource '" + key + "': " + error;
        return false;
    }
    if (!has_spirv_entry(bytes.data(), bytes.size(), static_cast<uint32_t>(stage), entry))
    {
        error = "legacy shader SPIR-V stage/entry mismatch: " + key;
        return false;
    }
    auto resource = std::make_unique<GameShaderResource>();
    resource->name = key;
    resource->stage = stage;
    if (!resource->module.initialize_bytes(device_, dispatch_, bytes.data(), bytes.size(), error))
    {
        error = "legacy shader module '" + key + "': " + error;
        return false;
    }
    result = resource.get();
    resources_.emplace(key, std::move(resource));
    error.clear();
    return true;
}

bool GameShaderResources::pipeline(DeferredPass &pass, const std::string &svs, const std::string &sps, SurfaceMode mode, bool hud, bool skinned,
                                   std::string &error)
{
    std::string vertex, pixel;
    if (!canonical_name(svs, GameShaderStage::Vertex, vertex, error) || !canonical_name(sps, GameShaderStage::Pixel, pixel, error))
        return false;
    if (mode != SurfaceMode::Opaque && mode != SurfaceMode::AlphaTest && mode != SurfaceMode::Transparent)
    {
        error = "invalid legacy SVS/SPS surface mode: " + vertex + " / " + pixel;
        return false;
    }
    if (hud && mode != SurfaceMode::Opaque)
    {
        error = "invalid legacy SVS/SPS HUD pass parameters: " + vertex + " / " + pixel;
        return false;
    }
    GameShaderResource *vs = nullptr, *ps = nullptr;
    if (!shader(vertex, GameShaderStage::Vertex, "main", 0, vs, error) || !shader(pixel, GameShaderStage::Pixel, "main", 0, ps, error))
        return false;
    return pass.create_game_pipeline(vs->name, ps->name, vs->module.handle(), ps->module.handle(), mode, hud, error, skinned);
}

void GameShaderResources::destroy()
{
    clear();
    device_ = VK_NULL_HANDLE;
    dispatch_ = {};
    source_ = {};
}
} // namespace xray::render::vulkan
