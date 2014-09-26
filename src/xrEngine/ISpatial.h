#ifndef XRENGINE_ISPATIAL_H_INCLUDED
#define XRENGINE_ISPATIAL_H_INCLUDED

#pragma once

#include "../xrcore/xrPool.h"
#include "xr_collide_defs.h"

#pragma pack(push,4)

/*
Requirements:
0. Generic
	* O(1) insertion
		- radius completely determines	"level"
		- position completely detemines "node"
	* O(1) removal
	* 
1. Rendering
	* Should live inside spatial DB
	* Should have at least "bounding-sphere" or "bounding-box"
	* Should have pointer to "sector" it lives in
	* Approximate traversal order relative to point ("camera")
2. Spatial queries
	* Should live inside spatial DB
	* Should have at least "bounding-sphere" or "bounding-box"
*/

const float						c_spatial_min	= 8.f;
//////////////////////////////////////////////////////////////////////////
enum
{
	STYPE_RENDERABLE			= (1<<0),
	STYPE_LIGHTSOURCE			= (1<<1),
	STYPE_COLLIDEABLE			= (1<<2),
	STYPE_VISIBLEFORAI			= (1<<3),
	STYPE_REACTTOSOUND			= (1<<4),
	STYPE_PHYSIC				= (1<<5),
	STYPE_OBSTACLE				= (1<<6),
	STYPE_SHAPE					= (1<<7),
	STYPE_LIGHTSOURCEHEMI		= (1<<8),

	STYPEFLAG_INVALIDSECTOR		= (1<<16)
};
//////////////////////////////////////////////////////////////////////////
// Comment: 
//		ordinal objects			- renderable?, collideable?, visibleforAI?
//		physical-decorations	- renderable, collideable
//		lights					- lightsource
//		particles(temp-objects)	- renderable
//		glow					- renderable
//		sound					- ???
//////////////////////////////////////////////////////////////////////////
class ENGINE_API				IRender_Sector;
class ENGINE_API				ISpatial;
class ENGINE_API				ISpatial_NODE;
class ENGINE_API				ISpatial_DB;

//////////////////////////////////////////////////////////////////////////
// Fast type conversion
class ENGINE_API				CObject;
class ENGINE_API				IRenderable;
class ENGINE_API				IRender_Light;
namespace Feel { class ENGINE_API Sound; }

//////////////////////////////////////////////////////////////////////////
class ENGINE_API				ISpatial
{
public:
	struct	_spatial
	{
		u32						type;
		Fsphere					sphere;
		Fvector					node_center;	// Cached node center for TBV optimization
		float					node_radius;	// Cached node bounds for TBV optimization
		ISpatial_NODE*			node_ptr;		// Cached parent node for "empty-members" optimization
		IRender_Sector*			sector;
		ISpatial_DB*			space;			// allow different spaces

		_spatial() : type(0)	{}				// safe way to enhure type is zero before any contstructors takes place
	}							spatial;
public:
	BOOL						spatial_inside		()			;
				void			spatial_updatesector_internal()	;
public:
	virtual		void			spatial_register	()	;
	virtual		void			spatial_unregister	()	;
	virtual		void			spatial_move		()	;
	virtual		Fvector			spatial_sector_point()	{ return spatial.sphere.P; }
	ICF			void			spatial_updatesector()	{
		if (0== (spatial.type&STYPEFLAG_INVALIDSECTOR))	return;
		spatial_updatesector_internal				()	;
	};

	virtual		CObject*		dcast_CObject		()	{ return 0;	}
	virtual		Feel::Sound*	dcast_FeelSound		()	{ return 0;	}
	virtual		IRenderable*	dcast_Renderable	()	{ return 0;	}
	virtual		IRender_Light*	dcast_Light			()	{ return 0; }

				ISpatial		(ISpatial_DB* space	);
	virtual		~ISpatial		();
};

//////////////////////////////////////////////////////////////////////////
class ENGINE_API				ISpatial_NODE
{
public:
	typedef	_W64 unsigned		ptrt;
public:
	ISpatial_NODE*				parent;					// parent node for "empty-members" optimization
	ISpatial_NODE*				children		[8];	// children nodes
	xr_vector<ISpatial*>		items;					// own items
public:
	void						_init			(ISpatial_NODE* _parent);
	void						_remove			(ISpatial*		_S);
	void						_insert			(ISpatial*		_S);
	BOOL						_empty			()						
	{
		return items.empty() && (
			0==(
				ptrt(children[0])|ptrt(children[1])|
				ptrt(children[2])|ptrt(children[3])|
				ptrt(children[4])|ptrt(children[5])|
				ptrt(children[6])|ptrt(children[7])
				)
			);	
	}
};

//////////////////////////////////////////////////////////////////////////
class ENGINE_API					ISpatial_DB
{
private:
	xrCriticalSection				cs;
	poolSS<ISpatial_NODE,128>		allocator;
	xr_vector<ISpatial_NODE*>		allocator_pool;
	ISpatial*						rt_insert_object;
public:
	ISpatial_NODE*					m_root;
	Fvector							m_center;
	float							m_bounds;
	xr_vector<ISpatial*>*			q_result;
	u32								stat_nodes;
	u32								stat_objects;
	CStatTimer						stat_insert;
	CStatTimer						stat_remove;
private:
	IC u32							_octant			(u32 x, u32 y, u32 z)			{	return z*4 + y*2 + x;	}
	IC u32							_octant			(Fvector& base, Fvector& rel)
	{
		u32 o	= 0;
		if (rel.x > base.x) o+=1;
		if (rel.y > base.y) o+=2;
		if (rel.z > base.z) o+=4;
		return	o;
	}

	ISpatial_NODE*					_node_create	();
	void 							_node_destroy	(ISpatial_NODE* &P);

	void							_insert			(ISpatial_NODE* N, Fvector& n_center, float n_radius);
	void							_remove			(ISpatial_NODE* N, ISpatial_NODE* N_sub);
public:
	ISpatial_DB();
	~ISpatial_DB();

	// managing
	void							initialize		(Fbox& BB);
	//void							destroy			();
	void							insert			(ISpatial* S);
	void							remove			(ISpatial* S);
	void							update			(u32 nodes=8);
	BOOL							verify			();

public:
	enum
	{
		O_ONLYFIRST		= (1<<0),
		O_ONLYNEAREST	= (1<<1),
		O_ORDERED		= (1<<2),
		O_force_u32		= u32(-1)
	};

	// query
	void							q_ray			(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_and, const Fvector&		_start,  const Fvector&	_dir, float _range);
	void							q_box			(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or,  const Fvector&		_center, const Fvector& _size);
	void							q_sphere		(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or,  const Fvector&		_center, const float _radius);
	void							q_frustum		(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or,  const CFrustum&	_frustum);
};

ENGINE_API extern ISpatial_DB*		g_SpatialSpace			;
ENGINE_API extern ISpatial_DB*		g_SpatialSpacePhysic	;

#pragma pack(pop)

#endif // #ifndef XRENGINE_ISPATIAL_H_INCLUDED