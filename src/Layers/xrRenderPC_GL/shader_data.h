#pragma once

#include <glm/glm.hpp>

namespace xray::render::RENDER_NAMESPACE
{
#pragma pack(push, 1)
struct ShaderInstanceData
{
    glm::mat4 xform;
    glm::mat4 xformView;
    glm::vec4 consts;
    glm::vec4 scale;
    glm::vec4 bias;
    glm::vec4 wind;
    glm::vec4 wave;
    glm::vec3 sun;

private:
    [[maybe_unused]]
    glm::vec1 _padding {};
}; // 224 bytes - fulfilling GLSL std140/std430 padding rules
#pragma pack(pop)
}