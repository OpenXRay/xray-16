#pragma once

namespace xray::ecs { class World; }

namespace xray::animation::bone_systems {

void ApplyLocalOverrides(xray::ecs::World&);
void ApplyBoneVisibility(xray::ecs::World&);
void UpdateSocketLocalTransforms(xray::ecs::World&);

}
