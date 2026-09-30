#version 450
layout(location = 0) in vec4 fragment_color;
layout(location = 1) in vec2 fragment_uv;
layout(location = 0) out vec4 pixel_color;
layout(set = 0, binding = 0) uniform sampler2D color_map;
layout(push_constant) uniform UiConstants { vec2 framebuffer_size; float alpha_ref; } ui;
void main()
{
    pixel_color = fragment_color * texture(color_map, fragment_uv);
    if (pixel_color.a < ui.alpha_ref) discard;
}
