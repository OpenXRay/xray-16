#pragma once

#include "../xrphysics/icollisiondamagereceiver.h"

class CPhysicsShellHolder;
//struct dContact;
//struct SGameMtl;

class CPHCollisionDamageReceiver:
	public ICollisionDamageReceiver
{
typedef std::pair<u16,float> SControledBone;
DEFINE_VECTOR(SControledBone,DAMAGE_CONTROLED_BONES_V,DAMAGE_BONES_I);
struct SFind{u16 id;SFind(u16 _id){id=_id;};bool operator () (const SControledBone& cb){return cb.first==id;}};
DAMAGE_CONTROLED_BONES_V m_controled_bones;

protected:
	virtual CPhysicsShellHolder*		PPhysicsShellHolder			()																		=0;
			void						Init						()																		;

			void						Clear						()																		;
private:
			void						BoneInsert					(u16 id,float k)														;

	IC		DAMAGE_BONES_I				FindBone					(u16 id)
	{
		return std::find_if(m_controled_bones.begin(),m_controled_bones.end(),SFind(id));
	}
//	static	void 						CollisionCallback			(bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2)	;
			void						CollisionHit							(u16 source_id,u16 bone_id,float power,const Fvector &dir,Fvector &pos)	;
};
