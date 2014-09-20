#include "stdafx.h"
#include "ispatial.h"
#include "render.h"
#include "xr_object.h"
#include "PS_Instance.h"

ENGINE_API ISpatial_DB*		g_SpatialSpace			= NULL;
ENGINE_API ISpatial_DB*		g_SpatialSpacePhysic	= NULL;

Fvector	c_spatial_offset	[8]	= 
{
	{ -1, -1, -1	},
	{  1, -1, -1	},
	{ -1,  1, -1	},
	{  1,  1, -1	},
	{ -1, -1,  1	},
	{  1, -1,  1	},
	{ -1,  1,  1	},
	{  1,  1,  1	}
};

//////////////////////////////////////////////////////////////////////////
ISpatial::ISpatial			(ISpatial_DB* space)
{
	spatial.sphere.P.set	(0,0,0);
	spatial.sphere.R		= 0;
	spatial.node_center.set	(0,0,0);
	spatial.node_radius		= 0;
	spatial.node_ptr		= NULL;
	spatial.sector			= NULL;
	spatial.space			= space;
}
ISpatial::~ISpatial			(void)
{
	spatial_unregister		();
}
BOOL	ISpatial::spatial_inside()
{
	float	dr	= -(- spatial.node_radius + spatial.sphere.R);
	if (spatial.sphere.P.x < spatial.node_center.x - dr)	return FALSE;
	if (spatial.sphere.P.x > spatial.node_center.x + dr)	return FALSE;
	if (spatial.sphere.P.y < spatial.node_center.y - dr)	return FALSE;
	if (spatial.sphere.P.y > spatial.node_center.y + dr)	return FALSE;
	if (spatial.sphere.P.z < spatial.node_center.z - dr)	return FALSE;
	if (spatial.sphere.P.z > spatial.node_center.z + dr)	return FALSE;
	return TRUE;
}

BOOL	verify_sp	(ISpatial* sp, Fvector& node_center, float node_radius)
{
	float	dr	= -(- node_radius + sp->spatial.sphere.R);
	if (sp->spatial.sphere.P.x < node_center.x - dr)	return FALSE;
	if (sp->spatial.sphere.P.x > node_center.x + dr)	return FALSE;
	if (sp->spatial.sphere.P.y < node_center.y - dr)	return FALSE;
	if (sp->spatial.sphere.P.y > node_center.y + dr)	return FALSE;
	if (sp->spatial.sphere.P.z < node_center.z - dr)	return FALSE;
	if (sp->spatial.sphere.P.z > node_center.z + dr)	return FALSE;
	return TRUE;
}

void	ISpatial::spatial_register	()
{
	spatial.type			|=	STYPEFLAG_INVALIDSECTOR;
	if (spatial.node_ptr)
	{
		// already registered - nothing to do
	} else {
		// register
		R_ASSERT				(spatial.space);
		spatial.space->insert	(this);
		spatial.sector			=	0;
	}
}

void	ISpatial::spatial_unregister()
{
	if (spatial.node_ptr)
	{
		// remove
		spatial.space->remove	(this);
		spatial.node_ptr		= NULL;
		spatial.sector			= NULL;
	} else {
		// already unregistered
	}
}

void	ISpatial::spatial_move	()
{
	if (spatial.node_ptr)
	{
		//*** somehow it was determined that object has been moved
		spatial.type		|=				STYPEFLAG_INVALIDSECTOR;

		//*** check if we are supposed to correct it's spatial location
		if						(spatial_inside())	return;		// ???
		spatial.space->remove	(this);
		spatial.space->insert	(this);
	} else {
		//*** we are not registered yet, or already unregistered
		//*** ignore request
	}
}

void	ISpatial::spatial_updatesector_internal()
{
	IRender_Sector*		S				=	::Render->detectSector(spatial_sector_point());
	spatial.type						&=	~STYPEFLAG_INVALIDSECTOR;
	if (S)				spatial.sector	=	S;
}

