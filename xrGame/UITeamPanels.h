#ifndef UITEAMPANELS
#define UITEAMPANELS

#include "ui/UIWindow.h"
#include "UIPanelsClassFactory.h"
#include "ui/xrUIXmlParser.h"
#include "ui/UIXmlInit.h"

class UITeamState;

class UITeamPanels : public CUIWindow
{
private:
	typedef CUIWindow inherited;
	UIPanelsClassFactory	panelsFactory;
	CUIXml					uiXml;
	bool					need_update_players;
	bool					need_update_panels;
	// this is only the pointers so it can be dereferenced in UIPlayerItem constructor
	typedef associative_vector<shared_str, UITeamState*> TTeamsMap;

	TTeamsMap myPanels;
	void UpdateExistingPlayers();
	void UpdatePanels();
	void InitAllFrames(shared_str const & frame_node);
	void InitAllTeams(shared_str const & team_node);
public:
	UITeamPanels();
	virtual ~UITeamPanels();
	virtual void Update();
	
	void Init(LPCSTR xmlName, LPCSTR panelsRootNode);
	
	void AddPlayer(ClientID const & clientId);
	void RemovePlayer(ClientID const & clientId);
	void UpdatePlayer(ClientID const & clientId);
	void NeedUpdatePlayers();
	void NeedUpdatePanels();

	void SetArtefactsCount(s32 greenTeamArtC, s32 blueTeamArtC);
};

#endif