#version 450
layout(push_constant) uniform Camera { mat4 mvp; } camera;
layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;
layout(location = 0) out vec2 texcoord;
void main()
{
    gl_Position = camera.mvp * vec4(position, 1.0);
    texcoord = uv;
}
