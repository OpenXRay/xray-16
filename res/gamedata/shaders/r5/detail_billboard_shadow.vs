// detail_billboard_shadow.vs — CoP billboard grass caster for CSM (r__detail_gpu 0)
#define SM_6_0

cbuffer ShadowCascadeCB : register(b5)
{
    float4x4 cb_LightVP;
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
    O.position = mul(cb_LightVP, world_pos);
    O.tc = float2(v.u, v.v);
    return O;
}
