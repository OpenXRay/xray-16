#version 450
layout(set = 0, binding = 0) uniform sampler2D diffuse;
layout(push_constant) uniform MaterialCutout { mat4 mvp; float alpha_ref; } material;
layout(location = 0) in vec2 texcoord;
void main()
{
    if (texture(diffuse, texcoord).a < material.alpha_ref) discard;
}
