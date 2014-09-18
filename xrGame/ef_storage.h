////////////////////////////////////////////////////////////////////////////
//	Module 		: ef_storage.h
//	Created 	: 25.03.2002
//  Modified 	: 11.10.2002
//	Author		: Dmitriy Iassenev
//	Description : Evaluation functions storage class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

#define AI_MAX_EVALUATION_FUNCTION_COUNT	128

class CGameObject;
class CEntityAlive;
class CSE_ALifeSchedulable;
class CSE_ALifeObject;
class CBaseFunction;
class CPatternFunction;

class CDistanceFunction;
class CGraphPointType0;
class CEquipmentType;
class CItemDeterioration;
class CEquipmentPreference;
class CMainWeaponType;
class CMainWeaponPreference;
class CItemValue;
class CWeaponAmmoCount;
class CDetectorType;
class CPersonalHealthFunction;
class CPersonalMoraleFunction;
class CPersonalCreatureTypeFunction;
class CPersonalWeaponTypeFunction;
class CPersonalAccuracyFunction;
class CPersonalIntelligenceFunction;
class CPersonalRelationFunction;
class CPersonalGreedFunction;
class CPersonalAggressivenessFunction;
class CPersonalEyeRange;
class CPersonalMaxHealth;
class CEnemyHealthFunction;
class CEnemyCreatureTypeFunction;
class CEnemyWeaponTypeFunction;
class CEnemyEquipmentCostFunction;
class CEnemyRukzakWeightFunction;
class CEnemyAnomalityFunction;
class CEnemyEyeRange;
class CEnemyMaxHealth;
class CEnemyAnomalyType;
class CEnemyDistanceToGraphPoint;

template <typename T1, typename T2>
class CEF_Params {
private:
	T1					*m_member;
	T1					*m_enemy;
	T2					*m_member_item;
	T2					*m_enemy_item;

public:
	IC			CEF_Params	()
	{
		clear			();
	}

	IC	void	clear		()
	{
		m_member		= 0;
		m_enemy			= 0;
		m_member_item	= 0;
		m_enemy_item	= 0;
	}

	IC	T1*&	member		()
	{
		return			(m_member);
	}

	IC	T1*&	enemy		()
	{
		return			(m_enemy);
	}

	IC	T2*&	member_item	()
	{
		return			(m_member_item);
	}

	IC	T2*&	enemy_item	()
	{
		return			(m_enemy_item);
	}
};

typedef CEF_Params<const CEntityAlive,const CGameObject>		CNonALifeParams;
typedef CEF_Params<CSE_ALifeSchedulable,const CSE_ALifeObject>	CALifeParams;

class CEF_Storage;

template <typename T>
struct CEnemyFunction : public T {
	IC				CEnemyFunction	(CEF_Storage *storage) : T(storage)
	{
	}

	template <typename P>
	IC		float	get_value		(P &params)
	{
		P						save = params;
		params.member()			= params.enemy();
		params.member_item()	= params.enemy_item();
		float					value = T::ffGetValue();
		params					= save;
		return					(value);
	}

	virtual float	ffGetValue		()
	{
		if (ef_storage().non_alife().member())
			return	(get_value(ef_storage().non_alife()));
		return		(get_value(ef_storage().alife()));
	}
};

class CEF_Storage {
public:
	typedef CEnemyFunction<CPersonalHealthFunction>			CEnemyHealthFunction;
	typedef CEnemyFunction<CPersonalCreatureTypeFunction>	CEnemyCreatureTypeFunction;
	typedef CEnemyFunction<CPersonalWeaponTypeFunction>		CEnemyWeaponTypeFunction;
	typedef CEnemyFunction<CPersonalEyeRange>				CEnemyEyeRange;
	typedef CEnemyFunction<CPersonalMaxHealth>				CEnemyMaxHealth;

public:
	CNonALifeParams							m_non_alife_params;
	CALifeParams							m_alife_params;
	// primary functions
	CBaseFunction							*m_fpaBaseFunctions		[AI_MAX_EVALUATION_FUNCTION_COUNT];

