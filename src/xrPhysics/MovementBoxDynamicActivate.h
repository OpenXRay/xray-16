#pragma once
class CPHCharacter;
class IPHMovementControl
{
public:
    virtual CPHCharacter* character() = 0;
    virtual void actor_calculate(
        Fvector& vAccel, const Fvector& camDir, float ang_speed, float jump, float dt, bool bLight) = 0;
    virtual const Fbox* Boxes() = 0;
    virtual const Fbox& Box() = 0;
    virtual void InterpolateBox(u32 id, float k) = 0;
};

XRPHYSICS_API bool ActivateBoxDynamic(IPHMovementControl* mov_control, bool character_exist, u32 id,
    int num_it /*=8*/, int num_steps /*5*/, float resolve_depth /*=0.01f*/);
