#version 450
layout(location = 0) in vec4 fragment_color;
layout(location = 1) in vec2 fragment_uv;
layout(location = 0) out vec4 pixel_color;
layout(set = 0, binding = 0) uniform sampler2D color_map;
layout(push_constant) uniform UiConstants { vec2 framebuffer_size; float alpha_ref; float blend_mode; } ui;
void main()
{
    vec4 sample_color = texture(color_map, fragment_uv);
    pixel_color = fragment_color * sample_color;
    if (ui.blend_mode == 6.0)
    {
        pixel_color.rgb = mix(sample_color.rgb, fragment_color.rgb, fragment_color.a);
        pixel_color.a = sample_color.a * fragment_color.a;
    }
    else if (ui.blend_mode >= 7.0)
    {
        pixel_color.rgb *= ui.blend_mode == 9.0 ? 4.0 : 2.0;
        pixel_color.a = sample_color.a;
    }
    if (pixel_color.a < ui.alpha_ref) discard;
}
