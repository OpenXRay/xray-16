#version 450
layout(push_constant) uniform Camera { mat4 mvp; } camera;
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;
layout(location = 6) in vec4 color;
layout(location = 1) out vec2 texcoord;
layout(location = 3) out vec4 tint;
void main()
{
    gl_Position = camera.mvp * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
    texcoord = uv;
    tint = color;
}
