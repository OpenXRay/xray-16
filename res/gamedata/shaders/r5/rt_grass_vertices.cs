struct InstanceData {
    float3 pos;
    uint packed;
};

struct GPUSlotData {
    float world_min_x;
    float world_min_z;
    float y_base;
    float y_height;
    uint packed_ids;
    uint packed_palette_01;
    uint packed_palette_23;
    float hemi;
};

StructuredBuffer<InstanceData> g_AllInstances : register(t0);
StructuredBuffer<GPUSlotData> g_SlotData : register(t1);
StructuredBuffer<uint> g_VisibleIndices : register(t2);
Texture2D g_WindTexture : register(t3);
SamplerState g_LinearSampler : register(s0);
RWByteAddressBuffer g_Output : register(u0);

cbuffer GrassRTCB : register(b5) {
    float4 detail_params;
    float4 g_wind_direction;
    float4 wave;
    float grass_wind_displacement;
    float grass_blade_height;
    float grass_blade_width;
    uint segments;
    uint vertsPerBlade;
    uint bladeCount;
    uint outputVertexOffset;
    uint pad;
};

static const float PACK_MAX_SCALE = 4.0;
static const float TWO_PI = 6.28318530718;
static const float M_PI = 3.1415926;
static const float HEIGHT_VARIATION_MIN = 0.7;
static const float HEIGHT_VARIATION_MAX = 1.15;
static const float HEIGHT_NOISE_SCALE = 0.05;

uint pack_normal(float3 n)
{
    uint3 u = uint3(clamp(n * 127.5 + 127.5, 0, 255));
    return (u.x << 16) | (u.y << 8) | u.z;
}

