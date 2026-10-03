#version 450
layout(set = 0, binding = 0) uniform sampler2D diffuse;
layout(location = 0) in vec2 texcoord;
void main()
{
    if (texture(diffuse, texcoord).a < 0.5) discard;
}
