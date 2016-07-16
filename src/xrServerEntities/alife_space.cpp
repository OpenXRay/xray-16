#include "stdafx.h"
#include "alife_space.h"

namespace ALife
{
const xr_token hit_types_token[] =
{
    {"burn", eHitTypeBurn},
    {"shock", eHitTypeShock},
    {"strike", eHitTypeStrike},
    {"wound", eHitTypeWound},
    {"radiation", eHitTypeRadiation},
    {"telepatic", eHitTypeTelepatic},
    {"fire_wound", eHitTypeFireWound},
    {"chemical_burn", eHitTypeChemicalBurn},
    {"explosion", eHitTypeExplosion},
    {"wound_2", eHitTypeWound_2},
    //{"physic_strike", eHitTypePhysicStrike},
    {"light_burn", eHitTypeLightBurn},
    {0, 0}
};

EHitType g_tfString2HitType(LPCSTR caHitType)
{
    if (!_stricmp(caHitType, "burn"))
        return eHitTypeBurn;
    if (!_stricmp(caHitType, "light_burn"))
        return eHitTypeLightBurn;
    if (!_stricmp(caHitType, "shock"))
        return eHitTypeShock;
    if (!_stricmp(caHitType, "strike"))
        return eHitTypeStrike;
    if (!_stricmp(caHitType, "wound"))
        return eHitTypeWound;
    if (!_stricmp(caHitType, "radiation"))
        return eHitTypeRadiation;
    if (!_stricmp(caHitType, "telepatic"))
        return eHitTypeTelepatic;
    if (!_stricmp(caHitType, "fire_wound"))
        return eHitTypeFireWound;
    if (!_stricmp(caHitType, "chemical_burn"))
        return eHitTypeChemicalBurn;
    if (!_stricmp(caHitType, "explosion"))
        return eHitTypeExplosion;
    if (!_stricmp(caHitType, "wound_2"))
        return eHitTypeWound_2;

    FATAL("Unsupported hit type!");
    NODEFAULT;
#ifdef DEBUG
    return eHitTypeMax;
#endif
}

} // namespace ALife
