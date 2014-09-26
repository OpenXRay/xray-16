#pragma once
#include "game_cl_teamdeathmatch.h"

class CUIGameAHunt;

class game_cl_ArtefactHunt :public game_cl_TeamDeathmatch
{
	friend class CUIMessagesWindow;
	CUIGameAHunt*						m_game_ui;	
	shared_str							m_Eff_Af_Spawn;
	shared_str							m_Eff_Af_Disappear;
	typedef game_cl_TeamDeathmatch inherited;

protected:
	virtual const shared_str			GetBaseCostSect			() {return "artefacthunt_base_cost";}
	virtual			void				TranslateGameMessage	(u32 msg, NET_Packet& P);
	virtual			void				shedule_Update			(u32 dt);

	virtual			BOOL				CanCallBuyMenu			();
	virtual			bool				CanBeReady				();
	virtual			void				UpdateMapLocations		();

	virtual			bool				NeedToSendReady_Spectator		(int key, game_PlayerState* ps);
	virtual			void				LoadSndMessages				();

	virtual			void				OnBuySpawnMenu_Ok		();	
public:
	u8									artefactsNum;//ah
	u16									artefactBearerID;//ah,ZoneMap
	u16									old_artefactBearerID;
	u8									teamInPossession;//ah,ZoneMap
	u8									old_teamInPossession;
	u16									artefactID;
	u16									old_artefactID;
	s32									iReinforcementTime;
	s32									dReinforcementTime;

	int									m_iSpawn_Cost;

public :
										game_cl_ArtefactHunt	();
	virtual								~game_cl_ArtefactHunt	();
	virtual			void				Init					();
	virtual			CUIGameCustom*		createGameUI			();
	virtual			void				SetGameUI				(CUIGameCustom*);
	virtual			void				net_import_state		(NET_Packet& P);
	virtual			void				GetMapEntities(xr_vector<SZoneMapEntityData>& dst);
	virtual			char*				getTeamSection			(int Team);
	virtual			bool				PlayerCanSprint			(CActor* pActor);

	virtual	void						SetScore				();
	virtual			void				OnSellItemsFromRuck		();

	virtual			void				OnSpawn					(CObject* pObj);
	virtual			void				OnDestroy				(CObject* pObj);	
	virtual			void				SendPickUpEvent			(u16 ID_who, u16 ID_what);
	virtual		void					OnConnected				();
};
