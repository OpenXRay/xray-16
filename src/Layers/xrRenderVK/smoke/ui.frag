#version 450
layout(location = 0) in vec4 fragment_color;
layout(location = 1) in vec2 fragment_uv;
layout(location = 0) out vec4 pixel_color;
layout(set = 0, binding = 0) uniform sampler2D color_map;
layout(set = 1, binding = 0) uniform sampler2D scene_map;
layout(push_constant) uniform UiConstants { vec2 framebuffer_size; float alpha_ref; float blend_mode; } ui;
void main()
{
    bool world_depth = ui.blend_mode >= 16.0;
    float blend_mode = world_depth ? ui.blend_mode - 16.0 : ui.blend_mode;
    if (world_depth)
    {
        ivec2 size = textureSize(scene_map, 0);
        ivec2 pixel = clamp(ivec2(gl_FragCoord.xy), ivec2(0), size - 1);
        // Read the depth copy made before the HUD clears its own attachment.
        if (gl_FragCoord.z > texelFetch(scene_map, pixel, 0).r) discard;
    }
    vec4 sample_color = texture(color_map, fragment_uv);
    if (blend_mode < 0.0)
    {
        float weight = clamp(sample_color.a * fragment_color.a, 0.0, 1.0);
        if (weight < 0.01 / 255.0) discard;
        vec2 extent = vec2(textureSize(scene_map, 0));
        vec2 uv = clamp(gl_FragCoord.xy / extent, 0.5 / extent, 1.0 - 0.5 / extent);
        vec3 scene = texture(scene_map, uv).rgb;
        // A menu selector must not move or duplicate glyphs in the captured UI.
        // Retain its alpha mask as a subtle neutral highlight, not vector RGB.
        pixel_color = vec4(scene * (1.0 + weight * 0.04), 1.0);
        return;
    }
    pixel_color = fragment_color * sample_color;
    if (blend_mode == 6.0)
    {
        pixel_color.rgb = mix(sample_color.rgb, fragment_color.rgb, fragment_color.a);
        pixel_color.a = sample_color.a * fragment_color.a;
    }
    else if (blend_mode >= 7.0)
    {
        pixel_color.rgb *= blend_mode == 9.0 ? 4.0 : 2.0;
        pixel_color.a = sample_color.a;
    }
    if (pixel_color.a < ui.alpha_ref) discard;
}
