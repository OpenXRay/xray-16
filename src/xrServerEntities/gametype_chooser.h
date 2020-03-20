#pragma once

#include "xrCore/_std_extensions.h"
#include "xrCore/_flags.h"
#include "xrCore/xrstring.h"
#include "xrCommon/xr_vector.h"

// fwd. decl.
class IReader;
class IWriter;
class CInifile;

// new
enum EGameIDs
{
    eGameIDNoGame = u32(0),
    eGameIDSingle = u32(1) << 0,
    eGameIDDeathmatch = u32(1) << 1,
    eGameIDTeamDeathmatch = u32(1) << 2,
    eGameIDArtefactHunt = u32(1) << 3,
    eGameIDCaptureTheArtefact = u32(1) << 4,
    eGameIDDominationZone = u32(1) << 5,
    eGameIDTeamDominationZone = u32(1) << 6,
};

// This enum should "extend" EGameIDs with the values which are defined in SoC.
// It's necessary for correct displaying game types in the unified master-list
enum EGameIDs_ext
{
    eGameIDTeamDeathmatch_SoC = u32(6),
    eGameIDArtefactHunt_SoC = u32(7),
};

inline EGameIDs ParseStringToGameType(pcstr str)
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

class PropValue;
class PropItem;
using PropItemVec = xr_vector<PropItem*>;

struct GameTypeChooser
{
    Flags16 m_GameType;
#ifndef XRGAME_EXPORTS
    void FillProp(LPCSTR pref, PropItemVec& items);
#endif // #ifndef XRGAME_EXPORTS

#ifdef _EDITOR
    bool LoadStream(IReader& F);
    bool LoadLTX(CInifile& ini, LPCSTR sect_name, bool bOldFormat);
    void SaveStream(IWriter&);
    void SaveLTX(CInifile& ini, LPCSTR sect_name);
#endif
    void SetDefaults() { m_GameType.one(); }
    bool MatchType(const u16 t) const { return (t == eGameIDNoGame) || !!m_GameType.test(t); };
};
