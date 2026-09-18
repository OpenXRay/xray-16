#pragma once

namespace xray::ecs { class World; }

namespace xray::animation::systems {

void Sample(xray::ecs::World&);
void MultiChannelSample(xray::ecs::World&);
void Blend(xray::ecs::World&);
void LocalToModel(xray::ecs::World&);
void IK(xray::ecs::World&);

void ProcessRequests(xray::ecs::World&);

}
