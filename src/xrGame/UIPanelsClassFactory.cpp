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
		return new UITeamState(etGreenTeam, teamPanels);
	} else if (teamName == "blueteam")
	{
		return new UITeamState(etBlueTeam, teamPanels);
	} else if (teamName == "spectatorsteam")
	{
		return new UITeamState(etSpectatorsTeam, teamPanels);
	} else if (teamName == "greenteam_pending")
	{
		return new UITeamState(etGreenTeam, teamPanels);
	} else if (teamName == "blueteam_pending")
	{
		return new UITeamState(etBlueTeam, teamPanels);
	}
	return NULL;
}