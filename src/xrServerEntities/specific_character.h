//////////////////////////////////////////////////////////////////////////
// specific_character.h:	игровая информация для о конкретном 
//							персонажей в игре
//////////////////////////////////////////////////////////////////////////

#pragma		once


#include "character_info_defs.h"
#include "shared_data.h"
#include "xml_str_id_loader.h"


#ifdef XRGAME_EXPORTS

#include "PhraseDialogDefs.h"
#include "character_community.h"

#endif


//////////////////////////////////////////////////////////////////////////
// SSpecificCharacterData: данные о конкретном персонаже
//////////////////////////////////////////////////////////////////////////
struct SSpecificCharacterData : CSharedResource
{

#ifdef  XRGAME_EXPORTS

	SSpecificCharacterData ();
	virtual ~SSpecificCharacterData ();

	//игровое имя персонажа
	xr_string	m_sGameName;
	//текст с биографией персонажа (линк на string table)
	shared_str	m_sBioText;
	//строка содержащая предметы, которые нужно проспавнить 
	xr_string	m_sSupplySpawn;
	//имя секции конфигурации настроек NPC для персонажа
	xr_string	m_sNpcConfigSect;
	//имя секции конфигурации звука для NPC персонажа
	xr_string	m_sound_voice_prefix;

	float		m_fPanic_threshold;
	float		m_fHitProbabilityFactor;
	int			m_crouch_type;
	bool		m_upgrade_mechanic;

	xr_string	m_critical_wound_weights;
#endif
	shared_str	m_terrain_sect;

	//имя модели
	xr_string m_sVisual;

#ifdef  XRGAME_EXPORTS
	
	//начальный диалог
	shared_str					m_StartDialog;
	//диалоги актера, которые будут доступны только при встрече с данным персонажем
	DIALOG_ID_VECTOR			m_ActorDialogs;

	shared_str					m_icon_name;
	//команда 
	CHARACTER_COMMUNITY			m_Community;

#endif
	
	//ранг
	CHARACTER_RANK_VALUE		m_Rank;
	//репутация
	CHARACTER_REPUTATION_VALUE	m_Reputation;

	//классы персонажа (военные-ветераны, ученые и т.д.)
	//к которым он принадлежит
	xr_vector<CHARACTER_CLASS>	m_Classes;


	//указание на то что персонаж не предназначен для случайного выбора
	//и задается только через явное указание ID
	bool m_bNoRandom;
	//если персонаж является заданым по умолчанию для своей команды
	bool m_bDefaultForCommunity;
#ifdef  XRGAME_EXPORTS
	struct SMoneyDef{
		u32				min_money;
		u32				max_money;
		bool			inf_money;
	};
	SMoneyDef			money_def;
#endif
};

class CInventoryOwner;
class CCharacterInfo;
class CSE_ALifeTraderAbstract;


class CSpecificCharacter: public CSharedClass<SSpecificCharacterData, shared_str, false>,
						  public CXML_IdToIndex<CSpecificCharacter>
{
private:
	typedef CSharedClass	<SSpecificCharacterData, shared_str, false>				inherited_shared;
	typedef CXML_IdToIndex	<CSpecificCharacter>									id_to_index;

	friend id_to_index;
	friend CInventoryOwner;
	friend CCharacterInfo;
	friend CSE_ALifeTraderAbstract;
public:

								CSpecificCharacter		();
								~CSpecificCharacter		();

	virtual void				Load					(shared_str		id);

protected:
	const SSpecificCharacterData* data					() const	{ VERIFY(inherited_shared::get_sd()); return inherited_shared::get_sd();}
	SSpecificCharacterData*		  data					()			{ VERIFY(inherited_shared::get_sd()); return inherited_shared::get_sd();}

	//загрузка из XML файла
	virtual void				load_shared				(LPCSTR);
	static void					InitXmlIdToIndex		();

	shared_str		m_OwnId;
public:

#ifdef  XRGAME_EXPORTS
	LPCSTR						Name					() const ;
	shared_str					Bio						() const ;
	const CHARACTER_COMMUNITY&	Community				() const ;
	SSpecificCharacterData::SMoneyDef& MoneyDef			() 	{return data()->money_def;}
#endif

	CHARACTER_RANK_VALUE		Rank					() const ;
	CHARACTER_REPUTATION_VALUE	Reputation				() const ;
	LPCSTR						Visual					() const ;

#ifdef  XRGAME_EXPORTS
	LPCSTR						SupplySpawn				() const ;
	LPCSTR						NpcConfigSect			() const ;
	LPCSTR						sound_voice_prefix		() const ;
	float						panic_threshold			() const ;
	float						hit_probability_factor	() const ;
	int							crouch_type				() const ;
	bool						upgrade_mechanic		() const ;
	LPCSTR						critical_wound_weights	() const ;

	const shared_str&			IconName				() const	{return data()->m_icon_name;};
#endif
	shared_str					terrain_sect			() const;
};


//////////////////////////////////////////////////////////////////////////
