#ifndef GPU_PARTICLE_ACTIONS_H
#define GPU_PARTICLE_ACTIONS_H

#include "gpu_particle_domains.h"
#include "gpu_particle_noise.h"

float4 GpuPapiInitialActionState(uint actionDataIndex)
{
    return g_Actions[actionDataIndex].p3;
}

bool GpuPapiInRadius(float squared, float radius)
{
    float maximum = radius * radius;
    return maximum >= 1.0e16 || squared < maximum;
}

bool GpuPapiPlaneHit(GpuPapiDomain domain, float3 position, float3 velocity, float interval,
    out float time, out float3 offset)
{
    float planeD = domain.type == 9u ? domain.radii.z : domain.radii.x;
    float oldDistance = dot(position, domain.p2.xyz) + planeD;
    float newDistance = dot(position + velocity * interval, domain.p2.xyz) + planeD;
    time = 0.0;
    offset = 0.0;
    if (oldDistance * newDistance >= 0.0)
        return false;
    time = -oldDistance / dot(domain.p2.xyz, velocity);
    offset = position + velocity * time - domain.p1.xyz;
    if (domain.type == 9u)
    {
        float squared = dot(offset, offset);
        return squared <= domain.radii.x * domain.radii.x && squared >= domain.radii.y * domain.radii.y;
    }
    if (domain.type == 2u || domain.type == 10u)
    {
        float3 u = domain.u.xyz;
        float3 v = domain.v.xyz;
        float3 w = cross(u, v);
        float inverseDet = 1.0 / (w.z * u.x * v.y - w.z * u.y * v.x - u.z * w.x * v.y -
            u.x * v.z * w.y + v.z * w.x * u.y + u.z * v.x * w.y);
        float upos = dot(offset, cross(v, w) * inverseDet);
        float vpos = dot(offset, cross(w, u) * inverseDet);
        if (upos < 0.0 || vpos < 0.0)
            return false;
        return domain.type == 2u ? upos + vpos <= 1.0 : upos <= 1.0 && vpos <= 1.0;
    }
    return domain.type == 3u;
}

float3 GpuPapiAvoid(GpuPapiDomain domain, float3 position, float3 velocity, float4 parameters, float dt)
{
    float3 safety;
    float distance;
    float speed = length(velocity);
    float3 direction = velocity / speed;
    if (domain.type == 3u)
    {
        distance = dot(position, domain.p2.xyz) + domain.radii.x;
        if (parameters.x < 1.0e16 && distance >= parameters.x)
            return velocity;
        safety = domain.p2.xyz;
    }
    else if (domain.type == 5u)
    {
        float3 toCenter = domain.p1.xyz - position;
        float projection = dot(toCenter, direction);
        float discriminant = domain.radii.x * domain.radii.x - dot(toCenter, toCenter) + projection * projection;
        if (discriminant < 0.0)
            return velocity;
        distance = projection - sqrt(discriminant);
        if (distance < 0.0 || distance > speed * parameters.x)
            return velocity;
        safety = cross(direction, GpuPapiSafeNormal(cross(direction, toCenter)));
    }
    else
    {
        float3 offset;
        if (!GpuPapiPlaneHit(domain, position, velocity, dt * parameters.x, distance, offset))
            return velocity;
        if (domain.type == 9u)
            safety = GpuPapiSafeNormal(offset);
        else
        {
            float3 un = domain.u.xyz / domain.radii.z;
            float3 vn = domain.v.xyz / domain.radii.w;
            float3 uofs = un * dot(un, offset) - offset;
            float3 vofs = vn * dot(vn, offset) - offset;
            float3 foffset = domain.type == 2u ? offset - domain.u.xyz : domain.u.xyz + domain.v.xyz - offset;
            float3 fn = domain.type == 2u ? GpuPapiSafeNormal(domain.v.xyz - domain.u.xyz) : un;
            float3 fofs = fn * dot(fn, foffset) - foffset;
            float ud = dot(uofs, uofs);
            float vd = dot(vofs, vofs);
            float fd = dot(fofs, fofs);
            safety = ud <= vd && ud <= fd ? uofs : (vd <= fd ? vofs : fofs);
            safety = GpuPapiSafeNormal(safety);
        }
    }
    float3 result = safety * (parameters.y * dt / (distance * distance + parameters.z)) + direction;
    return result * (speed / length(result));
}

