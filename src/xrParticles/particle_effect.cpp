#include "stdafx.h"

#include "particle_effect.h"

namespace PAPI
{
ParticleEffect::ParticleEffect(int mp)
{
    owner = nullptr;
    param = 0;
    b_cb = nullptr;
    d_cb = nullptr;
    p_count = 0;
    max_particles = mp;
    particles_allocated = max_particles;

    particles = xr_alloc<Particle>(max_particles);
    //Msg("Allocated %u bytes (%u particles) with base address 0x%p", max_particles * sizeof(Particle), max_particles, particles);
}

ParticleEffect::~ParticleEffect() { xr_free(particles); }

int ParticleEffect::Resize(u32 max_count)
{
    // Reducing max.
    if (particles_allocated >= max_count)
    {
        max_particles = max_count;

        // May have to kill particles.
        if (p_count > max_particles)
            p_count = max_particles;

        return max_count;
    }

    // Allocate particles.
    Particle* new_particles = xr_alloc<Particle>(max_count);
    if (!new_particles)
    {
        // ERROR - Not enough memory. Just give all we've got.
        max_particles = particles_allocated;
        return max_particles;
    }

    //Msg("Re-allocated %u bytes (%u particles) with base address 0x%p", max_count * sizeof(Particle), max_count, new_particles);

    CopyMemory(new_particles, particles, p_count*sizeof(Particle));
    xr_free(particles);

    particles = new_particles;

    max_particles = max_count;
    particles_allocated = max_count;
    return max_count;
}

void ParticleEffect::Remove(int i)
{
    if (0 == p_count)
        return;
    Particle& m = particles[i];

    if (d_cb)
        d_cb(owner, param, m, i);

    m = particles[--p_count]; // не менять правило удаления !!! (dependence ParticleGroup)
    //Msg("pDel() : %u", p_count);
}

bool ParticleEffect::Add(const pVector& pos, const pVector& posB, const pVector& size, const pVector& rot,
                         const pVector& vel, u32 color, const float age /*= 0.0f*/, u16 frame /*= 0*/,
                         u16 flags /*= 0*/)
{
    if (p_count >= max_particles)
        return false;
    Particle& P = particles[p_count];
    P.pos = pos;
    P.posB = posB;
    P.size = size;
    P.rot.x = rot.x;
    P.vel = vel;
    P.color = color;
    P.age = age;
    P.frame = frame;
    P.flags.assign(flags);
    if (b_cb)
        b_cb(owner, param, P, p_count);
    p_count++;
    //Msg("pAdd() : %u", p_count);
    return true;
}
} // namespace PAPI
