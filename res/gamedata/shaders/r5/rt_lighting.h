#ifndef RT_LIGHTING_H
#define RT_LIGHTING_H

struct RTEmissiveTriangle
{
    uint batchIndex;
    uint primitiveIndex;
    uint idLow;
    uint idHigh;
};

struct RTBatchTransform
{
    float4 row0;
    float4 row1;
    float4 row2;
};

StructuredBuffer<RTEmissiveTriangle> g_EmissiveTriangles : register(t24);
StructuredBuffer<RTBatchTransform> g_RTBatchTransforms : register(t25);
StructuredBuffer<uint> g_EmissiveBatchOffsets : register(t26);
StructuredBuffer<float> g_EnvironmentCDF : register(t27);

float3 RTTraceVisibility(RTSceneParams scene, float3 origin, float3 direction, float maxDistance,
    float coneWidth, float coneSpread, out bool exhausted);

float RTPowerHeuristic(float pdf, float otherPdf)
{
    if (!(pdf > 0.0))
        return 0.0;
    float scale = max(pdf, otherPdf);
    float a = pdf / scale;
    float b = otherPdf / scale;
    return a * a / (a * a + b * b);
}

float3 RTRotateY(float3 direction, float angle)
{
    float s, c;
    sincos(angle, s, c);
    return float3(c * direction.x + s * direction.z, direction.y, c * direction.z - s * direction.x);
}

float3 SampleRTSky(RTSceneParams scene, float3 direction)
{
    float3 localDirection = RTRotateY(direction, -scene.skyRotation);
    float3 sky0 = g_Sky0.SampleLevel(smp_rtlinear, localDirection, 0).rgb;
    float3 sky1 = g_Sky1.SampleLevel(smp_rtlinear, localDirection, 0).rgb;
    return lerp(sky0, sky1, scene.skyWeight);
}

float RTEnvironmentCellArea(uint row)
{
    return (2.0 * PI / 32.0) * (cos(PI * float(row) / 16.0) - cos(PI * float(row + 1u) / 16.0));
}

float RTEnvironmentPdf(RTSceneParams scene, float3 direction)
{
    float total = g_EnvironmentCDF[512];
    if (!(total > 0.0))
        return 1.0 / (4.0 * PI);
    float3 localDirection = RTRotateY(direction, -scene.skyRotation);
    float phi = atan2(localDirection.z, localDirection.x);
    if (phi < 0.0)
        phi += 2.0 * PI;
    uint column = min(uint(phi * (32.0 / (2.0 * PI))), 31u);
    uint row = min(uint(acos(clamp(localDirection.y, -1.0, 1.0)) * (16.0 / PI)), 15u);
    uint index = row * 32u + column;
    float previous = index > 0u ? g_EnvironmentCDF[index - 1u] : 0.0;
    float probability = max(g_EnvironmentCDF[index] - previous, 0.0) / total;
    return 0.95 * probability / RTEnvironmentCellArea(row) + 0.05 / (4.0 * PI);
}

float3 RTSampleEnvironment(RTSceneParams scene, inout uint rng, out float pdf)
{
    float total = g_EnvironmentCDF[512];
    float3 direction;
    if (!(total > 0.0) || rand_float(rng) < 0.05)
    {
        float phi = 2.0 * PI * rand_float(rng);
        float y = 1.0 - 2.0 * rand_float(rng);
        float r = sqrt(max(0.0, 1.0 - y * y));
        direction = float3(r * cos(phi), y, r * sin(phi));
    }
    else
    {
        float target = min(rand_float(rng) * total, asfloat(asuint(total) - 1u));
        uint first = 0u;
        uint last = 511u;
        while (first < last)
        {
            uint middle = (first + last) / 2u;
            if (g_EnvironmentCDF[middle] <= target)
                first = middle + 1u;
            else
                last = middle;
        }
        uint row = first / 32u;
        uint column = first % 32u;
        float phi = (float(column) + rand_float(rng)) * (2.0 * PI / 32.0);
        float y = lerp(cos(PI * float(row) / 16.0), cos(PI * float(row + 1u) / 16.0), rand_float(rng));
        float r = sqrt(max(0.0, 1.0 - y * y));
        direction = RTRotateY(float3(r * cos(phi), y, r * sin(phi)), scene.skyRotation);
    }
    pdf = RTEnvironmentPdf(scene, direction);
    return direction;
}

float RTSunSolidAngle(RTSceneParams scene)
{
    float s = sin(scene.sunAngularRadius * 0.5);
    return 4.0 * PI * s * s;
}

