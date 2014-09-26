#pragma once

#include "game_cl_mp.h"
#include "UIGameCTA.h"
#include "game_cl_capture_the_artefact_captions_manager.h"
/// This class describes the client part of "Capture The Artefact" game mode.
class game_cl_CaptureTheArtefact	: public game_cl_mp
{
private:
	typedef game_cl_mp inherited;
	
	CUIGameCTA * m_game_ui;

	BOOL	m_bReadMapDesc;
	BOOL	m_bTeamSelected;
	BOOL	m_bSkinSelected;
	BOOL	m_winnerTeamShowed;
	
	CTAGameClCaptionsManager	m_captions_manager;

	u32		m_curReinforcementTime;
	u32		m_maxReinforcementTime;

	u32		m_currentWarmupTime;
	s32		m_s32TimeLimit;
	bool	m_inWarmup;

	s32 maxScore;
	s32 greenTeamScore;
	s32 blueTeamScore;

	Fvector	greenTeamRPos;
	Fvector	blueTeamRPos;

	u16 greenArtefact;
	u16 blueArtefact;
	bool haveGotUpdate;
	//bool sendedSpawnMe;

	u16 greenArtefactOwner;	//GameID, if 0, then no body took the artefact
	u16 blueArtefactOwner;
	
	const shared_str & GetLocalPlayerTeamSection() const;
	ETeam GetLocalPlayerTeam() const;
	
	bool		m_player_on_base;
	bool		m_allow_buy;
	bool		m_bFriendlyIndicators;
	bool		m_bFriendlyNames;
	bool		m_bBearerCantSprint;
	bool		m_bCanActivateArtefact;
	bool		m_bShowPlayersNames;
	u32			m_dwVoteEndTime;
	float		m_baseRadius;

	s32		spawn_cost;
	s32		buy_amount;
	s32		total_money;
	s32		last_money;

	void UpdateMoneyIndicator();
	
	void LoadSndMessages();
	void OnPlayerEnterBase();
	void OnPlayerLeaveBase();
	void SetInvinciblePlayer(u16 const gameId, bool const invincible);
	void SpawnMe();

	void PlayCapturedTheArtefact(game_PlayerState const * capturer);
	void PlayDeliveredTheArtefact(game_PlayerState const * deliverer);
	void PlayReturnedTheArtefact(game_PlayerState const * returnerer);
	void PlayRankChangedSnd();

	void UpdateVotingTime(u32 current_time);
	void UpdateWarmupTime(u32 current_time);
	void UpdateTimeLimit(u32 current_time);
public:
	game_cl_CaptureTheArtefact();
	void	SetGameUI(CUIGameCustom * already_created_ui);
	virtual	~game_cl_CaptureTheArtefact();
	virtual	CUIGameCustom * createGameUI();
	/// Initializes teams
	virtual	void Init();
	virtual	void shedule_Update(u32 dt);
	virtual	void net_import_state(NET_Packet& P);
	virtual	void net_import_update(NET_Packet& P);
	virtual	void TranslateGameMessage(u32 msg, NET_Packet& P);
	
	virtual	void OnGameMenuRespond_ChangeSkin(NET_Packet& P);
	virtual	void OnGameMenuRespond_ChangeTeam(NET_Packet& P);
	virtual	void OnPlayerFlagsChanged(game_PlayerState* ps);
	virtual	void OnNewPlayerConnected(ClientID const & newClient);
	virtual void OnRender();
	
	virtual	bool IsEnemy(game_PlayerState* ps);

	virtual	bool CanBeReady();
	virtual bool NeedToSendReady_Actor			(int key, game_PlayerState* ps);
	virtual bool NeedToSendReady_Spectator		(int key, game_PlayerState* ps);

	virtual bool OnKeyboardPress(int key);
	virtual	bool OnKeyboardRelease(int key);
	
	virtual	void OnSpawn(CObject* pObj);
	virtual	BOOL CanCallBuyMenu();
	virtual BOOL CanCallSkinMenu() {return TRUE;};
	virtual BOOL CanCallTeamSelectMenu();
	virtual	BOOL CanCallInventoryMenu();
			bool LocalPlayerCanBuyItem(shared_str const & name_sect);
	
			void	Set_ShowPlayerNames			(bool Show) {m_bShowPlayersNames = Show;};
			bool	Get_ShowPlayerNames			()			{return m_bShowPlayersNames;};
			bool	Get_ShowPlayerNamesEnabled	()			{return m_bFriendlyNames;};

	//ui part
	virtual	void OnSpectatorSelect();
	virtual	void OnMapInfoAccept();
	
	virtual	void OnTeamChanged();
	virtual	void OnRankChanged(u8 OldRank);
			void OnTeamScoresChanged();

	virtual	void OnTeamMenuBack();
	virtual	void OnTeamMenu_Cancel();
	virtual	void OnTeamSelect(int Team);
	virtual	void OnBuyMenu_Ok();
	virtual	void OnBuyMenu_Cancel();
	virtual void OnBuySpawnMenu_Ok();
			
			void OnBuyMenuOpen();

	virtual	void OnSkinMenuBack			();
	virtual void OnSkinMenu_Ok			();
	virtual	void OnSkinMenu_Cancel		();

	virtual void UpdateMapLocations		();

	virtual	void	OnVoteStart(NET_Packet& P);
	virtual	void	OnVoteStop(NET_Packet& P);
	virtual	void	OnVoteEnd(NET_Packet& P);
	
	virtual	char*	getTeamSection			(int Team);

	virtual	s16	ModifyTeam(s16 Team) {return Team;};
	virtual	bool PlayerCanSprint(CActor* pActor);

	//very bad... need to fix problem with team indexes...
	virtual void OnSpeechMessage(NET_Packet& P);
	virtual	u8 GetTeamCount	() { return 2; };
	
	virtual	void OnSwitchPhase(u32 old_phase, u32 new_phase);
	virtual	void OnGameRoundStarted	();
	u16 GetGreenArtefactOwnerID		() const;
	u16 GetBlueArtefactOwnerID		() const;
	
	Fvector const & GetGreenArtefactRPoint	()	const;
	Fvector const & GetBlueArtefactRPoint	()	const;
	float			GetBaseRadius			()	const;
	u16				GetGreenArtefactID		()	const;
	u16				GetBlueArtefactID		()	const;
	bool			CanActivateArtefact		()	const;

	bool			InWarmUp				()	const;
	virtual	bool	Is_Rewarding_Allowed	()  const { return !InWarmUp(); };
	bool			HasTimeLimit			()	const;

	virtual	LPCSTR	GetGameScore			(string32&	score_dest);
	virtual	void	OnConnected				();
	s32				GetGreenTeamScore		() const { return greenTeamScore; };
	s32				GetBlueTeamScore		() const { return blueTeamScore; };
};