	CDistanceFunction						*m_pfDistance;
	CGraphPointType0						*m_pfGraphPointType0;
	CEquipmentType							*m_pfEquipmentType;
	CItemDeterioration						*m_pfItemDeterioration;
	CEquipmentPreference					*m_pfEquipmentPreference;
	CMainWeaponType							*m_pfMainWeaponType;
	CMainWeaponPreference					*m_pfMainWeaponPreference;
	CItemValue								*m_pfItemValue;
	CWeaponAmmoCount						*m_pfWeaponAmmoCount;
	CDetectorType							*m_pfDetectorType;

	CPersonalHealthFunction					*m_pfPersonalHealth;
	CPersonalMoraleFunction					*m_pfPersonalMorale;
	CPersonalCreatureTypeFunction			*m_pfPersonalCreatureType;
	CPersonalWeaponTypeFunction				*m_pfPersonalWeaponType;
	CPersonalAccuracyFunction				*m_pfPersonalAccuracy;
	CPersonalIntelligenceFunction			*m_pfPersonalIntelligence;
	CPersonalRelationFunction				*m_pfPersonalRelation;
	CPersonalGreedFunction					*m_pfPersonalGreed;
	CPersonalAggressivenessFunction			*m_pfPersonalAggressiveness;
	CPersonalEyeRange						*m_pfPersonalEyeRange;
	CPersonalMaxHealth						*m_pfPersonalMaxHealth;

	CEnemyHealthFunction					*m_pfEnemyHealth;
	CEnemyCreatureTypeFunction				*m_pfEnemyCreatureType;
	CEnemyWeaponTypeFunction				*m_pfEnemyWeaponType;
	CEnemyEquipmentCostFunction				*m_pfEnemyEquipmentCost;
	CEnemyRukzakWeightFunction				*m_pfEnemyRukzakWeight;
	CEnemyAnomalityFunction					*m_pfEnemyAnomality;
	CEnemyEyeRange							*m_pfEnemyEyeRange;
	CEnemyMaxHealth							*m_pfEnemyMaxHealth;
	CEnemyAnomalyType						*m_pfEnemyAnomalyType;
	CEnemyDistanceToGraphPoint				*m_pfEnemyDistanceToGraphPoint;

	// complex functions
	CPatternFunction						*m_pfWeaponEffectiveness;
	CPatternFunction						*m_pfCreatureEffectiveness;
	CPatternFunction						*m_pfIntellectCreatureEffectiveness;
	CPatternFunction						*m_pfAccuracyWeaponEffectiveness;
	CPatternFunction						*m_pfFinalCreatureEffectiveness;
	CPatternFunction						*m_pfVictoryProbability;
	CPatternFunction						*m_pfEntityCost;
	CPatternFunction						*m_pfExpediency;
	CPatternFunction						*m_pfSurgeDeathProbability;
	CPatternFunction						*m_pfEquipmentValue;
	CPatternFunction						*m_pfMainWeaponValue;
	CPatternFunction						*m_pfSmallWeaponValue;
	CPatternFunction						*m_pfTerrainType;
	CPatternFunction						*m_pfWeaponAttackTimes;
	CPatternFunction						*m_pfWeaponSuccessProbability;
	CPatternFunction						*m_pfEnemyDetectability;
	CPatternFunction						*m_pfEnemyDetectProbability;
	CPatternFunction						*m_pfEnemyRetreatProbability;
	CPatternFunction						*m_pfAnomalyDetectProbability;
	CPatternFunction						*m_pfAnomalyInteractProbability;
	CPatternFunction						*m_pfAnomalyRetreatProbability;
	CPatternFunction						*m_pfBirthPercentage;
	CPatternFunction						*m_pfBirthProbability;
	CPatternFunction						*m_pfBirthSpeed;

											CEF_Storage		();
	virtual									~CEF_Storage	();
			CBaseFunction					*function		(LPCSTR function) const;
	IC		void							alife_evaluation(bool value);
	IC		CNonALifeParams					&non_alife		();
	IC		CALifeParams					&alife			();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CEF_Storage)
#undef script_type_list
#define script_type_list save_type_list(CEF_Storage)

#include "ef_storage_inline.h"