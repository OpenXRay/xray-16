#version 450
layout(set = 0, binding = 0) uniform sampler2D diffuse;
layout(push_constant) uniform MaterialCutout { mat4 mvp; float alpha_ref; } material;
layout(location = 0) in vec3 world_normal;
layout(location = 1) in vec2 texcoord;
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal_buffer;
void main()
{
    albedo = texture(diffuse, texcoord);
    if (albedo.a < material.alpha_ref)
        discard;
    normal_buffer = vec4(normalize(world_normal) * 0.5 + 0.5, 1.0);
}
