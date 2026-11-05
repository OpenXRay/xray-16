#version 450
layout(set = 0, binding = 0) uniform sampler2D albedo_buffer;
layout(set = 0, binding = 1) uniform sampler2D normal_buffer;
layout(set = 0, binding = 2) uniform sampler2D depth_buffer;
layout(set = 0, binding = 5) uniform sampler2D surface_buffer;
layout(set = 1, binding = 0) uniform sampler2DArray shadow_array;
layout(set = 1, binding = 1, std140) uniform LocalLight {
    mat4 inverse_view_projection;
    mat4 shadow_matrices[6];
    vec4 position_range;
    vec4 direction_cone;
    vec4 color_type;
    vec4 shadow_params;
    vec4 spot_right;
    vec4 debug_mode;
} light;
layout(set=1,binding=2) uniform sampler2D spot_projection;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;

float visibility(vec3 world, vec3 delta, vec3 normal)
{
    if (light.shadow_params.z < 0.5) return 1.0;
    int face = 0;
    if (light.color_type.w < 0.5)
    {
        vec3 a = abs(delta);
        face = a.x >= a.y && a.x >= a.z ? (delta.x >= 0.0 ? 0 : 1) :
            a.y >= a.z ? (delta.y >= 0.0 ? 2 : 3) : (delta.z >= 0.0 ? 4 : 5);
    }
    vec4 clip = light.shadow_matrices[face] * vec4(world, 1.0);
    if (clip.w <= 0.0) return 1.0;
    vec3 projected = clip.xyz / clip.w;
    vec2 uv = projected.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) ||
        projected.z <= 0.0 || projected.z >= 1.0) return 1.0;
    // Bias in world units: a fixed 0..1 perspective-depth offset erases
    // shadows at distance because depth precision scales with 1/z squared.
    vec3 receiver = world - normalize(delta) * light.shadow_params.y;
    vec4 receiver_clip = light.shadow_matrices[face] * vec4(receiver, 1.0);
    if (receiver_clip.w <= 0.0) return 1.0;
    float reference_depth = receiver_clip.z / receiver_clip.w;
    // Correct each PCF tap to the receiver plane. A single centre depth
    // self-shadows sloped faces when the light texel covers several pixels.
    // Derive the receiver plane from its world normal. Screen derivatives
    // cross silhouettes and are undefined after the range/cone branches.
    vec3 tangent = normalize(cross(normal, abs(normal.y) < .9 ? vec3(0,1,0) : vec3(1,0,0)));
    vec3 bitangent = cross(normal, tangent);
    vec4 tx = light.shadow_matrices[face] * vec4(tangent, 0.0);
    vec4 ty = light.shadow_matrices[face] * vec4(bitangent, 0.0);
    vec3 dx = (tx.xyz - projected * tx.w) / clip.w;
    vec3 dy = (ty.xyz - projected * ty.w) / clip.w;
    vec2 ux = dx.xy * .5, uy = dy.xy * .5;
    float determinant = ux.x * uy.y - ux.y * uy.x;
    vec2 gradient = abs(determinant) > 1e-12 ?
        vec2(uy.y * dx.z - ux.y * dy.z, ux.x * dy.z - uy.x * dx.z) / determinant : vec2(0);
    float layer = light.shadow_params.x + float(face);
    ivec2 size = textureSize(shadow_array, 0).xy;
    vec2 grid = uv * vec2(size) - 0.5;
    ivec2 center = ivec2(floor(grid));
    vec2 fraction = grid - vec2(center);
    float lit = 0.0, total = 0.0;
    for (int y = 0; y <= 1; ++y)
        for (int x = 0; x <= 1; ++x)
        {
            vec2 weight = vec2(x == 0 ? 1.0-fraction.x : fraction.x, y == 0 ? 1.0-fraction.y : fraction.y);
            float w = weight.x * weight.y;
            ivec2 pixel = clamp(center + ivec2(x,y), ivec2(0), size - 1);
            float tap_depth = reference_depth + clamp(dot(gradient, (vec2(pixel) + .5) / vec2(size) - uv), -.01, .01);
            lit += (tap_depth <=
                texelFetch(shadow_array, ivec3(pixel, int(layer)), 0).r ? 1.0 : 0.0) * w;
            total += w;
        }
    return lit / max(total, 1e-6);
}

void main()
{
    // Match the base panel's G-buffer lookup. Local lights previously vanished
    // from all four-panel screenshots, making lamps and interiors look broken.
    vec2 screen_uv = ((light.debug_mode.x > 0.5 && light.debug_mode.x < 1.5) || light.debug_mode.x == 14.0) ?
        texcoord * 2.0 : texcoord;
    if (any(greaterThanEqual(screen_uv, vec2(1.0))))
    { pixel_color = vec4(0.0); return; }
    float depth = texture(depth_buffer, screen_uv).r;
    if (depth >= 0.99999) { pixel_color = vec4(0.0); return; }
    vec4 world_clip = light.inverse_view_projection *
        vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0, depth, 1.0);
    vec3 world = world_clip.xyz / max(abs(world_clip.w), 1e-6) * sign(world_clip.w);
    vec3 delta = world - light.position_range.xyz;
    float distance = length(delta);
    float range = max(light.position_range.w, 0.01);
    if (distance >= range || distance <= 1e-5) { pixel_color = vec4(0.0); return; }
    vec3 normal = normalize(texture(normal_buffer, screen_uv).rgb * 2.0 - 1.0);
    vec4 surface = texture(surface_buffer, screen_uv);
    float diffuse = max(dot(normal, -delta / distance), 0.0);
    float normalized_distance = distance / range;
    float attenuation = max(1.0 - normalized_distance * normalized_distance, 0.0);
    if (light.color_type.w >= 0.5)
        attenuation *= smoothstep(light.direction_cone.w, min(1.0, light.direction_cone.w + 0.05),
            dot(normalize(delta), normalize(light.direction_cone.xyz)));
    if(light.shadow_params.w>0.5)
    {
        vec3 direction=normalize(light.direction_cone.xyz);
        vec3 right=normalize(light.spot_right.xyz-direction*dot(light.spot_right.xyz,direction));
        vec3 up=cross(direction,right);
        float z=dot(delta,direction);
        vec2 projection_uv=vec2(dot(delta,right),-dot(delta,up))/max(z*light.spot_right.w,1e-6)*.5+.5;
        if(z<=0.0 || any(lessThan(projection_uv,vec2(0))) || any(greaterThan(projection_uv,vec2(1))))
        {pixel_color=vec4(0);return;}
        attenuation*=texture(spot_projection,projection_uv).r;
    }
    vec3 color = texture(albedo_buffer, screen_uv).rgb * light.color_type.rgb * diffuse;
    if (surface.r > 0.0)
    {
        vec4 near_clip = light.inverse_view_projection *
            vec4(screen_uv.x * 2.0 - 1.0, 1.0 - screen_uv.y * 2.0, 0.0, 1.0);
        vec3 view_ray = normalize(near_clip.xyz / near_clip.w - world);
        float specular = pow(max(dot(normal, normalize(view_ray - delta / distance)), 0.0),
            mix(4.0, 64.0, surface.r));
        color += specular * surface.r * mix(0.04, 0.2, surface.g) * light.color_type.rgb;
    }
    color *= attenuation * visibility(world, delta, normal);
    pixel_color = vec4(color, 0.0);
}
