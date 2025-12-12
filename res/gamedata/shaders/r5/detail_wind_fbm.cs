// detail_wind_fbm.cs - Fractal Brownian Motion wind generation for interactive grass
// Phase 5, Milestone 5.4: Generate natural wind using procedural noise
//
// NOTE: Don't include common.h - it's for graphics shaders, not compute shaders

// Constant buffer for wind parameters
cbuffer WindParams : register(b0)
{
    float g_time;
    float2 g_wind_direction;      // Normalized wind direction (XZ)
    float g_wind_speed;            // Base wind speed
    uint g_octaves;                // Number of FBM octaves (typically 4-6)
    float g_lacunarity;            // Frequency multiplier (2.0)
    float g_gain;                  // Amplitude multiplier (0.5)
    float g_pad1;
    float2 g_scroll_speed;         // Texture scroll speed (wind_direction * speed * 0.1)
    uint g_texture_size;           // 512
    uint3 g_pad2;
};

// Output: Wind texture (RGB=wind vector XZ, A=strength)
RWTexture2D<float4> g_wind_output : register(u0);

// Hash function for procedural noise (GPU-friendly)
// Uses a well-tested hash from https://www.shadertoy.com
float hash(float2 p)
{
    // Convert to 1D hash
    float h = dot(p, float2(127.1, 311.7));
    return frac(sin(h) * 43758.5453123);
}

// Value noise
float noise_wind_fbm(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);

    // Smooth interpolation (smoothstep)
    f = f * f * (3.0 - 2.0 * f);

    // Sample grid corners
    float a = hash(i);
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));

    // Bilinear interpolation
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

// Fractal Brownian Motion - Manually unrolled for SM 5.0 compatibility
float fbm(float2 p, uint octaves, float lacunarity, float gain)
{
    float value = 0.0;
    float amplitude = 1.0;
    float frequency = 1.0;
    float total_amplitude = 0.0;

    // Manually unroll for up to 6 octaves (SM 5.0 compatible)
    // Octave 1
    if (octaves >= 1)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    // Octave 2
    if (octaves >= 2)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    // Octave 3
    if (octaves >= 3)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    // Octave 4
    if (octaves >= 4)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    // Octave 5
    if (octaves >= 5)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    // Octave 6
    if (octaves >= 6)
    {
        value += amplitude * noise_wind_fbm(p * frequency);
        total_amplitude += amplitude;
        frequency *= lacunarity;
        amplitude *= gain;
    }

    // Normalize to 0-1 range (safe division)
    return total_amplitude > 0.0 ? value / total_amplitude : 0.0;
}

// Generate wind field
[numthreads(16, 16, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID)
{
    uint2 pixel = dispatch_id.xy;

    // Bounds check
    if (pixel.x >= g_texture_size || pixel.y >= g_texture_size)
        return;

    // Normalized UV coordinates
    float2 uv = pixel / float(g_texture_size);

    // World-space position (1km tile size, tiling)
    float2 world_pos = uv * 1000.0;
    world_pos += g_scroll_speed * g_time;  // Animate wind by scrolling

    // Generate FBM for wind turbulence
    float turbulence = fbm(world_pos * 0.01, g_octaves, g_lacunarity, g_gain);

    // Rotate wind direction based on turbulence
    float angle = turbulence * 3.14159 * 2.0;  // 0-2π rotation
    float2 turbulence_vec = float2(cos(angle), sin(angle));

    // Blend global wind direction with local turbulence
    float2 wind_vec = normalize(g_wind_direction + turbulence_vec * 0.3);

    // Wind strength varies with turbulence
    float strength = turbulence * g_wind_speed;

    // Add temporal variation (increased frequency for more lively movement)
    float temporal_wave = sin(g_time * 2.0 + world_pos.x * 0.1 + world_pos.y * 0.1) * 0.3 + 0.7;
    strength *= temporal_wave;

    // Remap wind direction from [-1, 1] to [0, 1] for texture storage
    float2 wind_encoded = wind_vec * 0.5 + 0.5;

    // Output: XZ wind vector (encoded) + strength
    g_wind_output[pixel] = float4(wind_encoded.x, 0.0, wind_encoded.y, strength);
}
