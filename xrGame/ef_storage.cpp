////////////////////////////////////////////////////////////////////////////
//	Module 		: ef_storage.cpp
//	Created 	: 25.03.2002
//  Modified 	: 11.10.2002
//	Author		: Dmitriy Iassenev
//	Description : Evaluation functions storage class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ef_storage.h"
#include "ef_primary.h"
#include "ef_pattern.h"

CEF_Storage::CEF_Storage()
{	
	ZeroMemory							(m_fpaBaseFunctions,sizeof(m_fpaBaseFunctions));
	
	m_fpaBaseFunctions[0]				= m_pfDistance				= xr_new<CDistanceFunction>				(this);
	m_fpaBaseFunctions[1]				= m_pfGraphPointType0		= xr_new<CGraphPointType0>				(this);
	m_fpaBaseFunctions[2]				= m_pfEquipmentType			= xr_new<CEquipmentType>				(this);
	m_fpaBaseFunctions[3]				= m_pfItemDeterioration		= xr_new<CItemDeterioration>			(this);
	m_fpaBaseFunctions[4]				= m_pfEquipmentPreference	= xr_new<CEquipmentPreference>			(this);
	m_fpaBaseFunctions[5]				= m_pfMainWeaponType		= xr_new<CMainWeaponType>				(this);
	m_fpaBaseFunctions[6]				= m_pfMainWeaponPreference	= xr_new<CMainWeaponPreference>			(this);
	m_fpaBaseFunctions[7]				= m_pfItemValue				= xr_new<CItemValue>					(this);
	m_fpaBaseFunctions[8]				= m_pfWeaponAmmoCount		= xr_new<CWeaponAmmoCount>				(this);
	m_fpaBaseFunctions[9]				= m_pfDetectorType			= xr_new<CDetectorType>					(this);
										  
	m_fpaBaseFunctions[21]				= m_pfPersonalHealth		= xr_new<CPersonalHealthFunction>		(this);
	m_fpaBaseFunctions[22]				= m_pfPersonalMorale		= xr_new<CPersonalMoraleFunction>		(this);			
	m_fpaBaseFunctions[23]				= m_pfPersonalCreatureType	= xr_new<CPersonalCreatureTypeFunction>	(this);	
	m_fpaBaseFunctions[24]				= m_pfPersonalWeaponType	= xr_new<CPersonalWeaponTypeFunction>	(this);		
	m_fpaBaseFunctions[25]				= m_pfPersonalAccuracy		= xr_new<CPersonalAccuracyFunction>		(this);		
	m_fpaBaseFunctions[26]				= m_pfPersonalIntelligence	= xr_new<CPersonalIntelligenceFunction>	(this);	
	m_fpaBaseFunctions[27]				= m_pfPersonalRelation		= xr_new<CPersonalRelationFunction>		(this);		
	m_fpaBaseFunctions[28]				= m_pfPersonalGreed			= xr_new<CPersonalGreedFunction>		(this);			
	m_fpaBaseFunctions[29]				= m_pfPersonalAggressiveness= xr_new<CPersonalAggressivenessFunction>(this);	
	m_fpaBaseFunctions[30]				= m_pfPersonalEyeRange		= xr_new<CPersonalEyeRange>				(this);	
	m_fpaBaseFunctions[31]				= m_pfPersonalMaxHealth		= xr_new<CPersonalMaxHealth>			(this);	
										  
	m_fpaBaseFunctions[41]				= m_pfEnemyHealth			= xr_new<CEnemyHealthFunction>			(this);			
	m_fpaBaseFunctions[42]				= m_pfEnemyCreatureType		= xr_new<CEnemyCreatureTypeFunction>	(this);		
	m_fpaBaseFunctions[43]				= m_pfEnemyWeaponType		= xr_new<CEnemyWeaponTypeFunction>		(this);		
	m_fpaBaseFunctions[44]				= m_pfEnemyEquipmentCost	= xr_new<CEnemyEquipmentCostFunction>	(this);		
	m_fpaBaseFunctions[45]				= m_pfEnemyRukzakWeight		= xr_new<CEnemyRukzakWeightFunction>	(this);		
	m_fpaBaseFunctions[46]				= m_pfEnemyAnomality		= xr_new<CEnemyAnomalityFunction>		(this);			
	m_fpaBaseFunctions[47]				= m_pfEnemyEyeRange			= xr_new<CEnemyEyeRange>				(this);	
	m_fpaBaseFunctions[48]				= m_pfEnemyMaxHealth		= xr_new<CEnemyMaxHealth>				(this);	
	m_fpaBaseFunctions[49]				= m_pfEnemyAnomalyType		= xr_new<CEnemyAnomalyType>				(this);	
	m_fpaBaseFunctions[50]				= m_pfEnemyDistanceToGraphPoint= xr_new<CEnemyDistanceToGraphPoint>	(this);	

	m_pfWeaponEffectiveness				= xr_new<CPatternFunction>	("common\\WeaponEffectiveness.efd",		this);
	m_pfCreatureEffectiveness			= xr_new<CPatternFunction>	("common\\CreatureEffectiveness.efd",	this);
	m_pfIntellectCreatureEffectiveness	= xr_new<CPatternFunction>	("common\\IntCreatureEffectiveness.efd",this);
	m_pfAccuracyWeaponEffectiveness		= xr_new<CPatternFunction>	("common\\AccWeaponEffectiveness.efd",	this);
	m_pfFinalCreatureEffectiveness		= xr_new<CPatternFunction>	("common\\FinCreatureEffectiveness.efd",this);
	m_pfVictoryProbability				= xr_new<CPatternFunction>	("common\\VictoryProbability.efd",		this);
	m_pfEntityCost						= xr_new<CPatternFunction>	("common\\EntityCost.efd",				this);
	m_pfExpediency						= xr_new<CPatternFunction>	("common\\Expediency.efd",				this);
	m_pfSurgeDeathProbability			= xr_new<CPatternFunction>	("common\\SurgeDeathProbability.efd",	this);
	m_pfEquipmentValue					= xr_new<CPatternFunction>	("common\\EquipmentValue.efd",			this);
	m_pfMainWeaponValue					= xr_new<CPatternFunction>	("common\\MainWeaponValue.efd",			this);
	m_pfSmallWeaponValue				= xr_new<CPatternFunction>	("common\\SmallWeaponValue.efd",		this);
	m_pfTerrainType						= xr_new<CPatternFunction>	("alife\\TerrainType.efd",				this);
	m_pfWeaponAttackTimes				= xr_new<CPatternFunction>	("alife\\WeaponAttackTimes.efd",		this);
	m_pfWeaponSuccessProbability		= xr_new<CPatternFunction>	("alife\\WeaponSuccessProbability.efd",	this);
	m_pfEnemyDetectability				= xr_new<CPatternFunction>	("alife\\EnemyDetectability.efd",		this);
	m_pfEnemyDetectProbability			= xr_new<CPatternFunction>	("alife\\EnemyDetectProbability.efd",	this);
	m_pfEnemyRetreatProbability			= xr_new<CPatternFunction>	("alife\\EnemyRetreatProbability.efd",	this);
	m_pfAnomalyDetectProbability		= xr_new<CPatternFunction>	("alife\\AnomalyDetectProbability.efd",	this);
	m_pfAnomalyInteractProbability		= xr_new<CPatternFunction>	("alife\\AnomalyInteractProbability.efd",this);
	m_pfAnomalyRetreatProbability		= xr_new<CPatternFunction>	("alife\\AnomalyRetreatProbability.efd",this);
	m_pfBirthPercentage					= xr_new<CPatternFunction>	("alife\\BirthPercentage.efd",			this);
	m_pfBirthProbability				= xr_new<CPatternFunction>	("alife\\BirthProbability.efd",			this);
	m_pfBirthSpeed						= xr_new<CPatternFunction>	("alife\\BirthSpeed.efd",				this);
}

CEF_Storage::~CEF_Storage()
{
	for (int i=0; i<AI_MAX_EVALUATION_FUNCTION_COUNT; ++i)
		xr_delete						(m_fpaBaseFunctions[i]);
}

CBaseFunction *CEF_Storage::function	(LPCSTR function) const
{
	for (int i=0; i<AI_MAX_EVALUATION_FUNCTION_COUNT; ++i) {
		if (!m_fpaBaseFunctions[i])
			continue;
		if (!xr_strcmp(function,m_fpaBaseFunctions[i]->Name()))
			return						(m_fpaBaseFunctions[i]);
	}
	return								(0);
}
