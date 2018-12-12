#pragma once

#include "particle_actions.h"

namespace PAPI
{
class CParticleManager : public IParticleManager
{
    // These are static because all threads access the same effects.
    // All accesses to these should be locked.
    using ParticleEffectVec = xr_vector<ParticleEffect*>;
    using ParticleActionsVec = xr_vector<ParticleActions*>;
    ParticleEffectVec effect_vec;
    ParticleActionsVec m_alist_vec;

public:
    CParticleManager();
    virtual ~CParticleManager();
    // Return an index into the list of particle effects where
    ParticleEffect* GetEffectPtr(int effect_id);
    ParticleActions* GetActionListPtr(int alist_id);

    // create&destroy
    int CreateEffect(u32 max_particles) override;
    void DestroyEffect(int effect_id) override;
    int CreateActionList() override;
    void DestroyActionList(int alist_id) override;

    // control
    void PlayEffect(int effect_id, int alist_id) override;
    void StopEffect(int effect_id, int alist_id, bool deffered = true) override;

    // update&render
    void Update(int effect_id, int alist_id, float dt) override;
    void Render(int effect_id) override;
    void Transform(int alist_id, const Fmatrix& m, const Fvector& velocity) override;

    // effect
    void RemoveParticle(int effect_id, u32 p_id) override;
    void SetMaxParticles(int effect_id, u32 max_particles) override;
    void SetCallback(int effect_id, OnBirthParticleCB b, OnDeadParticleCB d, void* owner, u32 param) override;
    void GetParticles(int effect_id, Particle*& particles, u32& cnt) override;
    u32 GetParticlesCount(int effect_id) override;

    // action
    ParticleAction* CreateAction(PActionEnum action_id) override;
    size_t LoadActions(int alist_id, IReader& R) override;
    void SaveActions(int alist_id, IWriter& W) override;
};
} // namespace PAPI
