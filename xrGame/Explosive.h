// Explosive.h: интерфейс для взврывающихся объектов
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "../xrEngine/Render.h"
#include "../xrEngine/feel_touch.h"
#include "inventory_item.h"
#include "ai_sounds.h"
#include "script_export_space.h"
#include "../xrphysics/DamageSource.h"
#include "wallmark_manager.h"
#include "ParticlesObject.h"
class IRender_Light;
DEFINE_VECTOR(CPhysicsShellHolder*,BLASTED_OBJECTS_V,BLASTED_OBJECTS_I);
class CExplosive : 
	public IDamageSource
{
private:
	collide::rq_results			rq_storage;

public:
								CExplosive(void);
	virtual						~CExplosive(void);

	virtual void 				Load(LPCSTR section);
	virtual void				Load(CInifile const * ini,LPCSTR section);

	virtual void 				net_Destroy		();
	virtual void				net_Relcase		(CObject* O);
	virtual void 				UpdateCL();

private:
	virtual void 				Explode();
public:
	virtual void 				ExplodeParams	(const Fvector& pos, const Fvector& dir);

	static float 				ExplosionEffect	(collide::rq_results& storage,CExplosive*exp_obj,CPhysicsShellHolder*blasted_obj,  const Fvector &expl_centre, const float expl_radius);


	virtual void 				OnEvent (NET_Packet& P, u16 type) ;//{inherited::OnEvent( P, type);}
	virtual void				OnAfterExplosion();
	virtual void				OnBeforeExplosion();
	virtual void 				SetCurrentParentID	(u16 parent_id) {m_iCurrentParentID = parent_id; }
	IC		u16 				CurrentParentID		() const {return m_iCurrentParentID;}

	virtual	void				SetInitiator(u16 id){SetCurrentParentID(id);}
	virtual	u16					Initiator();

	virtual void				UpdateExplosionPos(){}
	virtual void				GetExplVelocity(Fvector	&v);
	virtual void				GetExplPosition(Fvector &p) ;
	virtual void				GetExplDirection(Fvector &d);
	virtual void 				GenExplodeEvent (const Fvector& pos, const Fvector& normal);
	virtual void 				FindNormal(Fvector& normal);
	virtual CGameObject			*cast_game_object()=0;
	virtual CExplosive*			cast_explosive(){return this;}
	virtual IDamageSource*		cast_IDamageSource()	{return this;}
	virtual void				GetRayExplosionSourcePos(Fvector &pos);
	virtual	void				GetExplosionBox			(Fvector &size);
	virtual void				ActivateExplosionBox	(const Fvector &size,Fvector &in_out_pos);
			void				SetExplosionSize		(const Fvector &new_size);
	virtual bool				Useful					() const;
protected:
			bool				IsSoundPlaying			(){return !!sndExplode._feedback();}
			bool				IsExploded				(){return !!m_explosion_flags.test(flExploded);}
public:
			bool				IsExploding				(){return !!m_explosion_flags.test(flExploding);}
private:
			void				PositionUpdate			();
static		void				GetRaySourcePos			(CExplosive	*exp_obj,const Fvector &expl_centre,Fvector	&p);

			void				ExplodeWaveProcessObject(collide::rq_results& storage,CPhysicsShellHolder*sh);
			void				ExplodeWaveProcess		();
static		float				TestPassEffect			(const	Fvector	&source_p,	const	Fvector	&dir,float range,float ef_radius,collide::rq_results& storage, CObject* blasted_obj);
			void				LightCreate				();
			void				LightDestroy			();
protected:

	CWalmarkManager				m_wallmark_manager;
	//ID персонажа который иницировал действие
	u16							m_iCurrentParentID;
	
	//bool						m_bReadyToExplode;
	Fvector						m_vExplodePos;
	Fvector 					m_vExplodeSize;
	Fvector 					m_vExplodeDir;

	//параметры взрыва
	float 						m_fBlastHit;
	float 						m_fBlastHitImpulse;
	float 						m_fBlastRadius;
	
	//параметры и количество осколков
	float 						m_fFragsRadius; 
	float 						m_fFragHit;
	float 						m_fFragHitImpulse;
	int	  						m_iFragsNum;

	//типы наносимых хитов
	ALife::EHitType 			m_eHitTypeBlast;
	ALife::EHitType 			m_eHitTypeFrag;

	//фактор подпроса предмета вверх взрывной волной 
	float						m_fUpThrowFactor;

	//список пораженных объектов
	BLASTED_OBJECTS_V			m_blasted_objects;

	//текущая продолжительность взрыва
	float						m_fExplodeDuration;
	//общее время взрыва
	float						m_fExplodeDurationMax;
	//Время, через которое надо сделать взрывчатку невиимой, если она не становится невидимой во время взрыва
	float						m_fExplodeHideDurationMax;
	//флаг состояния взрыва
	enum{
		flExploding				=1<<0	,
		flExplodEventSent		=1<<1	,
		flReadyToExplode		=1<<2	,
		flExploded				=1<<3	
	};
	Flags8						m_explosion_flags;
	///////////////////////////////////////////////
	//Должен ли объект быть скрыт после взрыва: true - для всех кроме дымовой гранаты
	BOOL						m_bHideInExplosion;
	bool						m_bAlreadyHidden;
	virtual void				HideExplosive	();
	//bool						m_bExploding;
	//bool						m_bExplodeEventSent;

	//////////////////////////////////////////////
	//для разлета осколков
	float						m_fFragmentSpeed;
	
	//звуки
	ref_sound					sndExplode;
	ESoundTypes					m_eSoundExplode;

	//размер отметки на стенах
	float						fWallmarkSize;
	
	//эффекты и подсветка
	shared_str					m_sExplodeParticles;
	
	//подсветка взрыва
	ref_light					m_pLight;
	Fcolor						m_LightColor;
	float						m_fLightRange;
	float						m_fLightTime;
	
	virtual	void				StartLight	();
	virtual	void				StopLight	();

	BOOL						m_bDynamicParticles;
	CParticlesObject*			m_pExpParticle;
	virtual void				UpdateExplosionParticles ();	

	// эффектор
	struct {
		shared_str				effect_sect_name;
	} effector;
	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CExplosive)
#undef script_type_list
#define script_type_list save_type_list(CExplosive)

IC void random_point_in_object_box(Fvector &out_pos,CObject* obj)
{
	const Fbox &l_b1 = obj->BoundingBox();
	Fvector l_c, l_d;l_b1.get_CD(l_c,l_d);
	out_pos.random_point(l_d);
	obj->XFORM().transform_tiny(out_pos);
	out_pos.add(l_c);
}