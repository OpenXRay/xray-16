#version 450
layout(set = 0, binding = 0) uniform sampler2D water_texture;
layout(set = 1, binding = 0) uniform sampler2D refraction_scene;
layout(set = 1, binding = 1) uniform sampler2D reflection_scene;
layout(set = 1, binding = 2) uniform sampler2D opaque_depth;
layout(set = 1, binding = 3, std140) uniform Scene {
    mat4 view_projection;
    mat4 inverse_view_projection;
    vec4 camera_position;
} scene;
layout(push_constant) uniform Water { layout(offset = 64) vec4 params; } water;
layout(location = 0) in vec3 world_normal;
layout(location = 1) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;

void main()
{
    vec2 screen = gl_FragCoord.xy * water.params.yz;
    bool glass = water.params.w < 0.0;
    vec2 flow_a = texcoord * 2.0 + vec2(water.params.x * 0.035, water.params.x * 0.021);
    vec2 flow_b = texcoord * 1.7 - vec2(water.params.x * 0.027, water.params.x * 0.018);
    vec3 a = texture(water_texture, flow_a).rgb;
    vec3 b = texture(water_texture, flow_b).rgb;
    vec2 ripple = (a.rg + b.gb - 1.0) * 0.012;
    ripple += vec2(sin(water.params.x * 1.7 + texcoord.y * 18.0),
        cos(water.params.x * 1.3 + texcoord.x * 16.0)) * 0.002;
    if (glass) ripple = (a.rg - 0.5) * 0.002;
    float behind = texture(opaque_depth, screen).r;
    float shore = clamp((behind - gl_FragCoord.z) * 48.0, 0.0, 1.0);
    vec3 normal = normalize(world_normal + vec3(ripple * 12.0, 0.0));
    float fresnel = clamp(0.12 + pow(1.0 - abs(normal.y), 3.0) * 0.65, 0.1, 0.85);
    vec3 refracted = texture(refraction_scene, clamp(screen + ripple * shore, 0.001, 0.999)).rgb;
    // March the reflected view ray against the opaque depth buffer.
    vec4 surface_clip = scene.inverse_view_projection *
        vec4(screen.x * 2.0 - 1.0, 1.0 - screen.y * 2.0, gl_FragCoord.z, 1.0);
    vec3 surface = surface_clip.xyz / max(abs(surface_clip.w), 1e-5) * sign(surface_clip.w);
    vec3 view_ray = normalize(surface - scene.camera_position.xyz);
    vec3 reflected_ray = normalize(reflect(view_ray, normal));
    vec2 reflection_uv = clamp(vec2(screen.x, 1.0 - screen.y) + ripple * 2.0, 0.001, 0.999);
    for (int step_index = 1; step_index <= 16; ++step_index)
    {
        float step_length = 0.7 * float(step_index) * float(step_index);
        vec4 reflected_clip = scene.view_projection * vec4(surface + reflected_ray * step_length, 1.0);
        if (reflected_clip.w <= 0.0) break;
        vec3 projected = reflected_clip.xyz / reflected_clip.w;
        vec2 candidate = vec2(projected.x * 0.5 + 0.5, 0.5 - projected.y * 0.5);
        if (any(lessThan(candidate, vec2(0.002))) || any(greaterThan(candidate, vec2(0.998))) ||
            projected.z <= 0.0 || projected.z >= 1.0) break;
        float blocker = texture(opaque_depth, candidate).r;
        if (blocker < 0.99999 && projected.z > blocker + 0.0008)
        { reflection_uv = candidate; break; }
    }
    vec3 reflected = texture(reflection_scene, reflection_uv).rgb;
    vec3 water_color = mix(refracted, reflected, fresnel);
    if (!glass) water_color = mix(water_color, vec3(0.04, 0.16, 0.20), 0.18 * shore);
    float alpha = glass ? clamp(-water.params.w, 0.08, 0.65) :
        clamp(water.params.w * (0.25 + 0.75 * shore), 0.06, 0.88);
    pixel_color = vec4(water_color, alpha);
}
