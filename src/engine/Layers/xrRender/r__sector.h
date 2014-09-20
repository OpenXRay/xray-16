// Portal.h: interface for the CPortal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_PORTAL_H_)
#define _PORTAL_H_
#pragma once

class	CPortal;
class	CSector;

struct	_scissor					: public Fbox2
{
	float	depth;
};

// Connector
class	CPortal						: public IRender_Portal
#ifdef DEBUG
	, public pureRender
#endif
{
private:
	svector<Fvector,8>				poly;
	CSector							*pFace,*pBack;
public:
	Fplane							P;
	Fsphere							S;
	u32								marker;
	BOOL							bDualRender;

	void							Setup								(Fvector* V, int vcnt, CSector* face, CSector* back);

	svector<Fvector,8>&				getPoly()							{ return poly;		}
	CSector*						Back()								{ return pBack;		}
	CSector*						Front()								{ return pFace;		}
	CSector*						getSector		(CSector* pFrom)	{ return pFrom==pFace?pBack:pFace; }
	CSector*						getSectorFacing	(const Fvector& V)	{ if (P.classify(V)>0) return pFace; else return pBack; }
	CSector*						getSectorBack	(const Fvector& V)	{ if (P.classify(V)>0) return pBack; else return pFace;	}
	float							distance		(const Fvector &V)	{ return _abs(P.classify(V)); }

									CPortal			();
	virtual							~CPortal		();

#ifdef DEBUG
	virtual void					OnRender		();
#endif
};

class dxRender_Visual;

// Main 'Sector' class
class	 CSector					: public IRender_Sector
{
protected:
	dxRender_Visual*					m_root;			// whole geometry of that sector
	xr_vector<CPortal*>				m_portals;
public:
	xr_vector<CFrustum>				r_frustums;
	xr_vector<_scissor>				r_scissors;
	_scissor						r_scissor_merged;
	u32								r_marker;
public:
	// Main interface
	dxRender_Visual*					root			()				{ return m_root; }
	void							traverse		(CFrustum& F,	_scissor& R);
	void							load			(IReader& fs);

	CSector							()				{ m_root = NULL;	}
	virtual							~CSector		( );
};

class	CPortalTraverser
{
public:
	enum
	{
		VQ_HOM		= (1<<0),
		VQ_SSA		= (1<<1),
		VQ_SCISSOR	= (1<<2),
		VQ_FADE		= (1<<3),				// requires SSA to work
	};
public:
	u32										i_marker;		// input
	u32										i_options;		// input:	culling options
	Fvector									i_vBase;		// input:	"view" point
	Fmatrix									i_mXFORM;		// input:	4x4 xform
	Fmatrix									i_mXFORM_01;	// 
	CSector*								i_start;		// input:	starting point
	xr_vector<IRender_Sector*>				r_sectors;		// result
	xr_vector<std::pair<CPortal*, float> >	f_portals;		// 
	ref_shader								f_shader;
	ref_geom								f_geom;
public:
									CPortalTraverser	();
	void							initialize			();
	void							destroy				();
	void							traverse			(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options);
	void							fade_portal			(CPortal* _p, float ssa);
	void							fade_render			();
#ifdef DEBUG
	void							dbg_draw		();
#endif
};

extern	CPortalTraverser			PortalTraverser	;

#endif // !defined(AFX_PORTAL_H__1FC2D371_4A19_49EA_BD1E_2D0F8DEBBF15__INCLUDED_)
