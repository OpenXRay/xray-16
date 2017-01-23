#ifndef EXTENDED_GEOM
#define EXTENDED_GEOM
#include "PHObject.h"
#include "ode_include.h"
#include "physicscommon.h"
#include "MathUtils.h"



#ifdef DEBUG
extern	u32				dbg_total_saved_tries					;
#endif
class CPhysicsShellHolder;





class CObjectContactCallback
{
	
			CObjectContactCallback				*next			;
			ObjectContactCallbackFun			*callback		;
	public:
		CObjectContactCallback		(ObjectContactCallbackFun			*c)
			:callback(c)
		{
			next=NULL; VERIFY(c);
		}
		~CObjectContactCallback()
		{
			xr_delete(next);
		}
	void Add(ObjectContactCallbackFun		*c)
	{
		VERIFY(c);
		VERIFY(callback!=c);
	
		if(next)
		{
			next->Add(c);
		}
		else
		{
			next=xr_new<CObjectContactCallback>(c);
		}
	}
	bool	HasCallback(ObjectContactCallbackFun		*c)
	{
		for(CObjectContactCallback*i=this;i;i=i->next)
		{
			VERIFY(i->callback);
			if(c==i->callback) return true;
		}
		return false;
	}

static	void RemoveCallback(CObjectContactCallback*	&callbacks,ObjectContactCallbackFun	*c)
	{
		if(!callbacks) return;
		VERIFY(c);
		VERIFY(callbacks->callback);

			if(c==callbacks->callback)
			{
				CObjectContactCallback	*del=callbacks;
				callbacks=callbacks->next;
				del->next=NULL;
				xr_delete(del);
				VERIFY(!callbacks||!callbacks->HasCallback(c));
			} else{
				for(CObjectContactCallback	*i=callbacks->next,*p=callbacks;i;)
				{
					
					VERIFY(p->callback);
					VERIFY(i->callback);
					VERIFY(i);
					VERIFY(p);
					if(c==i->callback)
					{
						CObjectContactCallback	*del=i;
						p->next=i->next;del->next=NULL;xr_delete(del);
						VERIFY(!callbacks->HasCallback(c));
						break;
					}
					i=i->next;
					p=p->next;
					VERIFY(p->next==i);
				}
			}
	}

	void	Call(bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2)
	{
		for(CObjectContactCallback*i=this;i;i=i->next)
		{
			VERIFY(i->callback);
			i->callback(do_colide,bo1,c,material_1,material_2);
		}
	}
};

class CGameObject;
struct dxGeomUserData
{
	dVector3					last_pos										;
	bool						pushing_neg,pushing_b_neg,b_static_colide		;
	CDB::TRI					*neg_tri,*b_neg_tri								;
	CPHObject					*ph_object										;
	CPhysicsShellHolder			*ph_ref_object									;
	u16							material										;
	u16							tri_material									;
	ContactCallbackFun			*callback										;
	void						*callback_data									;
//	ObjectContactCallbackFun	*object_callback								;
	CObjectContactCallback		*object_callbacks								;
	u16							element_position								;
	u16							bone_id											;
	xr_vector<int>				cashed_tries									;
	Fvector						last_aabb_size									;
	Fvector						last_aabb_pos									;

//	struct ContactsParameters
//	{
//	dReal damping;
//	dReal spring;
//	dReal bonce;
//	dReal bonce_vel;
//	dReal mu;
//	unsigned int maxc;
//	};
};

IC dxGeomUserData* dGeomGetUserData(dxGeom* geom)
{
	return (dxGeomUserData*) dGeomGetData(geom);
}

IC dGeomID retrieveGeom(dGeomID geom)
{
	if(dGeomGetClass(geom)==dGeomTransformClass)
		return dGeomTransformGetGeom(geom);
	else
		return geom;
}

IC dxGeomUserData* retrieveGeomUserData(dGeomID geom)
{
			return dGeomGetUserData(retrieveGeom(geom));
			//if(dGeomGetClass(geom)==dGeomTransformClass)
			//	return dGeomGetUserData(dGeomTransformGetGeom(geom));
			//else
			//	return dGeomGetUserData(geom);
}

IC void	get_user_data( dxGeomUserData* &gd1, dxGeomUserData* &gd2, bool bo1, const dContactGeom &geom )
{

	if( bo1 )
	{
		gd1 =retrieveGeomUserData( geom.g1 );
		gd2 =retrieveGeomUserData( geom.g2 );
	}
	else
	{
		gd2 =retrieveGeomUserData( geom.g1 );
		gd1 =retrieveGeomUserData( geom.g2 );
	}
}

