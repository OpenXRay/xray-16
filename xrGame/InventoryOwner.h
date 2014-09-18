/////////////////////////////////////////////////////
// Для персонажей, имеющих инвентарь
// InventoryOwner.h
//////////////////////////////////////////////////////

#pragma once
#include "InfoPortionDefs.h"
#include "pda_space.h"
#include "attachment_owner.h"
#include "script_space_forward.h"
#include "character_info.h"
#include "inventory_space.h"

class CSE_Abstract;
class CInventory;
class CInventoryItem;
class CTrade;
class CPda;
class CGameObject;
class CEntityAlive;
class CCustomZone;
class CInfoPortionWrapper;
class NET_Packet;
class CCharacterInfo;
class CSpecificCharacter;
class CTradeParameters;
class CPurchaseList;
class CWeapon;
class CCustomOutfit;

class CInventoryOwner : public CAttachmentOwner {							
public:
					CInventoryOwner				();
	virtual			~CInventoryOwner			();

public:
	virtual CInventoryOwner*	cast_inventory_owner	()						{return this;}
public:

	virtual DLL_Pure	*_construct				();
	virtual BOOL		net_Spawn				(CSE_Abstract* DC);
	virtual void		net_Destroy				();
			void		Init					();
	virtual void		Load					(LPCSTR section);
	virtual void		reinit					();
	virtual void		reload					(LPCSTR section);
	virtual void		OnEvent					(NET_Packet& P, u16 type);

	//serialization
	virtual void	save						(NET_Packet &output_packet);
	virtual void	load						(IReader &input_packet);

	
	//обновление
	virtual void	UpdateInventoryOwner		(u32 deltaT);
	virtual bool	CanPutInSlot				(PIItem item, u32 slot){return true;};


	CPda* GetPDA		() const;


	// инвентарь
	CInventory	*m_inventory;			
	
	////////////////////////////////////
	//торговля и общение с персонажем

	virtual bool	AllowItemToTrade 	(CInventoryItem const * item, const SInvItemPlace& place) const;
	virtual void	OnFollowerCmd		(int cmd)		{};//redefine for CAI_Stalkker
			bool	bDisableBreakDialog;
	//инициализация объекта торговли
	CTrade* GetTrade();

	//для включения разговора
	virtual bool OfferTalk		(CInventoryOwner* talk_partner);
	virtual void StartTalk		(CInventoryOwner* talk_partner, bool start_trade = true);
	virtual void StopTalk		();
	virtual bool IsTalking		();

			void StartTrading	();
			void StopTrading	();
			bool IsTrading		();
	
	virtual void EnableTalk		()		{m_bAllowTalk = true;}
	virtual void DisableTalk	()		{m_bAllowTalk = false;}
	virtual bool IsTalkEnabled	()		{ return m_bAllowTalk;}

	void EnableTrade			()		{m_bAllowTrade = true;}
	void DisableTrade			()		{m_bAllowTrade = false;}
	bool IsTradeEnabled			()		{ return m_bAllowTrade;}

	void EnableInvUpgrade		()		{m_bAllowInvUpgrade = true;}
	void DisableInvUpgrade		()		{m_bAllowInvUpgrade = false;}
	bool IsInvUpgradeEnabled	()		{ return m_bAllowInvUpgrade;}

	CInventoryOwner* GetTalkPartner()	{return m_pTalkPartner;}
	virtual void	 NewPdaContact		(CInventoryOwner*);
	virtual void	 LostPdaContact		(CInventoryOwner*);

	//игровое имя 
	virtual LPCSTR	Name        () const;
	LPCSTR				IconName		() const;
	u32					get_money		() const				{return m_money;}
	void				set_money		(u32 amount, bool bSendEvent);
	bool				is_alive		();

protected:
	u32					m_money;
	// торговля
	CTrade*				m_pTrade;
	bool				m_bTrading;
	bool				m_bTalking; 
	CInventoryOwner*	m_pTalkPartner;

	bool				m_bAllowTalk;
	bool				m_bAllowTrade;
	bool				m_bAllowInvUpgrade;

	u16					m_tmp_active_slot_num;
	
	bool				m_play_show_hide_reload_sounds;
	//////////////////////////////////////////////////////////////////////////
	// сюжетная информация
public:
	//персонаж получил новую порцию информации
	virtual bool OnReceiveInfo	(shared_str info_id) const;
	//убрать информацию
	virtual void OnDisableInfo	(shared_str info_id) const;
	//передать/удалить информацию через сервер
	virtual void TransferInfo	(shared_str info_id, bool add_info) const;
	//есть ли информация у персонажа
	virtual bool				HasInfo		(shared_str info_id) const;
//	virtual bool				GetInfo		(shared_str info_id, INFO_DATA&) const;

	#ifdef DEBUG
	void CInventoryOwner::DumpInfo() const;
	#endif

	CInfoPortionWrapper			*m_known_info_registry;

