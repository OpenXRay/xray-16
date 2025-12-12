cbuffer WindParams : register(b0)
{
    float g_time;
    float2 g_wind_direction;
    float g_wind_speed;
    uint g_octaves;
    float g_lacunarity;
    float g_gain;
    float g_pad1;
    float2 g_scroll_speed;
    uint g_texture_size;
    uint3 g_pad2;
};

RWTexture2D<float4> g_wind_output : register(u0);

float hash(float2 p)
{
    float h = dot(p, float2(127.1, 311.7));
    return frac(sin(h) * 43758.5453123);
}

float noise_wind_fbm(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));

    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

float fbm(float2 p, uint octaves, float lacunarity, float gain)
{
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float total_amplitude = 0.0;

    if (octaves >= 1)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    if (octaves >= 2)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    if (octaves >= 3)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    if (octaves >= 4)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    if (octaves >= 5)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    if (octaves >= 6)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    return total_amplitude > 0.0 ? value / total_amplitude : 0.0;
}

[numthreads(16, 16, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint2 pixel = dispatch_id.xy;

    if (pixel.x >= g_texture_size || pixel.y >= g_texture_size)
        return;

    float2 uv = pixel / float(g_texture_size);

    float2 world_pos = uv * 1000.0;
    world_pos += g_scroll_speed * g_time;

    float turbulence = fbm(world_pos * 0.01, g_octaves, g_lacunarity, g_gain);

    float angle = turbulence * 3.14159 * 2.0;
    float2 turbulence_vec = float2(cos(angle), sin(angle));

    float2 wind_vec = normalize(g_wind_direction + turbulence_vec * 0.3);

    float strength = turbulence * g_wind_speed;

    float temporal_wave = sin(g_time * 2.0 + world_pos.x * 0.1 + world_pos.y * 0.1) * 0.3 + 0.7;
    strength *= temporal_wave;

    float2 wind_encoded = wind_vec * 0.5 + 0.5;

    g_wind_output[pixel] = float4(wind_encoded.x, 0.0, wind_encoded.y, strength);
}
