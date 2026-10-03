#pragma once

#include "DeferredPass.h"
#include "ShaderModule.h"
#include <functional>
#include <vector>

namespace xray::render::vulkan
{
enum class GameShaderStage : uint32_t
{
    Vertex = 0,
    Pixel = 4
};

// Vulkan counterparts of the legacy SVS/SPS resources. Neither contains a GL
// object. A returned resource is borrowed until clear/reload/device teardown.
struct GameShaderResource
{
    std::string name;
    GameShaderStage stage{};
    ShaderModule module;
};

class GameShaderResources
{
  public:
    using Source = std::function<bool(const std::string &, std::vector<uint8_t> &, std::string &)>;
    void configure(VkDevice device, const ShaderModuleDispatch &dispatch, Source source);
    bool shader(const std::string &name, GameShaderStage stage, const char *entry, uint32_t flags, GameShaderResource *&result, std::string &error);
    bool pipeline(DeferredPass &pass, const std::string &svs, const std::string &sps, SurfaceMode mode, bool hud, bool skinned, std::string &error);
    void clear()
    {
        resources_.clear();
    }
    void destroy();
    size_t size() const
    {
        return resources_.size();
    }
    static bool canonical_name(const std::string &name, GameShaderStage stage, std::string &result, std::string &error);

  private:
    VkDevice device_{};
    ShaderModuleDispatch dispatch_{};
    Source source_;
    std::unordered_map<std::string, std::unique_ptr<GameShaderResource>> resources_;
};
} // namespace xray::render::vulkan
