// ActorCondition.h: класс состояния игрока
//

#pragma once

#include "EntityCondition.h"
#include "actor_defs.h"

template <typename _return_type>
class CScriptCallbackEx;
class CActor;
class CActorDeathEffector;

class CActorCondition: public CEntityCondition {
private:
	typedef CEntityCondition inherited;
	enum {	eCriticalPowerReached			=(1<<0),
			eCriticalMaxPowerReached		=(1<<1),
			eCriticalBleedingSpeed			=(1<<2),
			eCriticalSatietyReached			=(1<<3),
			eCriticalRadiationReached		=(1<<4),
			eWeaponJammedReached			=(1<<5),
			ePhyHealthMinReached			=(1<<6),
			eCantWalkWeight					=(1<<7),
			eCantWalkWeightReached			=(1<<8),
			};
	Flags16											m_condition_flags;
private:
	CActor*											m_object;
	CActorDeathEffector*							m_death_effector;
	void				UpdateTutorialThresholds	();
			void 		UpdateSatiety				();
	virtual void		UpdateRadiation				();
public:
						CActorCondition				(CActor *object);
	virtual				~CActorCondition			();

	virtual void		LoadCondition				(LPCSTR section);
	virtual void		reinit						();

	virtual CWound*		ConditionHit				(SHit* pHDS);
	virtual void		UpdateCondition				();
			void		UpdateBoosters				();

	virtual void 		ChangeAlcohol				(const float value);
	virtual void 		ChangeSatiety				(const float value);

	void 				BoostParameters				(const SBooster& B);
	void 				DisableBoostParameters		(const SBooster& B);
	IC void				BoostMaxWeight				(const float value);
	IC void				BoostHpRestore				(const float value);
	IC void				BoostPowerRestore			(const float value);
	IC void				BoostRadiationRestore		(const float value);
	IC void				BoostBleedingRestore		(const float value);
	IC void				BoostBurnImmunity			(const float value);
	IC void				BoostShockImmunity			(const float value);
	IC void				BoostRadiationImmunity		(const float value);
	IC void				BoostTelepaticImmunity		(const float value);
	IC void				BoostChemicalBurnImmunity	(const float value);
	IC void				BoostExplImmunity			(const float value);
	IC void				BoostStrikeImmunity			(const float value);
	IC void				BoostFireWoundImmunity		(const float value);
	IC void				BoostWoundImmunity			(const float value);
	IC void				BoostRadiationProtection	(const float value);
	IC void				BoostTelepaticProtection	(const float value);
	IC void				BoostChemicalBurnProtection	(const float value);
	BOOSTER_MAP			GetCurBoosterInfluences		() {return m_booster_influences;};

	// хромание при потере сил и здоровья
	virtual	bool		IsLimping					() const;
	virtual bool		IsCantWalk					() const;
	virtual bool		IsCantWalkWeight			();
	virtual bool		IsCantSprint				() const;

			void		PowerHit					(float power, bool apply_outfit);
			float		GetPower					() const { return m_fPower; }

			void		ConditionJump				(float weight);
			void		ConditionWalk				(float weight, bool accel, bool sprint);
			void		ConditionStand				(float weight);
	IC		float		MaxWalkWeight				() const	{ return m_MaxWalkWeight; }
			
			float	xr_stdcall	GetAlcohol			()	{return m_fAlcohol;}
			float	xr_stdcall	GetPsy				()	{return 1.0f-GetPsyHealth();}
			float				GetSatiety			()  {return m_fSatiety;}
	IC		float				GetSatietyPower		() const {return m_fV_SatietyPower*m_fSatiety;};

			void		AffectDamage_InjuriousMaterialAndMonstersInfluence();
			float		GetInjuriousMaterialDamage	();
			
			void		SetZoneDanger				(float danger, ALife::EInfluenceType type);
			float		GetZoneDanger				() const;

public:
	IC		CActor		&object						() const
	{
		VERIFY			(m_object);
		return			(*m_object);
	}
	virtual void			save					(NET_Packet &output_packet);
	virtual void			load					(IReader &input_packet);
//	IC		float const&	Satiety					()	{ return m_fSatiety; }
	IC		float const&	V_Satiety				()	{ return m_fV_Satiety; }
	IC		float const&	V_SatietyPower			()	{ return m_fV_SatietyPower; }
	IC		float const&	V_SatietyHealth			()	{ return m_fV_SatietyHealth; }
	IC		float const&	SatietyCritical			()	{ return m_fSatietyCritical; }
	
	float	GetZoneMaxPower							(ALife::EInfluenceType type) const;
	float	GetZoneMaxPower							(ALife::EHitType hit_type) const;

	bool	DisableSprint							(SHit* pHDS);
	bool	PlayHitSound							(SHit* pHDS);
	float	HitSlowmo								(SHit* pHDS);
	virtual bool			ApplyInfluence			(const SMedicineInfluenceValues& V, const shared_str& sect);
	virtual bool			ApplyBooster			(const SBooster& B, const shared_str& sect);
	float	GetMaxPowerRestoreSpeed					() {return m_max_power_restore_speed;};
	float	GetMaxWoundProtection					() {return m_max_wound_protection;};
	float	GetMaxFireWoundProtection				() {return m_max_fire_wound_protection;};

protected:
	SMedicineInfluenceValues						m_curr_medicine_influence;
	float m_fAlcohol;
	float m_fV_Alcohol;
//--
	float m_fSatiety;
	float m_fV_Satiety;
	float m_fV_SatietyPower;
	float m_fV_SatietyHealth;
	float m_fSatietyCritical;
//--
	float m_fPowerLeakSpeed;

	float m_fJumpPower;
	float m_fStandPower;
	float m_fWalkPower;
	float m_fJumpWeightPower;
	float m_fWalkWeightPower;
	float m_fOverweightWalkK;
	float m_fOverweightJumpK;
	float m_fAccelK;
	float m_fSprintK;
	
	float	m_MaxWalkWeight;
	float	m_zone_max_power[ALife::infl_max_count];
	float	m_zone_danger[ALife::infl_max_count];
	float	m_f_time_affected;
	float	m_max_power_restore_speed;
	float	m_max_wound_protection;
	float	m_max_fire_wound_protection;

	mutable bool m_bLimping;
	mutable bool m_bCantWalk;
	mutable bool m_bCantSprint;

	//порог силы и здоровья меньше которого актер начинает хромать
	float m_fLimpingPowerBegin;
	float m_fLimpingPowerEnd;
	float m_fCantWalkPowerBegin;
	float m_fCantWalkPowerEnd;

	float m_fCantSprintPowerBegin;
	float m_fCantSprintPowerEnd;

	float m_fLimpingHealthBegin;
	float m_fLimpingHealthEnd;

	//typedef xr_vector<SMedicineInfluenceValues> BOOSTS_VECTOR;
	//typedef xr_vector<SMedicineInfluenceValues>::iterator BOOSTS_VECTOR_ITER;
	//BOOSTS_VECTOR m_vecBoosts;
	ref_sound m_use_sound;
};

class CActorDeathEffector
{
	CActorCondition*		m_pParent;
	ref_sound				m_death_sound;
	bool					m_b_actual;
	float					m_start_health;
	void xr_stdcall			OnPPEffectorReleased		();
public:
			CActorDeathEffector	(CActorCondition* parent, LPCSTR sect);	// -((
			~CActorDeathEffector();
	void	UpdateCL			();
	IC bool	IsActual			() {return m_b_actual;}
	void	Stop				();
};