#define SM_6_0
#include "shared/common.h"
#include "gpu_particle_types.h"

StructuredBuffer<GpuPapiParticle> g_Particles : register(t0);
StructuredBuffer<GpuPapiEmitter> g_Emitters : register(t2);
StructuredBuffer<GpuPapiProgram> g_Programs : register(t3);
StructuredBuffer<uint> g_ParticleIndices : register(t4);
StructuredBuffer<uint> g_BucketOffsets : register(t5);

cbuffer GpuParticleDrawParams : register(b5)
{
    float4x4 g_HudWarp;
    float4 g_CameraTop;
    float4 g_CameraRight;
};

struct VS_OUTPUT
{
    float4 hpos : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    float3 normal : TEXCOORD3;
    nointerpolation uint materialID : TEXCOORD4;
    nointerpolation uint drawBucket : TEXCOORD5;
};

float3 GpuParticleTransformDirection(GpuPapiEmitter emitter, float3 direction)
{
    return emitter.basisX.xyz * direction.x + emitter.basisY.xyz * direction.y + emitter.basisZ.xyz * direction.z;
}

VS_OUTPUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    uint drawBucket = vertexID / 6u;
    GpuPapiParticle particle = g_Particles[g_ParticleIndices[g_BucketOffsets[drawBucket] + instanceID]];
    GpuPapiEmitter emitter = g_Emitters[particle.emitter];
    GpuPapiProgram program = g_Programs[particle.program];
    bool localSpace = (emitter.flags & GPU_PAPI_LOCAL) != 0u;
    float3 position = particle.position;
    if (localSpace)
        position = GpuParticleTransformDirection(emitter, position) + emitter.origin.xyz;

    float2 radius = particle.size.xy * 0.5;
    float speed = 0.0;
    if ((program.flags & ((1u << 18u) | (1u << 15u))) != 0u)
        speed = length(particle.velocity);
    if ((program.flags & (1u << 18u)) != 0u)
        radius += speed * program.velocityScale.xy;

    float3 top = g_CameraTop.xyz;
    float3 right = g_CameraRight.xyz;
    if ((program.flags & (1u << 15u)) != 0u)
    {
        if (speed < 0.0000001 && (program.flags & (1u << 20u)) != 0u)
        {
            float3 sine;
            float3 cosine;
            sincos(program.alignRotation, sine, cosine);
            top = float3(-cosine.x * sine.y, sine.x, cosine.x * cosine.y);
            right = float3(cosine.y * cosine.z - sine.x * sine.y * sine.z,
                -cosine.x * sine.z, sine.x * cosine.y * sine.z + sine.y * cosine.z);
            if (localSpace)
            {
                top = GpuParticleTransformDirection(emitter, top);
                right = GpuParticleTransformDirection(emitter, right);
            }
        }
        else if (speed >= 0.0000001 && (program.flags & (1u << 21u)) != 0u)
        {
            float3 direction = particle.velocity / speed;
            float3 up = abs(direction.y) > 0.99 ? float3(0, 0, 1) : float3(0, 1, 0);
            right = normalize(cross(up, direction));
            top = normalize(cross(direction, right));
            if (localSpace)
            {
                top = GpuParticleTransformDirection(emitter, top);
                right = GpuParticleTransformDirection(emitter, right);
            }
        }
        else
        {
            if (speed >= 0.0000001)
                top = particle.velocity / speed;
            else
            {
                float2 sine;
                float2 cosine;
                sincos(program.alignRotation.xy, sine, cosine);
                top = float3(cosine.x * sine.y, -sine.x, cosine.x * cosine.y);
            }
            if (localSpace)
                top = GpuParticleTransformDirection(emitter, top);
            right = cross(top, camera_direction.xyz);
            float magnitude = length(right);
            right = magnitude > 0.0000001 ? right / magnitude : g_CameraRight.xyz;
        }
    }

    float sine;
    float cosine;
    sincos(particle.rotation.x, sine, cosine);
    float3 horizontal = radius.x * (top * sine + right * cosine);
    float3 vertical = radius.y * (top * cosine - right * sine);
    static const uint corners[6] = { 0u, 1u, 2u, 2u, 1u, 3u };
    uint corner = corners[vertexID % 6u];
    float2 uv = float2((corner & 2u) != 0u ? 1.0 : 0.0, (corner & 1u) != 0u ? 0.0 : 1.0);
    position += horizontal * (uv.x * 2.0 - 1.0) + vertical * (1.0 - uv.y * 2.0);
    if ((emitter.flags & GPU_PAPI_HUD) != 0u)
        position = mul(g_HudWarp, float4(position, 1.0)).xyz;

    if ((program.flags & (1u << 10u)) != 0u)
    {
        uint frame = uint(floor(float(particle.frame) / 255.0));
        uint columns = program.frameDimX;
        uv = (float2(frame % columns, frame / columns) + uv) * program.frameSize;
    }

    VS_OUTPUT output;
    output.hpos = mul(m_VP, float4(position, 1.0));
    if ((emitter.flags & GPU_PAPI_HUD) != 0u)
        output.hpos.z = 0.9 * output.hpos.w + 0.1 * output.hpos.z;
    output.texcoord = uv;
    output.color = float4((particle.color >> 16u) & 255u, (particle.color >> 8u) & 255u,
        particle.color & 255u, (particle.color >> 24u) & 255u) / 255.0;
    output.worldPos = position;
    output.normal = normalize(eye_position - position);
    output.materialID = program.materialID;
    output.drawBucket = drawBucket;
    return output;
}
