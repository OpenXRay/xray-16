#include "xrEngine/stdafx.h"
#include "src/Layers/xrRenderVK/ParticleVisual.h"

#include <cassert>

using namespace xray::render::vulkan;

int main()
{
    Core.Initialize("vulkan_particle_visual_test", nullptr, false);
    auto catalog = std::make_shared<ParticleCatalog>();
    auto& def = catalog->effects.emplace_back();
    def.name = "smoke";
    def.max_particles = 8;
    def.actions = {0, 0, 0, 0};
    def.time_limit = 1.f;
    auto& group = catalog->groups.emplace_back();
    group.name = "smoke_group";
    group.time_limit = .5f;
    group.effects.push_back({"smoke", "smoke", "", "", 0.f, .1f, 7u});
    group.effects.push_back({"smoke", "", "", "", .2f, .4f, 5u});
    {
        VulkanParticleEffect first(catalog, def);
        VulkanParticleEffect second(catalog, def);
        first.Play();
        first.OnFrame(40);
        assert(first.IsPlaying() && !second.IsPlaying());
        first.Stop(FALSE);
        assert(!first.IsPlaying() && first.ParticlesCount() == 0);
        VulkanParticleGroup instance(catalog, group);
        assert(instance.getType() == 9 && instance.getSubModel(0) && instance.getSubModel(1));
        assert(instance.getSubModel(2) == nullptr);
        instance.Play();
        instance.OnFrame(40);
        assert(instance.effects()[0]->IsPlaying() && !instance.effects()[1]->IsPlaying());
        assert(instance.triggered_count() == 1);
        instance.OnFrame(80);
        assert(!instance.effects()[0]->IsPlaying());
        instance.OnFrame(100);
        assert(instance.effects()[1]->IsPlaying());
        instance.Stop(FALSE);
        assert(!instance.IsPlaying() && !instance.effects()[0]->IsPlaying() &&
            !instance.effects()[1]->IsPlaying());
    }
    // A group whose start and end both lie in one update must emit once,
    // matching the legacy particle group's state transition ordering.
    ParticleGroupDef short_group;
    short_group.name = "short_flame";
    short_group.effects.push_back({"smoke", "", "", "", 0.f, .01f, 4u});
    VulkanParticleGroup short_effect(catalog, short_group);
    short_effect.Play();
    short_effect.OnFrame(40);
    assert(short_effect.effects()[0]->IsPlaying());
}
