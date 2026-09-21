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

float RTEmissivePdf(RTSceneParams scene, uint batchIndex, RTBatchInfo info, uint primitiveIndex,
    float3 previousPosition, float3 position)
{
    if (scene.emissiveCount == 0u || g_EmissiveBatchOffsets[batchIndex] == 0xFFFFFFFFu)
        return 0.0;
    RTTriangleVertex v0, v1, v2;
    RTLoadWorldTriangle(scene, batchIndex, info, primitiveIndex, RTEmitterTransform(batchIndex), v0, v1, v2);
    float3 areaNormal = cross(v1.position - v0.position, v2.position - v0.position);
    float twiceArea = length(areaNormal);
    float3 toEmitter = position - previousPosition;
    float distanceSquared = dot(toEmitter, toEmitter);
    if (!(twiceArea > 0.0) || !(distanceSquared > 0.0))
        return 0.0;
    float cosine = abs(dot(areaNormal / twiceArea, toEmitter * rsqrt(distanceSquared)));
    if (!(cosine > 0.0))
        return 0.0;
    return 2.0 * distanceSquared / (float(scene.emissiveCount) * twiceArea * cosine);
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

void RTAddLight(RTSceneParams scene, MaterialSurface surface, float3 position, float3 geoNormal,
    float3 V, float3 L, float3 radiance, float pdf, float distance, bool delta,
    bool continuation, float coneWidth, float coneSpread, inout RTDirectTerms result)
{
    if (!(pdf > 0.0) || !any(radiance > 0.0))
        return;
    if (surface.shadingClass != SHADING_CLASS_FOLIAGE && dot(L, geoNormal) <= 0.0)
        return;
    RTBSDFTerms terms = RTEvaluateBSDFTerms(surface, V, L, radiance, scene.diffuseMode);
    if (!any(terms.bsdf > 0.0) && !any(terms.transmission > 0.0))
        return;
    float side = dot(L, geoNormal) >= 0.0 ? 1.0 : -1.0;
    float3 origin = position + geoNormal * (RT_RAY_ORIGIN_OFFSET * side);
    float3 shadowDirection = L;
    float shadowDistance = scene.rayDistance;
    if (distance < scene.rayDistance)
    {
        float3 toLight = position + L * distance - origin;
        shadowDistance = length(toLight);
        shadowDirection = toLight / max(shadowDistance, 1e-20);
        shadowDistance = max(0.0, shadowDistance - RT_RAY_ORIGIN_OFFSET);
    }
    bool exhausted;
    float3 visibility = RTTraceVisibility(scene, origin, shadowDirection, shadowDistance,
        coneWidth, coneSpread, exhausted);
    result.invalid = result.invalid || exhausted;
    float weight = delta || !continuation ? 1.0 : RTPowerHeuristic(pdf, RTBSDFPdf(surface, V, L));
    float3 scale = visibility * (weight / pdf);
    result.diffuse += (terms.diffuse + terms.transmission) * scale;
    result.specular += terms.specular * scale;
}

RTDirectTerms RTDirectLightingTerms(RTSceneParams scene, MaterialSurface surface, float3 position,
    float3 geoNormal, float3 V, float coneWidth, float coneSpread, bool continuation, inout uint rng)
{
    RTDirectTerms result = (RTDirectTerms)0;
    float3 sunDirection = RTSampleSun(scene, rng);
    float solidAngle = RTSunSolidAngle(scene);
    bool deltaSun = !(solidAngle > 0.0);
    float sunPdf = deltaSun ? 1.0 : 1.0 / solidAngle;
    RTAddLight(scene, surface, position, geoNormal, V, sunDirection, scene.sunColor * sunPdf,
        sunPdf, scene.rayDistance, deltaSun, continuation, coneWidth, coneSpread, result);

    for (uint i = 0u; i < scene.lightCount; ++i)
    {
        GPULightData light = g_LightData[i];
        float3 L;
        float distance;
        float attenuation = PunctualLightAttenuation(light, position, L, distance);
        if (attenuation > 0.0 && distance <= scene.rayDistance)
            RTAddLight(scene, surface, position, geoNormal, V, L, light.colorAndRange.xyz * attenuation,
                1.0, distance, true, continuation, coneWidth, coneSpread, result);
    }

    float environmentPdf;
    float3 environmentDirection = RTSampleEnvironment(scene, rng, environmentPdf);
    RTAddLight(scene, surface, position, geoNormal, V, environmentDirection, SampleRTSky(scene, environmentDirection),
        environmentPdf, scene.rayDistance, false, continuation, coneWidth, coneSpread, result);

    if (scene.emissiveCount > 0u)
    {
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
        if (distance > 0.0 && distance <= scene.rayDistance)
        {
            float3 L = toLight / distance;
            float pdf = RTEmissivePdf(scene, emitter.batchIndex, info, emitter.primitiveIndex, position, lightPosition);
            RTSceneTrace lightTrace = (RTSceneTrace)0;
            lightTrace.batchIdx = emitter.batchIndex;
            lightTrace.info = info;
            lightTrace.primitiveIndex = emitter.primitiveIndex;
            lightTrace.barycentrics = barycentrics;
            lightTrace.objectToWorld = transform;
            lightTrace.t = distance;
            lightTrace.coneWidth = coneWidth;
            lightTrace.coneSpread = coneSpread;
            RTHitGeometry geometry = RTFetchHitGeometry(scene, lightTrace, L);
            float3 emission = RTEvaluateEmitter(scene, emitter.batchIndex, info, geometry.uv,
                geometry.uvDx, geometry.uvDy, true);
            RTAddLight(scene, surface, position, geoNormal, V, L, emission, pdf, distance, false,
                continuation, coneWidth, coneSpread, result);
        }
    }
    if (!all(isfinite(result.diffuse)) || !all(isfinite(result.specular)))
    {
        result.diffuse = 0.0;
        result.specular = 0.0;
        result.invalid = true;
    }
    return result;
}

#endif