//////////////////////////////////////////////////////////////////////////
void			ISpatial_NODE::_init			(ISpatial_NODE* _parent)
{
	parent		=	_parent;
	children[0]	=	children[1]	=	children[2]	=	children[3]	=
	children[4]	=	children[5]	=	children[6]	=	children[7]	=	NULL;
	items.clear();
}

void			ISpatial_NODE::_insert			(ISpatial* S)			
{	
	S->spatial.node_ptr			=	this;
	items.push_back					(S);
	S->spatial.space->stat_objects	++;
}

void			ISpatial_NODE::_remove			(ISpatial* S)			
{	
	S->spatial.node_ptr			=	NULL;
	xr_vector<ISpatial*>::iterator	it = std::find(items.begin(),items.end(),S);
	VERIFY				(it!=items.end());
	items.erase			(it);
	S->spatial.space->stat_objects	--;
}

//////////////////////////////////////////////////////////////////////////

ISpatial_DB::ISpatial_DB()
#ifdef PROFILE_CRITICAL_SECTIONS
	:cs(MUTEX_PROFILE_ID(ISpatial_DB))
#endif // PROFILE_CRITICAL_SECTIONS
{
	m_root					= NULL;
	stat_nodes				= 0;
	stat_objects			= 0;
}

ISpatial_DB::~ISpatial_DB()
{
	if ( m_root )
	{
		_node_destroy(m_root);
	}

	while (!allocator_pool.empty()){
		allocator.destroy		(allocator_pool.back());
		allocator_pool.pop_back	();
	}
}

void			ISpatial_DB::initialize(Fbox& BB)
{
	if (0==m_root)			
	{
		// initialize
		Fvector bbc,bbd;
		BB.get_CD				(bbc,bbd);

		bbc.set					(0,0,0);			// generic
		bbd.set					(1024,1024,1024);	// generic

		allocator_pool.reserve	(128);
		m_center.set			(bbc);
		m_bounds				= _max(_max(bbd.x,bbd.y),bbd.z);
		rt_insert_object		= NULL;
		if (0==m_root)	m_root	= _node_create();
		m_root->_init			(NULL);
	}
}
ISpatial_NODE*	ISpatial_DB::_node_create		()
{
	stat_nodes	++;
	if (allocator_pool.empty())			return allocator.create();
	else								
	{
		ISpatial_NODE*	N		= allocator_pool.back();
		allocator_pool.pop_back	();
		return			N;
	}
}
void			ISpatial_DB::_node_destroy(ISpatial_NODE* &P)
{
	VERIFY						(P->_empty());
	stat_nodes					--;
	allocator_pool.push_back	(P);
	P							= NULL;
}

void			ISpatial_DB::_insert	(ISpatial_NODE* N, Fvector& n_C, float n_R)
{
	//*** we are assured that object lives inside our node
	float	n_vR	= 2*n_R;
	VERIFY	(N);
	VERIFY	(verify_sp(rt_insert_object,n_C,n_vR));

	// we have to make sure we aren't the leaf node
	if (n_R<=c_spatial_min)
	{
		// this is leaf node
		N->_insert									(rt_insert_object);
		rt_insert_object->spatial.node_center.set	(n_C);
		rt_insert_object->spatial.node_radius		= n_vR;		// vR
		return;
	}

	// we have to check if it can be putted further down
	float	s_R			= rt_insert_object->spatial.sphere.R;	// spatial bounds
	float	c_R			= n_R/2;								// children bounds
	if (s_R<c_R)
	{
		// object can be pushed further down - select "octant", calc node position
		Fvector&	s_C					=	rt_insert_object->spatial.sphere.P;
		u32			octant				=	_octant	(n_C,s_C);
		Fvector		c_C;				c_C.mad	(n_C,c_spatial_offset[octant],c_R);
		VERIFY			(octant == _octant(n_C,c_C));				// check table assosiations
		ISpatial_NODE*	&chield			= N->children[octant];

		if (0==chield)	{
			chield			=	_node_create();
			VERIFY			(chield);
			chield->_init	(N);
			VERIFY			(chield);
		}
		VERIFY			(chield);
		_insert			(chield, c_C, c_R);
		VERIFY			(chield);
	}
	else
	{
		// we have to "own" this object (potentially it can be putted down sometimes...)
		N->_insert									(rt_insert_object);
		rt_insert_object->spatial.node_center.set	(n_C);
		rt_insert_object->spatial.node_radius		= n_vR;
	}
}