	//////////////////////////////////////////////////////////////////////////
	// инвентарь 
public:
	const CInventory &inventory() const {VERIFY (m_inventory); return(*m_inventory);}
	CInventory		 &inventory()		{VERIFY (m_inventory); return(*m_inventory);}

	//возвращает текуший разброс стрельбы (в радианах) с учетом движения
	virtual float GetWeaponAccuracy			() const;
	//максимальный переносимы вес
	virtual float MaxCarryWeight			() const;

	CCustomOutfit* GetOutfit				() const;

	bool CanPlayShHdRldSounds				() const {return m_play_show_hide_reload_sounds;};
	void SetPlayShHdRldSounds				(bool play) {m_play_show_hide_reload_sounds = play;};
//////////////////////////////////////////////////////////////////////////
	//игровые характеристики персонажа
public:
	CCharacterInfo&						CharacterInfo		() const {VERIFY(m_pCharacterInfo); return *m_pCharacterInfo;}
	IC const CSpecificCharacter&		SpecificCharacter	() const {return CharacterInfo().m_SpecificCharacter;};
	bool								InfinitiveMoney		()	{return CharacterInfo().m_SpecificCharacter.MoneyDef().inf_money;}

	//установка группировки на клиентском и серверном объкте
	virtual void			SetCommunity	(CHARACTER_COMMUNITY_INDEX);
	virtual void			SetRank			(CHARACTER_RANK_VALUE);
	virtual void			ChangeRank		(CHARACTER_RANK_VALUE);
	virtual void			SetReputation	(CHARACTER_REPUTATION_VALUE);
	virtual void			ChangeReputation(CHARACTER_REPUTATION_VALUE);

	//для работы с relation system
	u16								object_id	() const;
	CHARACTER_COMMUNITY_INDEX		Community	() const {return CharacterInfo().Community().index();};
	CHARACTER_RANK_VALUE			Rank		() const {return CharacterInfo().Rank().value();};
	CHARACTER_REPUTATION_VALUE		Reputation	() const {return CharacterInfo().Reputation().value();};
	float							Sympathy	() const {return CharacterInfo().Sympathy(); }

protected:
	CCharacterInfo*			m_pCharacterInfo;
	xr_string				m_game_name;

public:
	virtual void			renderable_Render		();
	virtual void			OnItemTake				(CInventoryItem *inventory_item);
	
	virtual void			OnItemBelt				(CInventoryItem *inventory_item, const SInvItemPlace& previous_place);
	virtual void			OnItemRuck				(CInventoryItem *inventory_item, const SInvItemPlace& previous_place);
	virtual void			OnItemSlot				(CInventoryItem *inventory_item, const SInvItemPlace& previous_place);
	
	virtual void			OnItemDrop				(CInventoryItem *inventory_item, bool just_before_destroy);
	virtual void			OnItemDropUpdate		();
	virtual bool			use_bolts				() const {return(true);}
	virtual	void			spawn_supplies			();

protected:
	shared_str					m_item_to_spawn;
	u32							m_ammo_in_box_to_spawn;

public:
	IC		const shared_str	&item_to_spawn			() const {return m_item_to_spawn;}
	IC		const u32			&ammo_in_box_to_spawn	() const {return m_ammo_in_box_to_spawn;}

public:
	virtual bool				unlimited_ammo			()	= 0;
	virtual	void				on_weapon_shot_start	(CWeapon *weapon);
	virtual	void				on_weapon_shot_update	();
	virtual	void				on_weapon_shot_stop		();
	virtual	void				on_weapon_shot_remove	(CWeapon *weapon);
	virtual	void				on_weapon_hide			(CWeapon *weapon);

public:
	virtual	bool				use_simplified_visual	() const {return (false);};

private:
	CTradeParameters			*m_trade_parameters;
	CPurchaseList				*m_purchase_list;
	BOOL						m_need_osoznanie_mode;
	bool						m_deadbody_can_take;
	bool						m_deadbody_closed;

public:
	IC		CTradeParameters	&trade_parameters		() const;
	virtual	LPCSTR				trade_section			() const;
			float				deficit_factor			(const shared_str &section) const;
			void				buy_supplies			(CInifile &ini_file, LPCSTR section);
			void				sell_useless_items		();
	virtual	void				on_before_sell			(CInventoryItem *item) {}
	virtual	void				on_before_buy			(CInventoryItem *item) {}
	virtual bool				can_use_dynamic_lights	() {return true;}
	virtual	bool				use_default_throw_force	();
	virtual	float				missile_throw_force		(); 
	virtual	bool				use_throw_randomness	();
	virtual bool				NeedOsoznanieMode		() {return m_need_osoznanie_mode!=FALSE;}

			void				deadbody_can_take		(bool status);
	IC		bool				deadbody_can_take_status() const { return m_deadbody_can_take; }
			void				deadbody_closed			(bool status);
	IC		bool				deadbody_closed_status	() const { return m_deadbody_closed; }
};

#include "inventory_owner_inline.h"