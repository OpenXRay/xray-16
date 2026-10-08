#pragma once

#include "LevelModels.h"

#include <array>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct ParticleEffectDef
{
    std::string name, shader, texture;
    uint32_t flags{}, max_particles{};
    float time_limit{-1.f};
    std::array<float, 2> frame_size{1.f, 1.f};
    uint32_t frame_columns{1}, frame_count{1};
    float frame_rate{24.f};
    std::vector<uint8_t> actions;
};

struct ParticleGroupItem
{
    std::string effect, on_play, on_birth, on_dead;
    float begin{}, end{};
    uint32_t flags{};
};

struct ParticleGroupDef
{
    std::string name;
    uint32_t flags{};
    float time_limit{};
    std::vector<ParticleGroupItem> effects;
};

struct ParticleCatalog
{
    std::vector<ParticleEffectDef> effects;
    std::vector<ParticleGroupDef> groups;
    const ParticleEffectDef* effect(const std::string& name) const;
    const ParticleGroupDef* group(const std::string& name) const;
};

bool parse_particle_catalog(LevelBytes bytes, ParticleCatalog& result, std::string& error);
} // namespace xray::render::vulkan
