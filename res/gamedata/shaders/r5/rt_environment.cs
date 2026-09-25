TextureCube<float4> g_Sky;
SamplerState smp_rtlinear;
RWStructuredBuffer<float> g_EnvironmentCDF;

#define ENV_LON_CELL_COUNT 32
#define ENV_LAT_CELL_COUNT 16
#define ENV_CELL_COUNT 512

static const float ENV_PI = 3.14159265359f;
static const float ENV_TWO_PI = 6.28318530718f;

groupshared float s_envWeights[ENV_CELL_COUNT];

float EnvSkyLuminance(float3 rgb)
{
    return dot(max(rgb, 0.0), float3(0.2126, 0.7152, 0.0722));
}

[numthreads(ENV_LON_CELL_COUNT, ENV_LAT_CELL_COUNT, 1)]
void main(uint3 gtid : SV_GroupThreadID)
{
    uint col = gtid.x;
    uint row = gtid.y;
    uint index = row * ENV_LON_CELL_COUNT + col;

    float dPhi = ENV_TWO_PI / ENV_LON_CELL_COUNT;
    float dTheta = ENV_PI / ENV_LAT_CELL_COUNT;
    float theta0 = row * dTheta;
    float theta1 = theta0 + dTheta;
    float theta = theta0 + 0.5 * dTheta;
    float phi = (col + 0.5) * dPhi;

    float sinTheta = sin(theta);
    float3 dir = float3(sinTheta * cos(phi), cos(theta), sinTheta * sin(phi));

    uint skySize, skyHeight, skyLevels;
    g_Sky.GetDimensions(0, skySize, skyHeight, skyLevels);
    float cellLod = clamp(log2(float(skySize) / float(ENV_LAT_CELL_COUNT / 2)), 0.0, float(skyLevels - 1u));
    float3 radiance = g_Sky.SampleLevel(smp_rtlinear, dir, cellLod).rgb;

    float solidAngle = dPhi * (cos(theta0) - cos(theta1));
    float weight = EnvSkyLuminance(radiance) * max(solidAngle, 0.0);
    if (!isfinite(weight) || weight < 0.0)
        weight = 0.0;

    s_envWeights[index] = weight;
    GroupMemoryBarrierWithGroupSync();

    for (uint offset = 1; offset < ENV_CELL_COUNT; offset <<= 1)
    {
        float add = index >= offset ? s_envWeights[index - offset] : 0.0;
        GroupMemoryBarrierWithGroupSync();
        s_envWeights[index] += add;
        GroupMemoryBarrierWithGroupSync();
    }

    g_EnvironmentCDF[index] = s_envWeights[index];
    if (index == 0)
        g_EnvironmentCDF[ENV_CELL_COUNT] = s_envWeights[ENV_CELL_COUNT - 1];
}
