#version 450
layout(set = 1, binding = 0) uniform sampler2D scene_color;
layout(set = 1, binding = 1) uniform sampler2D distortion_field;
layout(set = 1, binding = 2) uniform sampler2D opaque_depth;
layout(set = 1, binding = 3, std140) uniform Scene {
    mat4 view_projection;
    mat4 inverse_view_projection;
    vec4 camera_position;
} scene;
layout(location = 0) out vec4 color;

float view_depth(vec2 uv)
{
    float depth = texture(opaque_depth, uv).r;
    vec4 world = scene.inverse_view_projection * vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    return 1.0 / max(abs(world.w), 1e-8);
}

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 size = vec2(textureSize(scene_color, 0));
    vec2 uv = gl_FragCoord.xy / size;
    vec2 field = texelFetch(distortion_field, pixel, 0).rg - vec2(127.0 / 255.0);
    if (all(lessThanEqual(abs(field), vec2(.5 / 255.0))))
    {
        color = texelFetch(scene_color, pixel, 0);
        return;
    }
    vec2 displacement = field * .05;
    vec2 displaced = clamp(uv + displacement, .5 / size, 1.0 - .5 / size);
    if (view_depth(displaced) + .001 < view_depth(uv))
        displaced = uv;
    color = texture(scene_color, displaced);
}
