#pragma once
#include "PHSimpleCharacter.h"

class CPHAICharacter : public CPHSimpleCharacter
{
    typedef CPHSimpleCharacter inherited;

    bool m_forced_physics_control;

public:
    CPHAICharacter();
    virtual CPHAICharacter* CastAICharacter() { return this; }

    virtual void ValidateWalkOn();

    virtual bool TryPosition(Fvector pos, bool exact_state);
    virtual void Jump(const Fvector& jump_velocity);
    virtual void SetMaximumVelocity(float vel) { m_max_velocity = vel; }
    virtual void InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2);
    virtual void SetForcedPhysicsControl(bool v) { m_forced_physics_control = v; }
    virtual bool ForcedPhysicsControl() { return m_forced_physics_control; }
    virtual void Create(Fvector sizes);
    virtual void GetSavedVelocity(Fvector& vvel) override;
    virtual void GetVelocity(Fvector& vvel) const override;

private:
    virtual void UpdateStaticDamage(const Fvector& normal, const Fvector& pos, SGameMtl* tri_material, bool bo1) {}

#ifdef DEBUG
    virtual void OnRender();
#endif
};
