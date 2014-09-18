#pragma once
#include "game_cl_deathmatch.h"

class CUISpawnWnd;
class CUIGameTDM;

class game_cl_TeamDeathmatch :public game_cl_Deathmatch
{
typedef game_cl_Deathmatch inherited;
	CUIGameTDM*							m_game_ui;
protected:
	bool								m_bFriendlyIndicators;
	bool								m_bFriendlyNames;

	virtual			void				shedule_Update			(u32 dt);
	virtual			void				TranslateGameMessage	(u32 msg, NET_Packet& P);

	virtual			void				LoadSndMessages				();
public :
										game_cl_TeamDeathmatch	();
	virtual								~game_cl_TeamDeathmatch	();
	virtual			void				Init					();
	virtual			void				net_import_state		(NET_Packet& P);
	virtual			CUIGameCustom*		createGameUI			();
	virtual			void				SetGameUI				(CUIGameCustom*);
	virtual			void				GetMapEntities			(xr_vector<SZoneMapEntityData>& dst);
	
	virtual BOOL					CanCallBuyMenu			();
	virtual BOOL					CanCallSkinMenu			();
	virtual	BOOL					CanCallInventoryMenu	();
	virtual	BOOL					CanCallTeamSelectMenu	();


	virtual	void					OnSpectatorSelect		();
	virtual		void				OnSkinMenuBack			();
	virtual		void				OnTeamMenuBack			();
	virtual		void				OnMapInfoAccept			();

	virtual		void				OnGameMenuRespond_ChangeTeam	(NET_Packet& P);

	virtual			void				OnTeamSelect			(int Result);
	virtual			char*				getTeamSection			(int Team);
	virtual			void				OnTeamChanged			();
	virtual			void				PlayRankChangesSndMessage ();
	virtual			void				OnTeamMenu_Cancel		();

	virtual			void				Set_ShowPlayerNames		(bool Show) {m_bShowPlayersNames = Show;};
	virtual			bool				Get_ShowPlayerNames		() {return m_bShowPlayersNames;};
	virtual			s16					ModifyTeam				(s16 Team)	{return (Team != -1) ? Team-1 : Team;};
	virtual			bool				Get_ShowPlayerNamesEnabled () {return m_bFriendlyNames;};
	
	virtual			bool				IsPlayerInTeam			(game_PlayerState* ps, ETeam team);
	virtual			LPCSTR				GetGameScore			(string32&	score_dest);
	s32									GetGreenTeamScore		() const { return teams[0].score; };
	s32									GetBlueTeamScore		() const { return teams[1].score; };
//from UIGameTDM
protected:
	virtual const shared_str			GetBaseCostSect			() {return "teamdeathmatch_base_cost";}
	virtual const shared_str			GetTeamMenu				(s16 team);
//	CUISpawnWnd*						pUITeamSelectWnd;

	PRESET_ITEMS						PresetItemsTeam1;
	PRESET_ITEMS						PresetItemsTeam2;

	BOOL								m_bTeamSelected;
	bool								m_bShowPlayersNames;

	virtual bool						CanBeReady				();
	virtual	void						SetCurrentBuyMenu		();
	virtual	void						SetCurrentSkinMenu		();

	virtual	bool						OnKeyboardPress			(int key);

	virtual		void					OnRender				();
	virtual		bool					IsEnemy					(game_PlayerState* ps);
	virtual		bool					IsEnemy					(CEntityAlive* ea1, CEntityAlive* ea2);

	virtual void						UpdateMapLocations		();
	virtual	void						OnSwitchPhase			(u32 old_phase, u32 new_phase);	

	virtual	void						SetScore				();
	virtual	void						OnSwitchPhase_InProgress();

	virtual		u8						GetTeamCount			() { return 2; };
	virtual		void					OnConnected				();
};

IC bool	TDM_Compare_Players		(game_PlayerState* p1, game_PlayerState* p2)
{
	return DM_Compare_Players(p1, p2);
};