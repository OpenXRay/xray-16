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

EGameIDs ParseStringToGameType(pcstr str);

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