void			ISpatial_DB::insert		(ISpatial* S)
{
	cs.Enter			();
#ifdef DEBUG
	stat_insert.Begin	();

	BOOL		bValid	= _valid(S->spatial.sphere.R) && _valid(S->spatial.sphere.P);
	if (!bValid)	
	{
		CObject*	O	= dynamic_cast<CObject*>(S);
		if	(O)			Debug.fatal(DEBUG_INFO,"Invalid OBJECT position or radius (%s)",O->cName());
		else			{
			CPS_Instance* P = dynamic_cast<CPS_Instance*>(S);
			if (P)		Debug.fatal(DEBUG_INFO,"Invalid PS spatial position{%3.2f,%3.2f,%3.2f} or radius{%3.2f}",VPUSH(S->spatial.sphere.P),S->spatial.sphere.R);
			else		Debug.fatal(DEBUG_INFO,"Invalid OTHER spatial position{%3.2f,%3.2f,%3.2f} or radius{%3.2f}",VPUSH(S->spatial.sphere.P),S->spatial.sphere.R);
		}
	}
#endif

	if (verify_sp(S,m_center,m_bounds))
	{
		// Object inside our DB
		rt_insert_object			= S;
		_insert						(m_root,m_center,m_bounds);
		VERIFY						(S->spatial_inside());
	} else {
		// Object outside our DB, put it into root node and hack bounds
		// Object will reinsert itself until fits into "real", "controlled" space
		m_root->_insert				(S);
		S->spatial.node_center.set	(m_center);
		S->spatial.node_radius		=	m_bounds;
	}
#ifdef DEBUG
	stat_insert.End		();
#endif
	cs.Leave			();
}

void			ISpatial_DB::_remove	(ISpatial_NODE* N, ISpatial_NODE* N_sub)
{
	if (0==N)							return;

	//*** we are assured that node contains N_sub and this subnode is empty
	u32 octant	= u32(-1);
	if (N_sub==N->children[0])			octant = 0;
	else if (N_sub==N->children[1])		octant = 1;
	else if (N_sub==N->children[2])		octant = 2;
	else if (N_sub==N->children[3])		octant = 3;
	else if (N_sub==N->children[4])		octant = 4;
	else if (N_sub==N->children[5])		octant = 5;
	else if (N_sub==N->children[6])		octant = 6;
	else if (N_sub==N->children[7])		octant = 7;
	VERIFY		(octant<8);
	VERIFY		(N_sub->_empty());
	_node_destroy						(N->children[octant]);

	// Recurse
	if (N->_empty())					_remove(N->parent,N);
}

void			ISpatial_DB::remove		(ISpatial* S)
{
	cs.Enter			();
#ifdef DEBUG
	stat_remove.Begin	();
#endif
	ISpatial_NODE* N	= S->spatial.node_ptr;
	N->_remove			(S);

	// Recurse
	if (N->_empty())	_remove(N->parent,N);

#ifdef DEBUG
	stat_remove.End		();
#endif
	cs.Leave			();
}

void			ISpatial_DB::update		(u32 nodes/* =8 */)
{
#ifdef DEBUG
	if (0==m_root)	return;
	cs.Enter		();
	VERIFY			(verify());
	cs.Leave		();
#endif
}