[numthreads(256, 1, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
    uint global_vert = dtid.x;
    uint blade_idx = global_vert / vertsPerBlade;
    uint local_vert = global_vert % vertsPerBlade;

    if (blade_idx >= bladeCount)
        return;

    uint src_idx = g_VisibleIndices[blade_idx];
    InstanceData raw = g_AllInstances[src_idx];

    float3 base_pos = raw.pos;
    uint object_id = raw.packed & 0x3F;
    float rotation = float((raw.packed >> 8) & 0x3FF) / 1023.0 * TWO_PI;
    float scale = float((raw.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;

    float2 height_uv = base_pos.xz * HEIGHT_NOISE_SCALE + float2(0.37, 0.73);
    float height_noise = g_WindTexture.SampleLevel(g_LinearSampler, height_uv, 0).r;
    float height_multiplier = lerp(HEIGHT_VARIATION_MIN, HEIGHT_VARIATION_MAX, height_noise);
    float blade_height_val = scale * height_multiplier * grass_blade_height;

    float t;
    float local_x;
    float2 uv;
    if (local_vert < segments * 2) {
        uint seg = local_vert / 2;
        uint side = local_vert % 2;
        t = float(seg) / float(segments);
        float taper = 1.0 - pow(t, 0.7);
        float width = grass_blade_width * taper;
        local_x = (side == 0) ? -width * 0.5 : width * 0.5;
        uv = float2(float(side), 1.0 - t);
    } else {
        t = 1.0;
        local_x = 0.0;
        uv = float2(0.5, 0.0);
    }

    float3 P0 = base_pos;
    float3 P1 = base_pos + float3(0, blade_height_val * 0.33, 0);
    float3 P2 = base_pos + float3(0, blade_height_val * 0.67, 0);
    float3 P3 = base_pos + float3(0, blade_height_val, 0);

    float cos_rot = cos(rotation);
    float sin_rot = sin(rotation);
    float3 facing = normalize(float3(sin_rot, 0.0, cos_rot));
    float3 right = float3(cos_rot, 0.0, -sin_rot);

    float wind_speed = max(g_wind_direction.y, 0.1);
    float time = wave.w;

    float2 dir_uv = P0.zx * (0.005 / wind_speed) + time * (0.005 * wind_speed);
    float wind_dir_noise = g_WindTexture.SampleLevel(g_LinearSampler, dir_uv, 0).r;

    float2 str_uv = P0.xz * (0.025 / wind_speed) + time * 0.05;
    float wind_str_noise = g_WindTexture.SampleLevel(g_LinearSampler, str_uv, 0).r;

    float height_factor = t;
    float2 turb_uv = P0.xz * (0.025 / wind_speed) + (time + height_factor * height_factor * 0.25) * 0.05;
    float wind_turb_noise = g_WindTexture.SampleLevel(g_LinearSampler, turb_uv, 0).r;

    float fbm_wind_strength = lerp(0.25, 1.0, wind_str_noise);
    fbm_wind_strength *= fbm_wind_strength;
    fbm_wind_strength *= wind_speed;

    float wind_turbulence = lerp(0.25, 1.0, wind_turb_noise);
    wind_turbulence *= wind_turbulence;
    wind_turbulence *= min(wind_speed, 1.0) * 0.3;

    float fbm_turbulence = (wind_dir_noise * 2.0 - 1.0) * 0.3;
    float wind_angle_rad = g_wind_direction.x * (M_PI / 180.0);
    float2 global_wind_dir = float2(sin(wind_angle_rad), cos(wind_angle_rad));
    float2 perpendicular_dir = float2(-global_wind_dir.y, global_wind_dir.x);
    float2 wind_dir_with_turbulence = normalize(global_wind_dir + perpendicular_dir * fbm_turbulence);

    float2 facing_dir_xz = facing.xz;
    float2 wind_dir_xz = normalize(float2(wind_dir_with_turbulence.x, wind_dir_with_turbulence.y));
    float alignment = dot(wind_dir_xz, facing_dir_xz);
    float resistance = lerp(0.3, 1.0, abs(alignment));

    float wind_bend_strength = fbm_wind_strength * grass_wind_displacement * resistance;
    float3 wind_bend_dir = float3(wind_dir_xz.x, 0.0, wind_dir_xz.y);
    float3 total_bend_force = wind_bend_dir * wind_bend_strength;
    float total_bend_strength_val = length(total_bend_force);

    if (total_bend_strength_val > 0.001)
    {
        float3 bend_dir = normalize(total_bend_force);
        float max_bend_displacement = blade_height_val * 0.8;
        float bend_ratio = saturate(total_bend_strength_val / max_bend_displacement);
        float base_bend_angle = bend_ratio * 1.2;
        float bend_angle = base_bend_angle + wind_turbulence * height_factor;

        float tip_horizontal = blade_height_val * sin(bend_angle);
        float tip_vertical_drop = blade_height_val * (1.0 - cos(bend_angle));
        P3 += bend_dir * tip_horizontal;
        P3.y -= tip_vertical_drop;

        float p1_angle = bend_angle * 0.33;
        P1 += bend_dir * (blade_height_val * 0.33 * sin(p1_angle));
        P1.y -= blade_height_val * 0.33 * (1.0 - cos(p1_angle));

        float p2_angle = bend_angle * 0.67;
        P2 += bend_dir * (blade_height_val * 0.67 * sin(p2_angle));
        P2.y -= blade_height_val * 0.67 * (1.0 - cos(p2_angle));
    }

    float mt = 1.0 - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    float t2 = t * t;
    float t3 = t2 * t;
    float3 bezier_pos = mt3 * P0 + 3.0 * mt2 * t * P1 + 3.0 * mt * t2 * P2 + t3 * P3;

    float3 tangent_raw = 3.0 * mt2 * (P1 - P0) + 6.0 * mt * t * (P2 - P1) + 3.0 * t2 * (P3 - P2);
    float tangent_len = length(tangent_raw);
    float3 tangent = (tangent_len > 0.001) ? (tangent_raw / tangent_len) : float3(0, 1, 0);

    float3 width_offset = right * (local_x * scale);
    float3 world_pos = bezier_pos + width_offset;

    float3 normal_raw = cross(tangent, right);
    float normal_len = length(normal_raw);
    float3 normal = (normal_len > 0.001) ? (normal_raw / normal_len) : facing;

    uint outAddr = (outputVertexOffset + global_vert) * 24;
    g_Output.Store3(outAddr, asuint(world_pos));
    g_Output.Store(outAddr + 12, pack_normal(normal));
    g_Output.Store2(outAddr + 16, asuint(uv));
}
