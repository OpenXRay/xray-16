#version 450
layout(push_constant) uniform UiConstants { vec2 framebuffer_size; float alpha_ref; float blend_mode; } ui;
layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;
layout(location = 0) out vec4 fragment_color;
layout(location = 1) out vec2 fragment_uv;
void main()
{
    vec2 ndc = position / ui.framebuffer_size * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragment_color = color;
    fragment_uv = uv;
}
