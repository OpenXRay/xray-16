#include "pch.hpp"
#include "xrServerEntities/gametype_chooser.h"

EGameIDs ParseStringToGameType(pcstr str)
{
    auto IS = [&](pcstr name)
    {
        return !xr_strcmp(str, name);
    };

    if (IS("single"))                          return eGameIDSingle;
    if (IS("deathmatch") || IS("dm"))          return eGameIDDeathmatch;
    if (IS("teamdeathmatch") || IS("tdm"))     return eGameIDTeamDeathmatch;
    if (IS("artefacthunt") || IS("ah"))        return eGameIDArtefactHunt;
    if (IS("capturetheartefact") || IS("cta")) return eGameIDCaptureTheArtefact;
    if (IS("dominationzone"))                  return eGameIDDominationZone;
    if (IS("teamdominationzone"))              return eGameIDTeamDominationZone;
    return eGameIDNoGame; //EGameIDs
}