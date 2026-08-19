// detail_billboard_shadow.vs — CoP billboard grass caster for CSM (r__detail_gpu 0)
#define SM_6_0

cbuffer ShadowCascadeCB : register(b5)
{
    float4x4 cb_LightVP;
};

cbuffer GrassShadowCB : register(b6)
{
    float grass_blade_height;
    uint build_details_index;
    float wind_angle_deg;
    float wind_speed;
    float time;
    float wind_displacement;
    float2 grass_shadow_pad;
};

struct InstanceData
{
    float3 pos;
    uint packed;
};

struct DetailModelGPU
{
    float minScale;
    float maxScale;
    float flags;
    float geomExtentX;
    float geomExtentZ;
    float uv_min_x;
    float uv_min_y;
    float uv_max_x;
    float uv_max_y;
    uint pulledVertexBase;
    uint pulledIndexCount;
    float geomExtentY;
};

struct PulledVertex
{
    float px, py, pz;
    float u, v;
};

static const float PACK_MAX_SCALE = 4.0;
static const float TWO_PI = 6.28318530718;

StructuredBuffer<uint> visible_indices : register(t0);
StructuredBuffer<DetailModelGPU> detail_models : register(t1);
StructuredBuffer<PulledVertex> pulled_vertices : register(t2);
StructuredBuffer<InstanceData> all_instances : register(t3);
Texture3D<float> g_Perlin4D : register(t4);
SamplerState smp_linear : register(s0);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 tc : TEXCOORD0;
};

VS_OUTPUT main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    VS_OUTPUT O;
    O.position = float4(0, 0, 0, 1);
    O.tc = 0;

    uint src_idx = visible_indices[instance_id];
    InstanceData raw = all_instances[src_idx];

    uint object_id = raw.packed & 0x3F;
    float rotation = float((raw.packed >> 8) & 0x3FF) / 1023.0 * TWO_PI;
    float scale = float((raw.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;

    DetailModelGPU mdl = detail_models[object_id];
    if (vertex_id >= mdl.pulledIndexCount)
    {
        O.position = float4(asfloat(0x7FC00000), asfloat(0x7FC00000),
                        asfloat(0x7FC00000), asfloat(0x7FC00000));
        return O;
    }

    PulledVertex v = pulled_vertices[mdl.pulledVertexBase + vertex_id];
    float3 local_pos = float3(v.px, v.py, v.pz) * scale;

    float c = cos(rotation);
    float s = sin(rotation);
    float3 rotated;
    rotated.x = local_pos.x * c - local_pos.z * s;
    rotated.y = local_pos.y;
    rotated.z = local_pos.x * s + local_pos.z * c;

    float4 world_pos = float4(rotated + raw.pos, 1.0);
    float height_factor = saturate(v.py / max(mdl.geomExtentY, 0.01));
    float speed = max(wind_speed, 0.1);
    float wind_angle = wind_angle_deg * (3.14159265359 / 180.0);
    float2 global_wind_dir = float2(sin(wind_angle), cos(wind_angle));
    float2 dir_uv = world_pos.zx * (0.005 / speed) + time * (0.005 * speed);
    float wind_dir_noise = g_Perlin4D.SampleLevel(smp_linear, float3(dir_uv, 0), 0).r;
    float2 str_uv = world_pos.xz * (0.025 / speed) + time * 0.05;
    float wind_str_noise = g_Perlin4D.SampleLevel(smp_linear, float3(str_uv, 0), 0).r;
    float wind_strength = lerp(0.25, 1.0, wind_str_noise);
    wind_strength *= wind_strength * speed;
    float turbulence = (wind_dir_noise * 2.0 - 1.0) * 0.3;
    float2 perpendicular_dir = float2(-global_wind_dir.y, global_wind_dir.x);
    float2 wind_dir = normalize(global_wind_dir + perpendicular_dir * turbulence);
    float displacement = wind_strength * wind_displacement * height_factor;
    world_pos.xz += displacement * wind_dir;
    O.position = mul(cb_LightVP, world_pos);
    O.tc = float2(v.u, v.v);
    return O;
}