IC CPhysicsShellHolder* retrieveRefObject(dGeomID geom)
{
	dxGeomUserData* ud=dGeomGetUserData(retrieveGeom(geom));
	if(ud)return ud->ph_ref_object;
	else return NULL;
}
IC void dGeomCreateUserData(dxGeom* geom)
{
	if(!geom) return;
	dGeomSetData(geom,xr_new<dxGeomUserData>());
	(dGeomGetUserData(geom))->pushing_neg=false;
	(dGeomGetUserData(geom))->pushing_b_neg=false;
	(dGeomGetUserData(geom))->b_static_colide=true;
	(dGeomGetUserData(geom))->last_pos[0]=-dInfinity;
	(dGeomGetUserData(geom))->last_pos[1]=-dInfinity;
	(dGeomGetUserData(geom))->last_pos[2]=-dInfinity;
	(dGeomGetUserData(geom))->ph_object=NULL;
	(dGeomGetUserData(geom))->material=0;
	(dGeomGetUserData(geom))->tri_material=0;
	(dGeomGetUserData(geom))->callback=NULL;
	(dGeomGetUserData(geom))->object_callbacks=NULL;
	(dGeomGetUserData(geom))->ph_ref_object=NULL;
	(dGeomGetUserData(geom))->element_position=u16(-1);
	(dGeomGetUserData(geom))->bone_id=u16(-1);
	(dGeomGetUserData(geom))->callback_data=NULL;
	//((dxGeomUserData*)dGeomGetData(geom))->ContactsParameters::mu=1.f;
	//((dxGeomUserData*)dGeomGetData(geom))->ContactsParameters::damping=1.f;
	//((dxGeomUserData*)dGeomGetData(geom))->ContactsParameters::spring=1.f;
	//((dxGeomUserData*)dGeomGetData(geom))->ContactsParameters::bonce=0.f;
	//((dxGeomUserData*)dGeomGetData(geom))->ContactsParameters::bonce_vel=0.f;
}



IC void dGeomDestroyUserData(dxGeom* geom)
{
	if(!geom)			return							;
	dxGeomUserData*	P	= dGeomGetUserData(geom)		;
	if(P)
	{
#ifdef DEBUG
		dbg_total_saved_tries-=P->cashed_tries.size()		;
#endif
		P->cashed_tries		.clear()						;
		xr_delete			(P->object_callbacks)			;
	}
	xr_delete			(P)								;
	dGeomSetData		(geom,0)						;
}

IC void dGeomUserDataSetCallbackData(dxGeom* geom,void *cd)
{
	(dGeomGetUserData(geom))->callback_data=cd;
}
IC void dGeomUserDataSetPhObject(dxGeom* geom,CPHObject* phObject)
{
	(dGeomGetUserData(geom))->ph_object=phObject;
}

IC void dGeomUserDataSetPhysicsRefObject(dxGeom* geom,CPhysicsShellHolder* phRefObject)
{
	(dGeomGetUserData(geom))->ph_ref_object=phRefObject;
}

IC void dGeomUserDataSetContactCallback(dxGeom* geom,ContactCallbackFun* callback)
{
	(dGeomGetUserData(geom))->callback=callback;
}

IC void dGeomUserDataSetObjectContactCallback(dxGeom* geom,ObjectContactCallbackFun	*obj_callback)
{
	xr_delete((dGeomGetUserData(geom))->object_callbacks);
	if(obj_callback)(dGeomGetUserData(geom))->object_callbacks=xr_new<CObjectContactCallback>(obj_callback);
}

IC void dGeomUserDataAddObjectContactCallback(dxGeom* geom,ObjectContactCallbackFun	*obj_callback)
{
	if((dGeomGetUserData(geom))->object_callbacks)
	{
		(dGeomGetUserData(geom))->object_callbacks->Add(obj_callback);
	}
	else dGeomUserDataSetObjectContactCallback(geom,obj_callback);
}

IC void dGeomUserDataRemoveObjectContactCallback(dxGeom* geom,ObjectContactCallbackFun	*obj_callback)
{
	CObjectContactCallback::RemoveCallback((dGeomGetUserData(geom))->object_callbacks,(obj_callback));
}

IC bool dGeomUserDataHasCallback(dxGeom* geom,ObjectContactCallbackFun	*obj_callback)
{
	geom=retrieveGeom(geom);
	if(geom&&dGeomGetUserData(geom)&&(dGeomGetUserData(geom))->object_callbacks)
				return (dGeomGetUserData(geom))->object_callbacks->HasCallback(obj_callback);
	else return false;
}
IC void dGeomUserDataSetElementPosition(dxGeom* geom,u16 e_pos)
{
	(dGeomGetUserData(geom))->element_position=e_pos;
}
IC void dGeomUserDataSetBoneId(dxGeom* geom,u16 bone_id)
{
	(dGeomGetUserData(geom))->bone_id=bone_id;
}
IC void dGeomUserDataResetLastPos(dxGeom* geom)
{
	(dGeomGetUserData(geom))->last_pos[0]=-dInfinity;
	(dGeomGetUserData(geom))->last_pos[1]=-dInfinity;
	(dGeomGetUserData(geom))->last_pos[2]=-dInfinity;
	(dGeomGetUserData(geom))->pushing_neg=false;
	(dGeomGetUserData(geom))->pushing_b_neg=false;
	(dGeomGetUserData(geom))->b_static_colide=true;
}
IC void dGeomUserDataClearCashedTries(dxGeom* geom)
{
	dxGeomUserData*	P	= dGeomGetUserData(geom);

#ifdef DEBUG
	dbg_total_saved_tries-=P->cashed_tries.size();
#endif
	P->cashed_tries.clear();
	P->last_aabb_size.set(0.f,0.f,0.f);
}
#endif