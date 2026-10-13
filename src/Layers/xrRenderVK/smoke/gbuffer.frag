#version 450
layout(set = 0, binding = 0) uniform sampler2D diffuse;
layout(location = 0) in vec3 world_normal;
layout(location = 1) in vec2 texcoord;
layout(location = 0) out vec4 albedo;
layout(location = 1) out vec4 normal_buffer;
layout(location = 2) out vec4 surface_buffer;
void main()
{
    albedo = texture(diffuse, texcoord);
    normal_buffer = vec4(normalize(world_normal) * 0.5 + 0.5, 1.0);
    surface_buffer = vec4(0.0);
}
