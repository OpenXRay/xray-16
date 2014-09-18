#include "stdafx.h"
#include "bloodsucker_alien.h"
#include "bloodsucker.h"
#include "../../../level.h"
#include "../../../Actor.h"
#include "../../../ActorEffector.h"
#include "../../../inventory.h"
#include "../../../HudItem.h"
#include "../../../../xrEngine/CustomHUD.h"

#define EFFECTOR_ID_GEN(type) (type( u32(u64(this) & u32(-1)) ))


////////////////////////////////////////////////////////////////////////////////////
// CAlienEffectorPP
////////////////////////////////////////////////////////////////////////////////////
class CAlienEffectorPP : public CEffectorPP {
	typedef CEffectorPP inherited;

	SPPInfo		state;
	float		factor;
	float		target_factor;

public:
					CAlienEffectorPP	(const SPPInfo &ppi, EEffectorPPType type);
	virtual			~CAlienEffectorPP	();

	void	Update			(float new_factor) {factor = new_factor;}
	void	Destroy			();

private:
	virtual	BOOL	Process			(SPPInfo& pp);
};


CAlienEffectorPP::CAlienEffectorPP(const SPPInfo &ppi, EEffectorPPType type) :
CEffectorPP(type, flt_max, false)
{
	state			= ppi;
	factor			= 0.f;
	target_factor	= 1.f;
}

CAlienEffectorPP::~CAlienEffectorPP()
{
}

#define PERIOD_SPEED	0.3f

BOOL CAlienEffectorPP::Process(SPPInfo& pp)
{
	inherited::Process(pp);

	if (fsimilar(factor, target_factor)) {
		target_factor = (target_factor > 0.5f) ? .3f : .6f;
	}
	
	def_lerp			(factor,target_factor, PERIOD_SPEED, Device.fTimeDelta);
	pp.lerp				(pp_identity,state,factor);

	return TRUE;
}

void CAlienEffectorPP::Destroy()
{
	fLifeTime			= 0.f;
	CAlienEffectorPP	*self = this;
	xr_delete			(self);
}



//////////////////////////////////////////////////////////////////////////
// Alien Camera Effector
//////////////////////////////////////////////////////////////////////////

class CAlienEffector : public CEffectorCam {
	typedef CEffectorCam inherited;	

	float	m_time_total;
	Fvector	dangle_target;
	Fvector dangle_current;

	CAI_Bloodsucker *monster;

	float		m_current_fov;
	Fmatrix		m_prev_eye_matrix;
	float		m_inertion;

public:
					CAlienEffector	(ECamEffectorType type, CAI_Bloodsucker *obj);
	virtual BOOL	ProcessCam		(SCamEffectorInfo& info);
};


#define DELTA_ANGLE_X		10 * PI / 180
#define DELTA_ANGLE_Y		10 * PI / 180
#define DELTA_ANGLE_Z		10 * PI / 180
#define ANGLE_SPEED			0.2f	

#define MIN_FOV				70.f
#define	MAX_FOV				175.f
#define FOV_SPEED			80.f
#define	MAX_CAMERA_DIST		3.5f


CAlienEffector::CAlienEffector(ECamEffectorType type, CAI_Bloodsucker *obj) :
	inherited(type, flt_max)
{
	dangle_target.set		(angle_normalize(Random.randFs(DELTA_ANGLE_X)),angle_normalize(Random.randFs(DELTA_ANGLE_Y)),angle_normalize(Random.randFs(DELTA_ANGLE_Z)));
	dangle_current.set		(0.f, 0.f, 0.f);

	monster					= obj;
	
	m_prev_eye_matrix.c		= get_head_position(monster);
	m_prev_eye_matrix.k		= monster->Direction();
	Fvector::generate_orthonormal_basis(m_prev_eye_matrix.k,m_prev_eye_matrix.j,m_prev_eye_matrix.i);
	m_inertion				= 1.f;
	m_current_fov			= MIN_FOV;
}

