#version 450
layout(location = 0) in vec4 fragment_color;
layout(location = 1) in vec2 fragment_uv;
layout(location = 0) out vec4 pixel_color;
layout(set = 0, binding = 0) uniform sampler2D color_map;
layout(set = 1, binding = 0) uniform sampler2D scene_map;
layout(push_constant) uniform UiConstants { vec2 framebuffer_size; float alpha_ref; float blend_mode; } ui;
void main()
{
    vec4 sample_color = texture(color_map, fragment_uv);
    if (ui.blend_mode < 0.0)
    {
        float weight = clamp(sample_color.a * fragment_color.a, 0.0, 1.0);
        if (weight < 0.01 / 255.0) discard;
        vec2 extent = vec2(textureSize(scene_map, 0));
        // Desktop first alpha-blends the field over (127,127,0,127),
        // then samples the scene at its displaced screen coordinate.
        vec2 offset = (sample_color.xy * fragment_color.xy - vec2(127.0 / 255.0)) * weight * 0.05;
        vec2 uv = clamp(gl_FragCoord.xy / extent + offset, 0.5 / extent, 1.0 - 0.5 / extent);
        vec3 scene = texture(scene_map, uv).rgb;
        pixel_color = vec4(scene, 1.0);
        return;
    }
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
