#include "StdAfx.h"
#include "alife_space.h"
#include "xrCore/xr_token.h"

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
    {nullptr, 0}
};

EHitType g_tfString2HitType(LPCSTR caHitType)
{
    if (!xr_stricmp(caHitType, "burn"))
        return eHitTypeBurn;
    if (!xr_stricmp(caHitType, "light_burn"))
        return eHitTypeLightBurn;
    if (!xr_stricmp(caHitType, "shock"))
        return eHitTypeShock;
    if (!xr_stricmp(caHitType, "strike"))
        return eHitTypeStrike;
    if (!xr_stricmp(caHitType, "wound"))
        return eHitTypeWound;
    if (!xr_stricmp(caHitType, "radiation"))
        return eHitTypeRadiation;
    if (!xr_stricmp(caHitType, "telepatic"))
        return eHitTypeTelepatic;
    if (!xr_stricmp(caHitType, "fire_wound"))
        return eHitTypeFireWound;
    if (!xr_stricmp(caHitType, "chemical_burn"))
        return eHitTypeChemicalBurn;
    if (!xr_stricmp(caHitType, "explosion"))
        return eHitTypeExplosion;
    if (!xr_stricmp(caHitType, "wound_2"))
        return eHitTypeWound_2;

    FATAL("Unsupported hit type!");
    NODEFAULT;
#ifdef DEBUG
    return eHitTypeMax;
#endif
}

pcstr g_cafHitType2String(EHitType tHitType)
{
    return get_token_name(hit_types_token, tHitType);
}

} // namespace ALife
