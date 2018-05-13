#pragma once

namespace PAPI
{
// A effect of particles - Info and an array of Particles
struct PARTICLES_API ParticleEffect
{
    u32 p_count; // Number of particles currently existing.
    u32 max_particles; // Max particles allowed in effect.
    u32 particles_allocated; // Actual allocated size.
    Particle* particles; // Actually, num_particles in size
    OnBirthParticleCB b_cb;
    OnDeadParticleCB d_cb;
    void* owner;
    u32 param;

    ParticleEffect(int mp);

    ~ParticleEffect();

    int Resize(u32 max_count);

    void Remove(int i);

    bool Add(const pVector& pos, const pVector& posB, const pVector& size, const pVector& rot, const pVector& vel,
             u32 color, const float age = 0.0f, u16 frame = 0, u16 flags = 0);
};
} // namespace PAPI