BOOL CAlienEffector::ProcessCam(SCamEffectorInfo& info)
{
	// Инициализация
	Fmatrix	Mdef;
	Mdef.identity		();
	Mdef.j.set			(info.n);
	Mdef.k.set			(info.d);
	Mdef.i.crossproduct	(info.n, info.d);
	Mdef.c.set			(info.p);


	// set angle 
	if (angle_lerp(dangle_current.x, dangle_target.x, ANGLE_SPEED, Device.fTimeDelta)) {
		dangle_target.x = angle_normalize(Random.randFs(DELTA_ANGLE_X));
	}

	if (angle_lerp(dangle_current.y, dangle_target.y, ANGLE_SPEED, Device.fTimeDelta)) {
		dangle_target.y = angle_normalize(Random.randFs(DELTA_ANGLE_Y));
	}

	if (angle_lerp(dangle_current.z, dangle_target.z, ANGLE_SPEED, Device.fTimeDelta)) {
		dangle_target.z = angle_normalize(Random.randFs(DELTA_ANGLE_Z));
	}

	// update inertion
	Fmatrix cur_matrix;
	cur_matrix.k = monster->Direction();
	cur_matrix.c = get_head_position(monster);

	float	rel_dist = m_prev_eye_matrix.c.distance_to(cur_matrix.c) / MAX_CAMERA_DIST;
	clamp	(rel_dist, 0.f, 1.f);

	def_lerp(m_inertion, 1 - rel_dist, rel_dist, Device.fTimeDelta);

	// set pos and dir with inertion
	m_prev_eye_matrix.c.inertion(cur_matrix.c, m_inertion);
	m_prev_eye_matrix.k.inertion(cur_matrix.k, m_inertion);
	Fvector::generate_orthonormal_basis_normalized(m_prev_eye_matrix.k,m_prev_eye_matrix.j,m_prev_eye_matrix.i);	

	// apply position and direction
	Mdef = m_prev_eye_matrix;

	//set fov
	float	rel_speed = monster->m_fCurSpeed / 15.f;
	clamp	(rel_speed,0.f,1.f);

	float	m_target_fov = MIN_FOV + (MAX_FOV-MIN_FOV) * rel_speed;
	def_lerp(m_current_fov, m_target_fov, FOV_SPEED, Device.fTimeDelta);
	
	info.fFov = m_current_fov;
	//////////////////////////////////////////////////////////////////////////

	// Установить углы смещения
	Fmatrix		R;
	R.setHPB	(dangle_current.x,dangle_current.y,dangle_current.z);

	Fmatrix		mR;
	mR.mul		(Mdef,R);

	info.d.set		(mR.k);
	info.n.set		(mR.j);
	info.p.set		(mR.c);

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////


CBloodsuckerAlien::CBloodsuckerAlien()
{
	m_object	= 0;
}

CBloodsuckerAlien::~CBloodsuckerAlien()
{
}

void CBloodsuckerAlien::init_external(CAI_Bloodsucker *obj)
{
	m_object	= obj;
}

void CBloodsuckerAlien::reinit()
{
	m_active				= false;	
	m_crosshair_show		= false;
}

void CBloodsuckerAlien::activate()
{
	if (m_active) return;

	VERIFY	(Actor());
	m_object->CControlledActor::install			(Actor());
	m_object->CControlledActor::dont_need_turn	();

	if (!m_object->EnemyMan.get_enemy())	m_object->EnemyMan.add_enemy(Actor());

//.	Actor()->inventory().setSlotsBlocked			(true);
	Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);

	// hide crosshair
	m_crosshair_show			= !!psHUD_Flags.is(HUD_CROSSHAIR_RT);
	if (m_crosshair_show)		psHUD_Flags.set(HUD_CROSSHAIR_RT,FALSE);

	// Start effector
	m_effector_pp				= xr_new<CAlienEffectorPP>	(m_object->pp_vampire_effector, EFFECTOR_ID_GEN(EEffectorPPType));
	Actor()->Cameras().AddPPEffector	(m_effector_pp);
	
	m_effector					= xr_new<CAlienEffector>	(EFFECTOR_ID_GEN(ECamEffectorType),m_object);
	Actor()->Cameras().AddCamEffector	(m_effector);

	// make invisible
	m_object->state_invisible	= true;
	m_object->setVisible		(false);

	m_active					= true;
}

void CBloodsuckerAlien::deactivate()
{
	if (!m_active) return;

	m_object->CControlledActor::release			();

	Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, false);
	if (m_crosshair_show)							psHUD_Flags.set(HUD_CROSSHAIR_RT,TRUE);

	// Stop camera effector
	Actor()->Cameras().RemoveCamEffector	(EFFECTOR_ID_GEN(ECamEffectorType));
	m_effector						= 0;
	
	// Stop postprocess effector
	Actor()->Cameras().RemovePPEffector	(EFFECTOR_ID_GEN(EEffectorPPType));
	m_effector_pp->Destroy			();
	m_effector_pp					= 0;

	m_active						= false;

	// make visible
	m_object->state_invisible	= false;
	m_object->setVisible		(true);
}