float3 GpuPapiBounce(GpuPapiDomain domain, float3 position, float3 velocity, float4 parameters,
    float dt, inout uint rng)
{
    float3 normal = domain.p2.xyz;
    bool inside = false;
    if (domain.type == 5u)
    {
        if (!GpuPapiWithin(domain, position + velocity * dt, rng))
            return velocity;
        inside = GpuPapiWithin(domain, position, rng);
        normal = GpuPapiSafeNormal(position - domain.p1.xyz);
    }
    else
    {
        float time;
        float3 offset;
        if (!GpuPapiPlaneHit(domain, position, velocity, dt, time, offset))
            return velocity;
    }
    float normalMagnitude = dot(velocity, normal);
    float3 normalVelocity = normal * normalMagnitude;
    float3 tangentVelocity = velocity - normalVelocity;
    if (inside)
        return normalMagnitude < 0.0 ? tangentVelocity - normalVelocity : velocity;
    return tangentVelocity * (dot(tangentVelocity, tangentVelocity) <= parameters.z ? 1.0 : parameters.x) -
        normalVelocity * parameters.y;
}

void GpuPapiExecuteAction(inout GpuPapiEmitter emitter, uint actionDataIndex, uint actionIndex,
    float dt, inout float killOldTime)
{
    uint type = g_Actions[actionDataIndex].type;
    if (type == 21u && ((emitter.flags & GPU_PAPI_PLAYING) == 0u || (emitter.flags & GPU_PAPI_STOPPING) != 0u))
        return;
    uint flags = g_Actions[actionDataIndex].flags;
    float4 p0 = g_Actions[actionDataIndex].p0;
    float4 p1 = g_Actions[actionDataIndex].p1;
    uint stateIndex = emitter.actionStateFirst + actionIndex;
    float4 state = 0.0;
    if (type == 5u || type == 18u || type == 30u)
        state = g_ActionState[stateIndex];
    GpuPapiDomain domain = (GpuPapiDomain)0;
    if (type == 0u || type == 1u || type == 19u || type == 21u)
        domain = GpuPapiTransformDomain(g_Actions[actionDataIndex].domains[0], emitter, flags, false);
    if (type == 9u || type == 15u || type == 16u || type == 17u || type == 20u)
        domain = GpuPapiTransformDomain(g_Actions[actionDataIndex].domains[0], emitter, flags, true);
    if (type == 21u)
    {
        float expected = p0.y * dt;
        float integral = floor(expected);
        if (GpuPapiRandom(emitter.rng) < expected - integral)
            integral += 1.0;
        uint rate = uint(max(0.0, min(integral, float(emitter.capacity - emitter.count))));
        GpuPapiDomain velocityDomain = GpuPapiTransformDomain(g_Actions[actionDataIndex].domains[1], emitter, flags, true);
        GpuPapiDomain rotationDomain = g_Actions[actionDataIndex].domains[2];
        GpuPapiDomain sizeDomain = g_Actions[actionDataIndex].domains[3];
        GpuPapiDomain colorDomain = g_Actions[actionDataIndex].domains[4];
        float3 parentVelocity = p1.xyz;
        if ((emitter.flags & GPU_PAPI_PARENT_SET) != 0u)
            parentVelocity = emitter.parentVelocity * p1.w;
        for (uint ordinal = 0u; ordinal < rate; ++ordinal)
        {
            GpuPapiParticle particle = (GpuPapiParticle)0;
            particle.position = GpuPapiGenerate(domain, emitter.rng);
            particle.size = GpuPapiGenerate(sizeDomain, emitter.rng);
            if ((flags & (1u << 29u)) != 0u)
                particle.size = particle.size.xxx;
            float3 rotation = GpuPapiGenerate(rotationDomain, emitter.rng);
            particle.rotation.x = rotation.x;
            particle.velocity = GpuPapiGenerate(velocityDomain, emitter.rng) + parentVelocity;
            float3 color = GpuPapiGenerate(colorDomain, emitter.rng);
            particle.color = GpuPapiPackColor(float4(color, p0.x));
            particle.age = p0.z + GpuPapiNormalRandom(p0.w, emitter.rng);
            particle.positionB = (flags & (1u << 31u)) != 0u ? particle.position : 0.0;
            particle.child = GPU_PAPI_INVALID;
            if (!GpuAddParticle(emitter, particle))
                break;
        }
        return;
    }
    if (type == 10u || type == 19u || type == 20u)
    {
        if (type == 10u)
            killOldTime = p0.x;
        for (uint ordinal = emitter.count; ordinal > 0u; --ordinal)
        {
            GpuPapiParticle particle = g_Particles[GpuParticleIndex(emitter, ordinal - 1u)];
            bool remove;
            if (type == 10u)
                remove = (particle.age < p0.x) == (p0.y != 0.0);
            else
                remove = GpuPapiWithin(domain, type == 19u ? particle.position : particle.velocity, emitter.rng) ==
                    (p0.x != 0.0);
            if (remove)
                GpuRemoveParticle(emitter, ordinal - 1u);
        }
        return;
    }
    uint octaves = 0u;
    bool floorCoordinates = false;
    if (type == 30u)
    {
        state.x += dt;
        octaves = g_Actions[actionDataIndex].pad0;
        floorCoordinates = (g_Actions[actionDataIndex].pad1 & 1u) != 0u;
    }
    float3 center = 0.0;
    if (type == 5u || type == 9u || type == 13u || type == 14u || type == 29u || type == 31u)
        center = GpuPapiPoint(emitter, flags, p0.xyz);
    float3 axis = 0.0;
    float maximumRadius = 0.0;
    if (type == 13u || type == 29u)
    {
        axis = GpuPapiDirection(emitter, flags, p1.xyz);
        maximumRadius = g_Actions[actionDataIndex].p2.x;
    }
    float3 targetVelocity = 0.0;
    if (type == 27u || type == 28u)
        targetVelocity = GpuPapiDirection(emitter, flags, p0.xyz);
    for (uint ordinal = 0u; ordinal < emitter.count; ++ordinal)
    {
        uint index = GpuParticleIndex(emitter, ordinal);
        GpuPapiParticle particle = g_Particles[index];
        switch (type)
        {
        case 0u:
            particle.velocity = GpuPapiAvoid(domain, particle.position, particle.velocity, p0, dt);
            break;
        case 1u:
            particle.velocity = GpuPapiBounce(domain, particle.position, particle.velocity, p0, dt, emitter.rng);
            break;
        case 3u:
            if (p0.x != 0.0)
                particle.positionB = particle.position;
            break;
        case 4u:
        {
            float squared = dot(particle.velocity, particle.velocity);
            if (squared >= p0.w && squared <= p1.x)
                particle.velocity *= 1.0 - (1.0 - p0.xyz) * dt;
            break;
        }
        case 5u:
        {
            float3 direction = particle.position - center;
            float squared = dot(direction, direction);
            float distance = sqrt(squared);
            float waveDistance = p0.w * state.x - distance;
            float inverseSigma = 1.0 / p1.y;
            float exponent = -0.5 * (inverseSigma * inverseSigma);
            float gaussian = exp(waveDistance * waveDistance * exponent) * (0.3989422804014327 * inverseSigma);
            particle.velocity += direction * (gaussian * p1.x * dt / ((distance + 0.00001) * (squared + p1.z)));
            break;
        }
        case 6u:
            if (ordinal + 1u < emitter.count)
            {
                float3 direction = g_Particles[GpuParticleIndex(emitter, ordinal + 1u)].position - particle.position;
                float squared = dot(direction, direction);
                if (GpuPapiInRadius(squared, p0.z))
                    particle.velocity += direction * (p0.x * dt / (sqrt(squared) * (squared + p0.y)));
            }
            break;
        case 7u:
        case 11u:
            for (uint otherOrdinal = ordinal + 1u; otherOrdinal < emitter.count; ++otherOrdinal)
            {
                uint otherIndex = GpuParticleIndex(emitter, otherOrdinal);
                float3 otherPosition = g_Particles[otherIndex].position;
                float3 direction = otherPosition - particle.position;
                float squared = dot(direction, direction) + (type == 7u ? 0.0000001 : 0.0);
                if (GpuPapiInRadius(squared, p0.z))
                {
                    float3 otherVelocity = g_Particles[otherIndex].velocity;
                    float3 acceleration = type == 7u ?
                        direction * (p0.x * dt / (sqrt(squared) * (squared + p0.y))) :
                        otherVelocity * (p0.x * dt / (squared + p0.y));
                    particle.velocity += acceleration;
                    g_Particles[otherIndex].velocity = otherVelocity - acceleration;
                }
            }
            break;
        case 8u:
            particle.velocity += p0.xyz * dt;
            break;
        case 9u:
        case 31u:
        {
            float3 direction = particle.position - center;
            float squared = dot(direction, direction);
            if (GpuPapiInRadius(squared, p1.y))
            {
                float3 acceleration;
                if (type == 9u)
                    acceleration = GpuPapiGenerate(domain, emitter.rng);
                else
                    acceleration = direction / sqrt(squared);
                particle.velocity += acceleration * (p0.w * dt / (squared + p1.x));
            }
            break;
        }
        case 12u:
            particle.age += dt;
            particle.positionB = particle.position;
            particle.position += particle.velocity * dt;
            break;
        case 13u:
        case 14u:
        {
            float3 direction = center - particle.position;
            if (type == 13u)
            {
                float3 offset = particle.position - center;
                direction = axis * dot(offset, axis) - offset;
            }
            float squared = dot(direction, direction);
            float maximum = type == 13u ? maximumRadius : p1.y;
            float epsilon = type == 13u ? p1.w : p1.x;
            if (GpuPapiInRadius(squared, maximum))
                particle.velocity += direction * (p0.w * dt / (sqrt(squared) + (squared + epsilon)));
            break;
        }
        case 15u:
            particle.velocity += GpuPapiGenerate(domain, emitter.rng) * dt;
            break;
        case 16u:
            particle.position += GpuPapiGenerate(domain, emitter.rng) * dt;
            break;
        case 17u:
            particle.velocity = GpuPapiGenerate(domain, emitter.rng);
            break;
        case 18u:
            if (state.x <= 0.0)
            {
                particle.position = particle.positionB;
                particle.velocity = 0.0;
            }
            else
            {
                float time = state.x;
                float3 b = (-2.0 * time * particle.velocity + 3.0 * particle.positionB - 3.0 * particle.position) *
                    (dt * 2.0 / (time * time));
                float3 a = (time * particle.velocity - particle.positionB - particle.positionB +
                    particle.position + particle.position) * (dt * dt * 3.0 / (time * time * time));
                particle.velocity += a + b;
            }
            break;
        case 22u:
        {
            float squared = dot(particle.velocity, particle.velocity);
            if (squared < p0.x * p0.x && squared != 0.0)
                particle.velocity *= p0.x / sqrt(squared);
            else if (squared > p0.y * p0.y)
                particle.velocity *= p0.y / sqrt(squared);
            break;
        }
        case 23u:
            if (particle.age >= p1.y * killOldTime && particle.age <= p1.z * killOldTime)
            {
                float4 color = GpuPapiUnpackColor(particle.color);
                particle.color = GpuPapiPackColor(color + (p0 - color) * (p1.x * dt));
            }
            break;
        case 24u:
            particle.size += (p0.xyz - particle.size) * (p1.xyz * dt);
            break;
        case 25u:
        case 26u:
            particle.rotation.x += (abs(p0.x) - abs(particle.rotation.x)) *
                (particle.rotation.x >= 0.0 ? p0.w * dt : -p0.w * dt);
            break;
        case 27u:
        case 28u:
            particle.velocity += (targetVelocity - particle.velocity) * (p0.w * dt);
            break;
        case 29u:
        {
            float3 offset = particle.position - center;
            float squared = dot(offset, offset);
            float maximum = maximumRadius * maximumRadius;
            if (maximum >= 1.0e16 || squared <= maximum)
            {
                float radius = sqrt(squared);
                float3 normal = offset / radius;
                float3 w = axis * dot(normal, axis);
                float3 u = normal - w;
                float3 v = cross(axis, u);
                float theta = p0.w * dt / (squared + p1.w);
                particle.position = (u * cos(theta) + v * sin(theta) + w) * radius + center;
            }
            break;
        }
        case 30u:
        {
            float3 position = particle.position + p1.xyz * state.x;
            float base = GpuPapiFractalSum(position, p0.x, octaves, floorCoordinates);
            float3 perturbation;
            perturbation.x = GpuPapiFractalSum(position + float3(p0.z, 0.0, 0.0), p0.x, octaves, floorCoordinates);
            perturbation.y = GpuPapiFractalSum(position + float3(0.0, p0.z, 0.0), p0.x, octaves, floorCoordinates);
            perturbation.z = GpuPapiFractalSum(position + float3(0.0, 0.0, p0.z), p0.x, octaves, floorCoordinates);
            float speed = sqrt(particle.velocity.x * particle.velocity.x + particle.velocity.z * particle.velocity.z +
                particle.velocity.y * particle.velocity.y);
            particle.velocity += (perturbation - base) * p0.y;
            float newSpeed = sqrt(particle.velocity.x * particle.velocity.x + particle.velocity.z * particle.velocity.z +
                particle.velocity.y * particle.velocity.y);
            particle.velocity *= speed / newSpeed;
            break;
        }
        }
        g_Particles[index] = particle;
    }
    if (type == 5u)
        state.x += dt;
    if (type == 18u)
        state.x -= dt;
    if (type == 5u || type == 18u || type == 30u)
        g_ActionState[stateIndex] = state;
}

#endif
