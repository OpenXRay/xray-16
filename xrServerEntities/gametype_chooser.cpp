#include "stdafx.h"
#pragma hdrstop


#include "gametype_chooser.h"
#include "xrServer_Objects_Abstract.h"
//old
enum ERPGameType{		// [0..255]
	rpgtGameAny							= u8(0),
	rpgtGameDeathmatch,
	rpgtGameTeamDeathmatch,
	rpgtGameArtefactHunt,
	rpgtGameCaptureTheArtefact,
	rpgtGameCount,
};

xr_token rpoint_game_type[]={
	{ "Any game",			rpgtGameAny					},
	{ "Deathmatch",			rpgtGameDeathmatch			},
	{ "TeamDeathmatch",		rpgtGameTeamDeathmatch		},
	{ "ArtefactHunt",		rpgtGameArtefactHunt		},
	{ "CaptureTheArtefact",	rpgtGameCaptureTheArtefact	},
	{ 0,					0	}
};


#ifdef _EDITOR
bool GameTypeChooser::LoadStream(IReader& F)
{
    m_GameType.assign	(F.r_u16());

    return true;
}

bool GameTypeChooser::LoadLTX(CInifile& ini, LPCSTR sect_name, bool bOldFormat)
{
    if(bOldFormat/*version==0x0014*/)
    {
        u8 tmp 					= ini.r_u8	(sect_name, "game_type");
        m_GameType.zero		();
        switch(tmp)
        {
            case rpgtGameAny:
                m_GameType.one();
                break;
            case rpgtGameDeathmatch:
                m_GameType.set(eGameIDDeathmatch,TRUE);
                break;
            case rpgtGameTeamDeathmatch:
                m_GameType.set(eGameIDTeamDeathmatch,TRUE);
                break;
            case rpgtGameArtefactHunt:
                m_GameType.set(eGameIDArtefactHunt,TRUE);
                break;
            case rpgtGameCaptureTheArtefact:
                m_GameType.set(eGameIDCaptureTheArtefact,TRUE);
                break;
        }
    }else
        m_GameType.assign		(ini.r_u16	(sect_name, "game_type"));
    return true;
}

void GameTypeChooser::SaveStream(IWriter& F)
{
   F.w_u16 	(m_GameType.get());
}

void GameTypeChooser::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
  ini.w_u16(sect_name, "game_type", m_GameType.get());
}
#endif

#ifndef XRGAME_EXPORTS
void  GameTypeChooser::FillProp(LPCSTR pref, PropItemVec& items)
{
	PHelper().CreateGameType		(items, PrepareKey(pref, "Game Type"), this);
/*
    PHelper().CreateFlag16  (items, PrepareKey(pref, "Game Type\\single"),      			&m_GameType, eGameIDSingle);
	PHelper().CreateFlag16  (items, PrepareKey(pref, "Game Type\\deathmatch"),				&m_GameType, eGameIDDeathmatch);
    PHelper().CreateFlag16  (items, PrepareKey(pref, "Game Type\\team deathmatch"),     	&m_GameType, eGameIDTeamDeathmatch);
    PHelper().CreateFlag16  (items, PrepareKey(pref, "Game Type\\artefact hunt"),       	&m_GameType, eGameIDArtefactHunt);
    PHelper().CreateFlag16  (items, PrepareKey(pref, "Game Type\\capture the artefact"),	&m_GameType, eGameIDCaptureTheArtefact);
    PHelper().CreateFlag16  (items, PrepareKey(pref, "Game Type\\domination zone"),     	&m_GameType, eGameIDDominationZone);
    PHelper().CreateFlag16  (items, PrepareKey(pref, "Game Type\\team domination zone"),	&m_GameType, eGameIDTeamDominationZone);
*/
 }
#endif // #ifndef XRGAME_EXPORTS