#version 450
layout(set = 0, binding = 0) uniform texture2D frame_image;
layout(set = 0, binding = 1) uniform sampler frame_sampler;
layout(set = 0, binding = 2) uniform texture2D color_map_a;
layout(set = 0, binding = 3) uniform texture2D color_map_b;

layout(push_constant) uniform Settings {
    vec4 grade;
    vec4 tint;
    vec4 add;
    vec4 gray_weights;
    vec4 effect;
    vec4 noise;
    vec4 color_map;
} settings;
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 pixel_color;

float random_noise(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 sample_source(vec2 uv)
{
    vec2 border = 0.5 / vec2(textureSize(sampler2D(frame_image, frame_sampler), 0));
    return texture(sampler2D(frame_image, frame_sampler), clamp(uv, border, 1.0 - border)).rgb;
}

void main()
{
    vec2 pixel = 1.0 / vec2(textureSize(sampler2D(frame_image, frame_sampler), 0));
    // The legacy duality values are normalized screen offsets.
    vec2 dual = settings.effect.yz * 0.5;
    vec3 image = 0.5 * (sample_source(texcoord + dual) + sample_source(texcoord - dual));
    if (settings.effect.x > 0.0)
    {
        vec2 radius = pixel * settings.effect.x * 0.5;
        vec3 blurred = image * 4.0;
        blurred += sample_source(texcoord + vec2(radius.x, 0.0));
        blurred += sample_source(texcoord - vec2(radius.x, 0.0));
        blurred += sample_source(texcoord + vec2(0.0, radius.y));
        blurred += sample_source(texcoord - vec2(0.0, radius.y));
        image = blurred * 0.125;
    }
    if (settings.color_map.x > 0.0)
    {
        float luminance = clamp(dot(image, vec3(0.3333)), 0.001, 0.999);
        vec2 uv = vec2(luminance, 0.5);
        vec3 mapped = mix(texture(sampler2D(color_map_a, frame_sampler), uv).rgb,
                          texture(sampler2D(color_map_b, frame_sampler), uv).rgb,
                          settings.color_map.y);
        image = mix(image, mapped, settings.color_map.x);
    }
    float gray = dot(image, settings.gray_weights.rgb);
    image = mix(image, vec3(gray), settings.effect.w);
    if (settings.noise.x > 0.0)
    {
        float tick = floor(settings.noise.w * max(settings.noise.z, 0.01));
        vec2 cell = floor(texcoord * vec2(textureSize(sampler2D(frame_image, frame_sampler), 0)) /
                          max(settings.noise.y, 1.0));
        float n = random_noise(cell + tick * vec2(13.0, 37.0));
        image *= mix(1.0, n * 2.0, settings.noise.x);
    }
    image = image * settings.tint.rgb + settings.add.rgb;
    image = (image - 0.5) * settings.grade.z + 0.5;
    image = pow(max(image * settings.grade.y, 0.0),
                vec3(1.0 / max(settings.grade.x, 0.01)));
    pixel_color = vec4(image, 1.0);
}
