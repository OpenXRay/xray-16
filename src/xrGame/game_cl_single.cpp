#include "pch_script.h"
#include "game_cl_single.h"
#include "UIGameSP.h"
#include "actor.h"
#include "clsid_game.h"

using namespace luabind;

ESingleGameDifficulty g_SingleGameDifficulty = egdStalker;

xr_token	difficulty_type_token						[ ]={
	{ "gd_novice",						egdNovice									},
	{ "gd_stalker",						egdStalker									},
	{ "gd_veteran",						egdVeteran									},
	{ "gd_master",						egdMaster									},
	{ 0,							0											}
};

game_cl_Single::game_cl_Single()
{
}

CUIGameCustom* game_cl_Single::createGameUI()
{
	CLASS_ID clsid			= CLSID_GAME_UI_SINGLE;
	CUIGameSP*	pUIGame		= smart_cast<CUIGameSP*> ( NEW_INSTANCE ( clsid ) );
	R_ASSERT				(pUIGame);
	pUIGame->Load			();
	pUIGame->SetClGame		(this);
	pUIGame->Init			(0);
	pUIGame->Init			(1);
	pUIGame->Init			(2);
	return					pUIGame;
}

char*	game_cl_Single::getTeamSection(int Team)
{
	return NULL;
};

void game_cl_Single::OnDifficultyChanged()
{
	Actor()->OnDifficultyChanged();
}

#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_time_manager.h"
ALife::_TIME_ID game_cl_Single::GetGameTime		()
{
	if (ai().get_alife() && ai().alife().initialized())
		return(ai().alife().time_manager().game_time());
	else
		return(inherited::GetGameTime());
}

ALife::_TIME_ID game_cl_Single::GetStartGameTime	()
{
	if (ai().get_alife() && ai().alife().initialized())
		return(ai().alife().time_manager().start_game_time());
	else
		return(inherited::GetStartGameTime());
}

float game_cl_Single::GetGameTimeFactor		()
{
	if (ai().get_alife() && ai().alife().initialized())
		return(ai().alife().time_manager().time_factor());
	else
		return(inherited::GetGameTimeFactor());
}

void game_cl_Single::SetGameTimeFactor(const float fTimeFactor)
{
	Level().Server->game->SetGameTimeFactor(fTimeFactor);
}

ALife::_TIME_ID game_cl_Single::GetEnvironmentGameTime	()
{
	if (ai().get_alife() && ai().alife().initialized())
		return	(ai().alife().time_manager().game_time());
	else
		return	(inherited::GetEnvironmentGameTime());
}

float game_cl_Single::GetEnvironmentGameTimeFactor		()
{
	if (ai().get_alife() && ai().alife().initialized())
		return	(ai().alife().time_manager().time_factor());
	else
		return	(inherited::GetEnvironmentGameTimeFactor());
}

void game_cl_Single::SetEnvironmentGameTimeFactor		(const float fTimeFactor)
{
	if (ai().get_alife() && ai().alife().initialized())
		Level().Server->game->SetGameTimeFactor(fTimeFactor);
	else
		inherited::SetEnvironmentGameTimeFactor(fTimeFactor);
}

#pragma optimize("s",on)
void CScriptGameDifficulty::script_register(lua_State *L)
{
	module(L)
		[
			class_<enum_exporter<ESingleGameDifficulty> >("game_difficulty")
			.enum_("game_difficulty")
			[
				value("novice",				int(egdNovice			)),
				value("stalker",			int(egdStalker			)),
				value("veteran",			int(egdVeteran			)),
				value("master",				int(egdMaster			))
			]
		];
}