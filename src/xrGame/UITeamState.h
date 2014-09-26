#ifndef UITEAMSTATE
#define UITEAMSTATE

#include "ui/UIWindow.h"
#include "ui/xrUIXmlParser.h"
#include "ui/UIXmlInit.h"

#include "game_cl_base.h"
#include "string_table.h"
#include "game_base.h"
#include "level.h"

#include "associative_vector.h"

class UITeamPanels;
class CUIFrameLineWnd;
class CUIScrollView;
class CUIXml;
class UIPlayerItem;
class UITeamHeader;

class UITeamState : public CUIWindow
{
private:
	typedef CUIWindow inherited;
	
	typedef std::pair<CUIScrollView*, UITeamHeader*>	TScrollPanel;
	typedef xr_vector<TScrollPanel>						TScrollPanels;

	struct TPlayerItem
	{
		UIPlayerItem*				m_player_wnd;
		TScrollPanels::size_type	m_panel_number;
		TPlayerItem(UIPlayerItem* player_wnd, TScrollPanels::size_type panel_number)
		{
			m_player_wnd	= player_wnd;
			m_panel_number	= panel_number;
		}
	};

	typedef associative_vector<ClientID, TPlayerItem> MapClientIdToUIPlayer;

	
	
	ETeam					myTeam;
	MapClientIdToUIPlayer	myPlayers;
	
	XML_NODE*				teamXmlNode;
	CUIXml*					mainUiXml;
	
		
	TScrollPanels			m_scroll_panels;
	
	xr_vector<ClientID>		toDeletePlayers;

	s32						m_artefact_count;
	
	UITeamPanels*	m_teamPanels;
	
	UITeamState();
	bool __stdcall SortingLessFunction(CUIWindow* left, CUIWindow* right);
	int				InitScrollPanels();
	
	int		m_last_panel;

	inline	TScrollPanels::size_type	GetNeedScrollPanelIndex();
			void						ReStoreAllPlayers();
			void						CleanupInternal();
public:
	UITeamState(ETeam teamId, UITeamPanels *teamPanels);
	virtual ~UITeamState();
	void Init(CUIXml& uiXml, LPCSTR teamNodeName, int index);

		
	void		AddPlayer(ClientID const & clientId);
	void		RemovePlayer(ClientID const & clientId);
	bool		UpdatePlayer(ClientID const & clientId);

	void		SetArtefactsCount(s32 greenTeamArtC, s32 blueTeamArtC);

	s32			GetFieldValue(shared_str const & field_name) const;
	s32			GetSummaryFrags() const;

	virtual void Update();
	virtual void Draw();
};

#endif