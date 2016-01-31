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
	
	m_fpaBaseFunctions[0]				= m_pfDistance				= new CDistanceFunction				(this);
	m_fpaBaseFunctions[1]				= m_pfGraphPointType0		= new CGraphPointType0				(this);
	m_fpaBaseFunctions[2]				= m_pfEquipmentType			= new CEquipmentType				(this);
	m_fpaBaseFunctions[3]				= m_pfItemDeterioration		= new CItemDeterioration			(this);
	m_fpaBaseFunctions[4]				= m_pfEquipmentPreference	= new CEquipmentPreference			(this);
	m_fpaBaseFunctions[5]				= m_pfMainWeaponType		= new CMainWeaponType				(this);
	m_fpaBaseFunctions[6]				= m_pfMainWeaponPreference	= new CMainWeaponPreference			(this);
	m_fpaBaseFunctions[7]				= m_pfItemValue				= new CItemValue					(this);
	m_fpaBaseFunctions[8]				= m_pfWeaponAmmoCount		= new CWeaponAmmoCount				(this);
	m_fpaBaseFunctions[9]				= m_pfDetectorType			= new CDetectorType					(this);
										  
	m_fpaBaseFunctions[21]				= m_pfPersonalHealth		= new CPersonalHealthFunction		(this);
	m_fpaBaseFunctions[22]				= m_pfPersonalMorale		= new CPersonalMoraleFunction		(this);			
	m_fpaBaseFunctions[23]				= m_pfPersonalCreatureType	= new CPersonalCreatureTypeFunction	(this);	
	m_fpaBaseFunctions[24]				= m_pfPersonalWeaponType	= new CPersonalWeaponTypeFunction	(this);		
	m_fpaBaseFunctions[25]				= m_pfPersonalAccuracy		= new CPersonalAccuracyFunction		(this);		
	m_fpaBaseFunctions[26]				= m_pfPersonalIntelligence	= new CPersonalIntelligenceFunction	(this);	
	m_fpaBaseFunctions[27]				= m_pfPersonalRelation		= new CPersonalRelationFunction		(this);		
	m_fpaBaseFunctions[28]				= m_pfPersonalGreed			= new CPersonalGreedFunction		(this);			
	m_fpaBaseFunctions[29]				= m_pfPersonalAggressiveness= new CPersonalAggressivenessFunction(this);	
	m_fpaBaseFunctions[30]				= m_pfPersonalEyeRange		= new CPersonalEyeRange				(this);	
	m_fpaBaseFunctions[31]				= m_pfPersonalMaxHealth		= new CPersonalMaxHealth			(this);	
										  
	m_fpaBaseFunctions[41]				= m_pfEnemyHealth			= new CEnemyHealthFunction			(this);			
	m_fpaBaseFunctions[42]				= m_pfEnemyCreatureType		= new CEnemyCreatureTypeFunction	(this);		
	m_fpaBaseFunctions[43]				= m_pfEnemyWeaponType		= new CEnemyWeaponTypeFunction		(this);		
	m_fpaBaseFunctions[44]				= m_pfEnemyEquipmentCost	= new CEnemyEquipmentCostFunction	(this);		
	m_fpaBaseFunctions[45]				= m_pfEnemyRukzakWeight		= new CEnemyRukzakWeightFunction	(this);		
	m_fpaBaseFunctions[46]				= m_pfEnemyAnomality		= new CEnemyAnomalityFunction		(this);			
	m_fpaBaseFunctions[47]				= m_pfEnemyEyeRange			= new CEnemyEyeRange				(this);	
	m_fpaBaseFunctions[48]				= m_pfEnemyMaxHealth		= new CEnemyMaxHealth				(this);	
	m_fpaBaseFunctions[49]				= m_pfEnemyAnomalyType		= new CEnemyAnomalyType				(this);	
	m_fpaBaseFunctions[50]				= m_pfEnemyDistanceToGraphPoint= new CEnemyDistanceToGraphPoint	(this);	

	m_pfWeaponEffectiveness				= new CPatternFunction	("common\\WeaponEffectiveness.efd",		this);
	m_pfCreatureEffectiveness			= new CPatternFunction	("common\\CreatureEffectiveness.efd",	this);
	m_pfIntellectCreatureEffectiveness	= new CPatternFunction	("common\\IntCreatureEffectiveness.efd",this);
	m_pfAccuracyWeaponEffectiveness		= new CPatternFunction	("common\\AccWeaponEffectiveness.efd",	this);
	m_pfFinalCreatureEffectiveness		= new CPatternFunction	("common\\FinCreatureEffectiveness.efd",this);
	m_pfVictoryProbability				= new CPatternFunction	("common\\VictoryProbability.efd",		this);
	m_pfEntityCost						= new CPatternFunction	("common\\EntityCost.efd",				this);
	m_pfExpediency						= new CPatternFunction	("common\\Expediency.efd",				this);
	m_pfSurgeDeathProbability			= new CPatternFunction	("common\\SurgeDeathProbability.efd",	this);
	m_pfEquipmentValue					= new CPatternFunction	("common\\EquipmentValue.efd",			this);
	m_pfMainWeaponValue					= new CPatternFunction	("common\\MainWeaponValue.efd",			this);
	m_pfSmallWeaponValue				= new CPatternFunction	("common\\SmallWeaponValue.efd",		this);
	m_pfTerrainType						= new CPatternFunction	("alife\\TerrainType.efd",				this);
	m_pfWeaponAttackTimes				= new CPatternFunction	("alife\\WeaponAttackTimes.efd",		this);
	m_pfWeaponSuccessProbability		= new CPatternFunction	("alife\\WeaponSuccessProbability.efd",	this);
	m_pfEnemyDetectability				= new CPatternFunction	("alife\\EnemyDetectability.efd",		this);
	m_pfEnemyDetectProbability			= new CPatternFunction	("alife\\EnemyDetectProbability.efd",	this);
	m_pfEnemyRetreatProbability			= new CPatternFunction	("alife\\EnemyRetreatProbability.efd",	this);
	m_pfAnomalyDetectProbability		= new CPatternFunction	("alife\\AnomalyDetectProbability.efd",	this);
	m_pfAnomalyInteractProbability		= new CPatternFunction	("alife\\AnomalyInteractProbability.efd",this);
	m_pfAnomalyRetreatProbability		= new CPatternFunction	("alife\\AnomalyRetreatProbability.efd",this);
	m_pfBirthPercentage					= new CPatternFunction	("alife\\BirthPercentage.efd",			this);
	m_pfBirthProbability				= new CPatternFunction	("alife\\BirthProbability.efd",			this);
	m_pfBirthSpeed						= new CPatternFunction	("alife\\BirthSpeed.efd",				this);
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
