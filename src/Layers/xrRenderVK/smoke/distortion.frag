#version 450
layout(set = 0, binding = 0) uniform sampler2D distortion_map;
layout(set = 1, binding = 0) uniform sampler2D scene_color;
layout(set = 1, binding = 2) uniform sampler2D scene_depth;
layout(push_constant) uniform Settings { layout(offset = 64) vec4 params; } settings;
layout(location = 1) in vec2 texcoord;
layout(location = 3) in vec4 tint;
layout(location = 0) out vec4 pixel_color;
void main()
{
    vec4 field = texture(distortion_map, texcoord);
    // particles_xdistort.s / particle_distort.ps: alpha weights the
    // displacement field. Its RGB encodes offsets, never emitted light.
    float weight = clamp(field.a * dot(tint.rgb, vec3(0.33)), 0.0, 1.0);
    if (weight < 0.01 / 255.0) discard;
    vec2 screen = gl_FragCoord.xy * settings.params.yz;
    vec2 border = 0.5 / vec2(textureSize(scene_color, 0));
    vec2 displaced = clamp(screen + (field.xy - vec2(127.0 / 255.0)) * 0.05,
        border, 1.0 - border);
    // Do not refract geometry which is in front of this particle.
    if (texture(scene_depth, displaced).r < gl_FragCoord.z)
        displaced = clamp(screen, border, 1.0 - border);
    pixel_color = vec4(texture(scene_color, displaced).rgb, weight);
}