float3 RTSampleSun(RTSceneParams scene, inout uint rng)
{
    if (!(scene.sunAngularRadius > 0.0))
        return scene.sunDir;
    float halfChordSquared = rand_float(rng) * (RTSunSolidAngle(scene) / (4.0 * PI));
    float y = 1.0 - 2.0 * halfChordSquared;
    float r = 2.0 * sqrt(max(0.0, halfChordSquared * (1.0 - halfChordSquared)));
    float phi = 2.0 * PI * rand_float(rng);
    float3 up = abs(scene.sunDir.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 T = normalize(cross(up, scene.sunDir));
    float3 B = cross(scene.sunDir, T);
    return RTSafeNormalize(T * (r * cos(phi)) + B * (r * sin(phi)) + scene.sunDir * y, scene.sunDir);
}

float3 RTMissRadiance(RTSceneParams scene, float3 direction, float bsdfPdf, bool delta)
{
    float environmentWeight = delta ? 1.0 : RTPowerHeuristic(bsdfPdf, RTEnvironmentPdf(scene, direction));
    float3 radiance = SampleRTSky(scene, direction) * environmentWeight;
    float solidAngle = RTSunSolidAngle(scene);
    float3 sunOffset = direction - scene.sunDir;
    if (solidAngle > 0.0 && dot(sunOffset, sunOffset) <= solidAngle / PI)
    {
        float sunWeight = delta ? 1.0 : RTPowerHeuristic(bsdfPdf, 1.0 / solidAngle);
        radiance += scene.sunColor * (sunWeight / solidAngle);
    }
    return radiance;
}

float3x4 RTEmitterTransform(uint batchIndex)
{
    RTBatchTransform transform = g_RTBatchTransforms[batchIndex];
    return float3x4(transform.row0, transform.row1, transform.row2);
}

float RTEmissivePdfFromArea(RTSceneParams scene, float3 areaNormal, float3 previousPosition, float3 position)
{
    float twiceArea = length(areaNormal);
    float3 toEmitter = position - previousPosition;
    float distanceSquared = dot(toEmitter, toEmitter);
    if (scene.emissiveCount == 0u || !(twiceArea > 0.0) || !(distanceSquared > 0.0))
        return 0.0;
    float cosine = abs(dot(areaNormal / twiceArea, toEmitter * rsqrt(distanceSquared)));
    if (!(cosine > 0.0))
        return 0.0;
    return 2.0 * distanceSquared / (float(scene.emissiveCount) * twiceArea * cosine);
}

float RTEmissivePdf(RTSceneParams scene, uint batchIndex, RTBatchInfo info, uint primitiveIndex,
    float3 previousPosition, float3 position)
{
    if (scene.emissiveCount == 0u || g_EmissiveBatchOffsets[batchIndex] == 0xFFFFFFFFu)
        return 0.0;
    RTTriangleVertex v0, v1, v2;
    RTLoadWorldTriangle(scene, batchIndex, info, primitiveIndex, RTEmitterTransform(batchIndex), v0, v1, v2);
    return RTEmissivePdfFromArea(scene, cross(v1.position - v0.position, v2.position - v0.position),
        previousPosition, position);
}

float RTEmissionWeightFromArea(RTSceneParams scene, uint batchIndex, float3 areaNormal,
    float3 previousPosition, float3 position, float bsdfPdf, bool delta)
{
    if (delta)
        return 1.0;
    if (scene.emissiveCount == 0u || g_EmissiveBatchOffsets[batchIndex] == 0xFFFFFFFFu)
        return 1.0;
    return RTPowerHeuristic(bsdfPdf, RTEmissivePdfFromArea(scene, areaNormal, previousPosition, position));
}

float RTEmissionWeight(RTSceneParams scene, uint batchIndex, RTBatchInfo info, uint primitiveIndex,
    float3 previousPosition, float3 position, float bsdfPdf, bool delta)
{
    if (delta)
        return 1.0;
    return RTPowerHeuristic(bsdfPdf, RTEmissivePdf(scene, batchIndex, info, primitiveIndex, previousPosition, position));
}

float RTWaterFresnel(float3 normal, float3 V)
{
    float cosine = saturate(abs(dot(normal, V)));
    return 0.02037 + 0.97963 * pow(1.0 - cosine, 5.0);
}

float3 RTWaterTransmission()
{
    return float3(0.7, 0.85, 0.8);
}

RTBSDFSample RTSampleSurface(RTHitSurface hit, RTHitGeometry geometry, float3 V,
    uint diffuseMode, inout uint rng, out bool passthrough)
{
    passthrough = false;
    if ((hit.flags & MAT_FLAG_WATER) == 0u)
        return RTSampleBSDF(hit.surface, V, diffuseMode, rng);
    RTBSDFSample result = (RTBSDFSample)0;
    float3 normal = hit.surface.N;
    if (dot(normal, V) <= 0.0)
        normal = geometry.geoNormal;
    result.delta = true;
    result.pdf = 1.0;
    result.valid = true;
    if (rand_float(rng) < RTWaterFresnel(normal, V))
    {
        result.direction = reflect(-V, normal);
        result.weight = 1.0;
        result.specularWeight = 1.0;
        result.valid = dot(result.direction, geometry.geoNormal) > 0.0;
    }
    else
    {
        result.direction = -V;
        result.weight = RTWaterTransmission();
        result.specularWeight = result.weight;
        passthrough = true;
    }
    return result;
}

struct RTDirectTerms
{
    float3 diffuse;
    float3 specular;
    bool invalid;
};

struct RTLightCandidate
{
    float3 diffuse;
    float3 specular;
    float3 origin;
    float3 direction;
    float distance;
    bool valid;
};

RTLightCandidate RTEvaluateLightCandidate(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float3 L, float3 radiance, float pdf, float distance, bool delta, bool continuation)
{
    RTLightCandidate candidate = (RTLightCandidate)0;
    if (!(pdf > 0.0) || !any(radiance > 0.0))
        return candidate;
    if (surface.shadingClass != SHADING_CLASS_FOLIAGE && dot(L, geoNormal) <= 0.0)
        return candidate;
    RTBSDFTerms terms = RTEvaluateBSDFTerms(surface, V, L, radiance, scene.diffuseMode);
    if (!any(terms.bsdf > 0.0) && !any(terms.transmission > 0.0))
        return candidate;
    float side = dot(L, geoNormal) >= 0.0 ? 1.0 : -1.0;
    candidate.origin = position + geoNormal * (RT_RAY_ORIGIN_OFFSET * side);
    candidate.direction = L;
    candidate.distance = scene.rayDistance;
    if (distance < scene.rayDistance)
    {
        float3 toLight = position + L * distance - candidate.origin;
        float shadowDistance = length(toLight);
        candidate.direction = toLight / max(shadowDistance, 1e-20);
        candidate.distance = max(0.0, shadowDistance - RT_RAY_ORIGIN_OFFSET);
    }
    float weight = delta || !continuation ? 1.0 : RTPowerHeuristic(pdf, RTBSDFPdf(surface, V, L));
    float scale = weight / pdf;
    candidate.diffuse = (terms.diffuse + terms.transmission) * scale;
    candidate.specular = terms.specular * scale;
    candidate.valid = true;
    return candidate;
}

void RTTraceLightCandidate(RTSceneParams scene, RTLightCandidate candidate, float scale, float coneWidth,
    float coneSpread, inout RTDirectTerms result)
{
    if (!candidate.valid)
        return;
    bool exhausted;
    float3 visibility = RTTraceVisibility(scene, candidate.origin, candidate.direction, candidate.distance,
        coneWidth, coneSpread, exhausted);
    result.invalid = result.invalid || exhausted;
    result.diffuse += candidate.diffuse * (visibility * scale);
    result.specular += candidate.specular * (visibility * scale);
}

struct RTLightReservoir
{
    RTLightCandidate selected;
    float weightSum;
    float selectedTarget;
};

RTLightReservoir RTLightReservoirBegin()
{
    RTLightReservoir reservoir = (RTLightReservoir)0;
    return reservoir;
}

void RTLightReservoirAdd(inout RTLightReservoir reservoir, RTLightCandidate candidate, inout uint rng)
{
    if (!candidate.valid)
        return;
    float target = Luminance(candidate.diffuse + candidate.specular);
    if (!(target > 0.0) || !isfinite(target))
        return;
    reservoir.weightSum += target;
    if (rand_float(rng) * reservoir.weightSum < target)
    {
        reservoir.selected = candidate;
        reservoir.selectedTarget = target;
    }
}

void RTLightReservoirResolve(RTSceneParams scene, RTLightReservoir reservoir, float coneWidth, float coneSpread,
    inout RTDirectTerms result)
{
    if (!(reservoir.weightSum > 0.0) || !(reservoir.selectedTarget > 0.0))
        return;
    RTTraceLightCandidate(scene, reservoir.selected, reservoir.weightSum / reservoir.selectedTarget,
        coneWidth, coneSpread, result);
}

void RTAddLight(RTSceneParams scene, MaterialSurface surface, float3 position, float3 geoNormal,
    float3 V, float3 L, float3 radiance, float pdf, float distance, bool delta,
    bool continuation, float coneWidth, float coneSpread, inout RTDirectTerms result)
{
    RTTraceLightCandidate(scene, RTEvaluateLightCandidate(scene, surface, position, geoNormal, V, L, radiance,
        pdf, distance, delta, continuation), 1.0, coneWidth, coneSpread, result);
}

RTLightCandidate RTSunCandidate(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, bool continuation, inout uint rng)
{
    float3 sunDirection = RTSampleSun(scene, rng);
    float solidAngle = RTSunSolidAngle(scene);
    bool deltaSun = !(solidAngle > 0.0);
    float sunPdf = deltaSun ? 1.0 : 1.0 / solidAngle;
    return RTEvaluateLightCandidate(scene, surface, position, geoNormal, V, sunDirection, scene.sunColor * sunPdf,
        sunPdf, scene.rayDistance, deltaSun, continuation);
}

void RTDirectLightingSun(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float coneWidth, float coneSpread, bool continuation,
    inout uint rng, inout RTDirectTerms result)
{
    RTTraceLightCandidate(scene, RTSunCandidate(scene, surface, position, geoNormal, V, continuation, rng), 1.0,
        coneWidth, coneSpread, result);
}

struct RTLightList
{
    uint offset;
    uint count;
    bool indexed;
};

RTLightList RTResolveLightList(RTSceneParams scene, float3 position, bool primary)
{
    RTLightList list;
    list.offset = 0u;
    list.count = scene.lightCount;
    list.indexed = false;
    if (!primary || scene.clusterLights == 0u || list.count == 0u || !(cluster_params.w > 0.0))
        return list;
    float linearDepth = mul(m_V, float4(position, 1.0)).z;
    if (!(linearDepth > 0.0) || linearDepth > cluster_scales.y)
        return list;
    uint clusterIdx = GetClusterIndex(float2(scene.clusterPixel) + 0.5, linearDepth, cluster_params.xyz, cluster_scales);
    uint2 clusterData = g_ClusterGrid[clusterIdx];
    list.indexed = clusterData.x != 0xFFFFFFFFu;
    list.offset = list.indexed ? clusterData.x : 0u;
    list.count = clusterData.y;
    return list;
}

RTLightCandidate RTLocalLightCandidate(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, bool continuation, uint lightIndex)
{
    GPULightData light = g_LightData[lightIndex];
    float3 L;
    float distance;
    float attenuation = PunctualLightAttenuation(light, position, L, distance);
    if (!(attenuation > 0.0) || distance > scene.rayDistance)
    {
        RTLightCandidate none = (RTLightCandidate)0;
        return none;
    }
    return RTEvaluateLightCandidate(scene, surface, position, geoNormal, V, L, light.colorAndRange.xyz * attenuation,
        1.0, distance, true, continuation);
}

void RTDirectLightingLocalLights(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float coneWidth, float coneSpread, bool continuation,
    inout RTDirectTerms result, bool primary = false)
{
    RTLightList list = RTResolveLightList(scene, position, primary);
    for (uint i = 0u; i < list.count; ++i)
    {
        uint lightIndex = list.indexed ? g_LightIndexList[list.offset + i] : i;
        RTTraceLightCandidate(scene, RTLocalLightCandidate(scene, surface, position, geoNormal, V, continuation,
            lightIndex), 1.0, coneWidth, coneSpread, result);
    }
}

RTLightCandidate RTEnvironmentCandidate(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, bool continuation, inout uint rng)
{
    float environmentPdf;
    float3 environmentDirection = RTSampleEnvironment(scene, rng, environmentPdf);
    return RTEvaluateLightCandidate(scene, surface, position, geoNormal, V, environmentDirection,
        SampleRTSky(scene, environmentDirection), environmentPdf, scene.rayDistance, false, continuation);
}

void RTDirectLightingEnvironment(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float coneWidth, float coneSpread, bool continuation,
    inout uint rng, inout RTDirectTerms result)
{
    RTTraceLightCandidate(scene, RTEnvironmentCandidate(scene, surface, position, geoNormal, V, continuation, rng),
        1.0, coneWidth, coneSpread, result);
}

RTLightCandidate RTEmissiveCandidate(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, bool continuation, inout uint rng)
{
    RTLightCandidate none = (RTLightCandidate)0;
    if (scene.emissiveCount == 0u)
        return none;
    uint index = min(uint(rand_float(rng) * float(scene.emissiveCount)), scene.emissiveCount - 1u);
    RTEmissiveTriangle emitter = g_EmissiveTriangles[index];
    RTBatchInfo info = g_BatchInfo[emitter.batchIndex];
    RTTriangleVertex v0, v1, v2;
    float3x4 transform = RTEmitterTransform(emitter.batchIndex);
    RTLoadWorldTriangle(scene, emitter.batchIndex, info, emitter.primitiveIndex, transform, v0, v1, v2);
    float root = sqrt(rand_float(rng));
    float2 barycentrics = float2(root * (1.0 - rand_float(rng)), 0.0);
    barycentrics.y = root - barycentrics.x;
    float3 lightPosition = v0.position * (1.0 - root) + v1.position * barycentrics.x + v2.position * barycentrics.y;
    float3 toLight = lightPosition - position;
    float distance = length(toLight);
    if (!(distance > 0.0) || distance > scene.rayDistance)
        return none;
    float3 L = toLight / distance;
    float3 areaNormal = cross(v1.position - v0.position, v2.position - v0.position);
    float pdf = RTEmissivePdfFromArea(scene, areaNormal, position, lightPosition);
    float2 uv = RTInterpolateUV(v0.uv, v1.uv, v2.uv, barycentrics);
    float3 emission = RTEvaluateEmitter(scene, emitter.batchIndex, info, uv, 0.0, 0.0, true);
    return RTEvaluateLightCandidate(scene, surface, position, geoNormal, V, L, emission, pdf, distance, false,
        continuation);
}

void RTDirectLightingEmissive(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float coneWidth, float coneSpread, bool continuation,
    inout uint rng, inout RTDirectTerms result)
{
    RTTraceLightCandidate(scene, RTEmissiveCandidate(scene, surface, position, geoNormal, V, continuation, rng),
        1.0, coneWidth, coneSpread, result);
}

void RTDirectLightingSanitize(inout RTDirectTerms result)
{
    if (!all(isfinite(result.diffuse)) || !all(isfinite(result.specular)))
    {
        result.diffuse = 0.0;
        result.specular = 0.0;
        result.invalid = true;
    }
}

RTDirectTerms RTDirectLightingResampled(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float coneWidth, float coneSpread, bool continuation, inout uint rng,
    bool primary)
{
    RTDirectTerms result = (RTDirectTerms)0;
    RTLightReservoir reservoir = RTLightReservoirBegin();
    RTLightCandidate sun = RTSunCandidate(scene, surface, position, geoNormal, V, continuation, rng);
    if (scene.lightRays >= 2u)
        RTTraceLightCandidate(scene, sun, 1.0, coneWidth, coneSpread, result);
    else
        RTLightReservoirAdd(reservoir, sun, rng);
    RTLightReservoirAdd(reservoir, RTEnvironmentCandidate(scene, surface, position, geoNormal, V, continuation, rng), rng);
    RTLightReservoirAdd(reservoir, RTEmissiveCandidate(scene, surface, position, geoNormal, V, continuation, rng), rng);
    RTLightList list = RTResolveLightList(scene, position, primary);
    for (uint i = 0u; i < list.count; ++i)
    {
        uint lightIndex = list.indexed ? g_LightIndexList[list.offset + i] : i;
        RTLightReservoirAdd(reservoir, RTLocalLightCandidate(scene, surface, position, geoNormal, V, continuation,
            lightIndex), rng);
    }
    RTLightReservoirResolve(scene, reservoir, coneWidth, coneSpread, result);
    RTDirectLightingSanitize(result);
    return result;
}

RTDirectTerms RTDirectLightingTerms(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float coneWidth, float coneSpread, bool continuation, inout uint rng,
    bool primary = false)
{
    if (scene.lightRays != 0u)
        return RTDirectLightingResampled(scene, surface, position, geoNormal, V, coneWidth, coneSpread,
            continuation, rng, primary);
    RTDirectTerms result = (RTDirectTerms)0;
    RTDirectLightingSun(scene, surface, position, geoNormal, V, coneWidth, coneSpread, continuation,
        rng, result);
    RTDirectLightingLocalLights(scene, surface, position, geoNormal, V, coneWidth, coneSpread,
        continuation, result, primary);
    RTDirectLightingEnvironment(scene, surface, position, geoNormal, V, coneWidth, coneSpread,
        continuation, rng, result);
    RTDirectLightingEmissive(scene, surface, position, geoNormal, V, coneWidth, coneSpread,
        continuation, rng, result);
    RTDirectLightingSanitize(result);
    return result;
}

#endif
