////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_property_evaluators.h
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker property evaluators classes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "wrapper_abstract.h"
#include "property_evaluator_const.h"
#include "property_evaluator_member.h"
#include "danger_object.h"
#include "smart_cover.h"

class CAI_Stalker;

typedef CWrapperAbstract2<CAI_Stalker,CPropertyEvaluator>		CStalkerPropertyEvaluator;
typedef CWrapperAbstract2<CAI_Stalker,CPropertyEvaluatorConst>	CStalkerPropertyEvaluatorConst;
typedef CWrapperAbstract2<CAI_Stalker,CPropertyEvaluatorMember>	CStalkerPropertyEvaluatorMember;

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorALife
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorALife : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorALife	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorAlive
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorAlive : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorAlive	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItems
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorItems : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorItems	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemies
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorEnemies : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;
protected:
	u32					m_time_to_wait;
	const bool			*m_dont_wait;

public:
						CStalkerPropertyEvaluatorEnemies(CAI_Stalker *object = 0, LPCSTR evaluator_name = "", u32 time_to_wait = 0, const bool *dont_wait = 0);
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorSeeEnemy
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorSeeEnemy : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorSeeEnemy	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate							();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemySeeMe
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorEnemySeeMe : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorEnemySeeMe	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate							();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItemToKill
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorItemToKill : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorItemToKill	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate							();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItemCanKill
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorItemCanKill : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorItemCanKill	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorFoundItemToKill
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorFoundItemToKill : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorFoundItemToKill	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorFoundAmmo
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorFoundAmmo : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorFoundAmmo	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToKill
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorReadyToKill : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

private:
	u32					m_min_ammo_count;

public:
						CStalkerPropertyEvaluatorReadyToKill	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "", u32 min_ammo_count = 0);
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToKillSmartCover
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorReadyToKillSmartCover : public CStalkerPropertyEvaluatorReadyToKill {
protected:
	typedef CStalkerPropertyEvaluatorReadyToKill inherited;

public:
						CStalkerPropertyEvaluatorReadyToKillSmartCover	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "", u32 min_ammo_count = 0);
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToDetour
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorReadyToDetour : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorReadyToDetour	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate						();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorAnomaly
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorAnomaly : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorAnomaly	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate							();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorInsideAnomaly
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorInsideAnomaly : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorInsideAnomaly	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate								();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorPanic
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorPanic : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorPanic		(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate							();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorSmartTerrainTask
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorSmartTerrainTask : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorSmartTerrainTask	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyReached
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorEnemyReached : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorEnemyReached	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate								();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorPlayerOnThePath
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorPlayerOnThePath : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorPlayerOnThePath(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate								();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyCriticallyWounded
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorEnemyCriticallyWounded : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorEnemyCriticallyWounded	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate										();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorShouldThrowGrenade
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorShouldThrowGrenade : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorShouldThrowGrenade		(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate										();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorTooFarToKillEnemy
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorTooFarToKillEnemy : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorTooFarToKillEnemy	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate									();
};

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorLowCover
//////////////////////////////////////////////////////////////////////////

class CStalkerPropertyEvaluatorLowCover : public CStalkerPropertyEvaluator {
protected:
	typedef CStalkerPropertyEvaluator inherited;

public:
						CStalkerPropertyEvaluatorLowCover	(CAI_Stalker *object = 0, LPCSTR evaluator_name = "");
	virtual _value_type	evaluate							();
};

#include "stalker_property_evaluators_inline.h"
