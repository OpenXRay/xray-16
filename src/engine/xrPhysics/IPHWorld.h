#pragma once
#include "physicsexternalcommon.h"
#include "iphysics_scripted.h"
class CPhysicsShell;
class IPHWorldUpdateCallbck
{
public:
	virtual	void		update_step			()					= 0;
	virtual	void		phys_shell_relcase	(CPhysicsShell* sh)	= 0;
protected:
	virtual ~IPHWorldUpdateCallbck() {}
};

class IPHWorld:
	public iphysics_scripted_class
{
	public:
	virtual								~IPHWorld						()									{}
	virtual  float						Gravity							()									= 0;
	virtual  void						SetGravity						( float	g )							= 0;
	virtual  bool						Processing						()									= 0;
	virtual  u32						CalcNumSteps					(u32 dTime)							= 0;
	virtual  u64						&StepsNum						()									= 0;

	virtual  float						FrameTime						()									= 0;
	virtual  void 						Freeze							()									= 0;
	virtual  void 						UnFreeze						()									= 0;
	virtual  void						Step							()									= 0;
	virtual  void						SetStep							( float s )							= 0;
	virtual  void						StepNumIterations				( int num_it )						= 0;
	virtual  void						set_default_contact_shotmark	(ContactCallbackFun	*f)				= 0;
	virtual  void						set_default_character_contact_shotmark(ContactCallbackFun	*f)		= 0;
	virtual  void						set_step_time_callback			(PhysicsStepTimeCallback* cb)		= 0;
	virtual  void						set_update_callback				( IPHWorldUpdateCallbck* cb)		= 0;
#ifdef DEBUG
	virtual  u16						ObjectsNumber					()									= 0;
	virtual  u16						UpdateObjectsNumber				()									= 0;
	virtual  void 						OnRender						()									= 0;
#endif

};

extern "C" XRPHYSICS_API	IPHWorld *		__stdcall	physics_world();
class CObjectSpace;
class CObjectList;
class CRenderDeviceBase;
extern "C" XRPHYSICS_API	void			__stdcall	create_physics_world( bool mt, CObjectSpace* os, CObjectList *lo, CRenderDeviceBase *dv  );
extern "C" XRPHYSICS_API	void			__stdcall	destroy_physics_world();
class CGameMtlLibrary;
extern "C" XRPHYSICS_API	void			__stdcall	set_mtl_lib(CGameMtlLibrary * l);
extern "C" XRPHYSICS_API	CObjectSpace*	__stdcall	create_object_space();
struct hdrCFORM;
extern "C" XRPHYSICS_API	CObjectSpace*	__stdcall	mesh_create_object_space(Fvector* verts, CDB::TRI* tris, const hdrCFORM &H, CDB::build_callback build_callback);
extern "C" XRPHYSICS_API	void			__stdcall	destroy_object_space(CObjectSpace* &os);