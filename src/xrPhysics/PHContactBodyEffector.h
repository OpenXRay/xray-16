#pragma once

#include "PHBaseBodyEffector.h"
#include "xrPhysicsCore/IPhysicsCore.h"

struct SGameMtl;
class CPHContactBodyEffector : public CPHBaseBodyEffector
{
    Fvector m_contact_normal;
    float m_contact_depth;
    float m_recip_flotation;
    SGameMtl* m_material;

public:
    void Init(CharacterVirtualHandle body, const Fvector& normal, float depth, SGameMtl* material);
    void Merge(const Fvector& normal, float depth, SGameMtl* material);
    void Apply();
};
