#pragma once
#include "xrMaterialSystem/GameMtlLib.h"
#include "xrPhysicsCore/IPhysicsCore.h"

inline u16 NativeContactMaterialIndex(const SGameMtl* material, const NativePhysicsContact* contact)
{
    if (contact) {
        for (const auto index : {contact->material1, contact->material2})
            if (index < GMLib.CountMaterial() && GMLib.GetMaterialByIdx(index) == material) return index;
    }
    // Fallback materials and callbacks can resolve a different material than
    // the raw native shape tag. Preserve the existing name lookup in that case.
    return GMLib.GetMaterialIdx(material->m_Name.c_str());
}

template <class Parameters>
inline bool NativeContactHasEffects(float energy, float squareCameraDistance, bool passable, bool triangle,
    float squareSoundDistance, float squareParticleDistance)
{
    return (triangle && energy > Parameters::vel_cret_wallmark) ||
        (squareCameraDistance < squareSoundDistance && (passable || energy > Parameters::vel_cret_sound)) ||
        (squareCameraDistance < squareParticleDistance && energy > Parameters::vel_cret_particles);
}
