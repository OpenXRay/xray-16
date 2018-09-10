#include "StdAfx.h"

#include "UIStats.h"
#include "UIXmlInit.h"
#include "UIStatsPlayerList.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "Level.h"
#include "game_base_space.h"

CUIStats::CUIStats() {}
CUIStats::~CUIStats() {}
CUIWindow* CUIStats::InitStats(CUIXml& xml_doc, LPCSTR path, int team)
{
    string256 _path;
    CUIXmlInit::InitScrollView(xml_doc, path, 0, this);
    this->SetFixedScrollBar(false);
    CUIWindow* pWnd = NULL;
    CUIWindow* pTinfo = NULL;

    // players
    CUIStatsPlayerList* pPList = new CUIStatsPlayerList();
    pPList->SetTeam(team);
    pPList->Init(xml_doc, strconcat(sizeof(_path), _path, path, ":player_list"));
    pPList->SetMessageTarget(this);
    pWnd = pPList->GetHeader();
    pTinfo = pPList->GetTeamHeader();
    AddWindow(pWnd, true);
    AddWindow(pPList, true);

    if (xml_doc.NavigateToNode(strconcat(sizeof(_path), _path, path, ":spectator_list"), 0))
    {
        // spectators
        pPList = new CUIStatsPlayerList();
        pPList->SetTeam(team);
        pPList->Init(xml_doc, _path);
        pPList->SetMessageTarget(this);
        pWnd = pPList->GetHeader();
        AddWindow(pWnd, true);
        AddWindow(pPList, true);
    }

    return pTinfo;
}
