#include "StdAfx.h"
#include "UIPanelsClassFactory.h"
#include "game_base.h"
#include "UITeamPanels.h"

UIPanelsClassFactory::UIPanelsClassFactory()
{
}

UIPanelsClassFactory::~UIPanelsClassFactory()
{
}
	
UITeamState* UIPanelsClassFactory::CreateTeamPanel(shared_str const & teamName, UITeamPanels *teamPanels)
{
	if (teamName == "greenteam")
	{
		return xr_new<UITeamState>(etGreenTeam, teamPanels);
	} else if (teamName == "blueteam")
	{
		return xr_new<UITeamState>(etBlueTeam, teamPanels);
	} else if (teamName == "spectatorsteam")
	{
		return xr_new<UITeamState>(etSpectatorsTeam, teamPanels);
	} else if (teamName == "greenteam_pending")
	{
		return xr_new<UITeamState>(etGreenTeam, teamPanels);
	} else if (teamName == "blueteam_pending")
	{
		return xr_new<UITeamState>(etBlueTeam, teamPanels);
	}
	return NULL;
}