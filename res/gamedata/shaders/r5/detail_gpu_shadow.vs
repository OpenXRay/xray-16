// detail_gpu_shadow.vs — depth-only grass caster for CSM (LOD0/1)
// Instance packing must match detail_gpu.vs
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

struct v_blade_sdf
{
    float3 pos : POSITION;
    float2 tc : TEXCOORD;
    float t : COLOR0;
    float width_scale : COLOR1;
};

struct InstanceData
{
    float3 pos;
    uint packed;
};

StructuredBuffer<uint> visible_indices : register(t0);
StructuredBuffer<InstanceData> all_instances : register(t1);

struct VS_OUTPUT
{
    float4 position : SV_Position;
    float2 tc : TEXCOORD0;
};

static const float PACK_MAX_SCALE = 4.0;
static const float TWO_PI = 6.28318530718;

VS_OUTPUT main(v_blade_sdf I, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT O;
    uint idx = visible_indices[instanceID];
    InstanceData raw = all_instances[idx];

    float rotation = float((raw.packed >> 8) & 0x3FF) / 1023.0 * TWO_PI;
    float scale = float((raw.packed >> 18) & 0x3FF) / 1023.0 * PACK_MAX_SCALE;
    float blade_height = max(scale * grass_blade_height, 0.05);

    float3 facing = normalize(float3(sin(rotation), 0.0, cos(rotation)));
    float3 right = normalize(float3(facing.z, 0.0, -facing.x));

    float3 world = raw.pos;
    world += right * (I.pos.x * I.width_scale * scale * 0.5);
    world.y += I.t * blade_height;

    O.position = mul(cb_LightVP, float4(world, 1.0));
    O.tc = I.tc;
    return O;
}
