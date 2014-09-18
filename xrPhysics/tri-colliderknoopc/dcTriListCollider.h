//#include "stdafx.h"
#ifndef __DCTRILISTCOLLIDER_INCLUDED__
#define __DCTRILISTCOLLIDER_INCLUDED__
#include "dcTriangle.h"
struct	dxGeom;
struct	dxTriList;
enum{
	fl_engaged_v0 =1<<0,
	fl_engaged_v1 =1<<1,
	fl_engaged_v2 =1<<2,
	fl_engaged_s0 =1<<3,
	fl_engaged_s1 =1<<4,
	fl_engaged_s2 =1<<5,
};
class	dcTriListCollider;
class dcTriListCollider
{
	dxGeom* Geometry;	// The geom object
	dxTriList* GeomData;	// Buffered pointer to classdata

	xr_vector<Triangle> 		pos_tries			;
	xr_vector<Triangle> 		neg_tries			;
	xr_vector< flags8 >			gl_cl_tries_state	;
	xr_vector<int>::iterator	I,E,B				;
public:
	dcTriListCollider(dxGeom* Geometry);
	~dcTriListCollider();
	int CollideBox(dxGeom* Box, int Flags, dContactGeom* Contact, int Stride);
	int CollideSphere(dxGeom* Sphere, int Flags, dContactGeom* Contact, int Stride);
	int CollideCylinder(dxGeom* Cylinder, int Flags, dContactGeom* Contacts, int Stride);
private:
	IC	void VxToGlClTriState(u32 v,CDB::TRI*       T_array);
	IC	void SideToGlClTriState(u32 v0,u32 v1,CDB::TRI*       T_array);
	template<class T>
IC	int dSortTriPrimitiveCollide (T primitive,	
				dxGeom		*o1,		dxGeom			*o2,
				int			flags,		dContactGeom	*contact,	int skip,
				const Fvector&	AABB
				);
	IC dReal PointSphereTest(const dReal* center, const dReal radius,
		const dReal* pt,dReal* norm);
	IC dReal FragmentonSphereTest(const dReal* center, const dReal radius,
		const dReal* pt1, const dReal* pt2,dReal* norm);
	IC bool FragmentonSphereTest(const dReal* center, const dReal radius,
		const dReal* pt1, const dReal* pt2,dReal* norm,dReal& depth);
	IC	bool PointSphereTest(const dReal* center, const dReal radius,
		const dReal* pt,dReal* norm,dReal &depth);
		int dSortedTriSphere(const dReal*	/**v1/**/,const dReal*	/**v2/**/,
							const dReal* triAx,
							CDB::TRI* T,
							dReal dist,
							dxGeom* Sphere,
							dxGeom* Geometry, 
							int Flags, 
							dContactGeom* Contacts,
							int			skip
							);
IC	float	dSphereProj(dxGeom* sphere,const dReal* /**normal/**/)
		{
			VERIFY (dGeomGetClass(sphere)== dSphereClass);
			return	dGeomSphereGetRadius(sphere);
		}
int dTriSphere(const dReal* v0,const dReal* v1,const dReal* v2,
			Triangle* T,
			dxGeom* Sphere,dxGeom* Geometry, int Flags, 
			dContactGeom* Contacts,int /**skip/**/);
IC	 float	dBoxProj(dxGeom* box,const dReal* normal);
IC			void	CrossProjLine(const dReal* pt1,const dReal* vc1,const dReal* pt2,const dReal* vc2,dReal* proj);
IC			void	CrossProjLine1(const dReal* pt1,const dReal* vc1,const dReal* pt2,const dReal* vc2,dReal* proj);
IC			bool	CrossProjLine14(const dReal* pt1,const dReal* vc1,const dReal* pt2,const dReal* vc2,dReal hside,dReal* proj);
IC			bool	IsPtInBx(const dReal* Pt,const dReal* BxP,const dReal* BxEx,const dReal* BxR);
IC			dReal	FragmentonBoxTest(const dReal* Pt1,const dReal* Pt2,const dReal* BxP,const dReal* BxEx,const dReal* R,dReal* norm,dReal* pos);
	int		dSortedTriBox (
			const dReal* triSideAx0,const dReal* triSideAx1,
			const dReal* triAx,
			//const dReal* v0,
			//const dReal* v1,
			//const dReal* v2,
			CDB::TRI* T,
			dReal dist,
			dxGeom *o1, dxGeom *o2,
			int flags, dContactGeom *contact, int skip
			);
int dTriBox (
			const dReal* v0,const dReal* v1,const dReal* v2,
			Triangle* T,
			dxGeom *o1, dxGeom *o2,
			int flags, dContactGeom *contact, int skip
			);
IC	bool	circleLineIntersection(const dReal* cn,const dReal* cp,dReal r,const dReal* lv,const dReal* lp,dReal sign,dVector3 point);
IC	float	dCylProj(dxGeom* cylinder,const dReal* normal);
	int		dSortedTriCyl (
									  const dReal* triSideAx0,const dReal* triSideAx1,
									  const dReal* triAx,
									  //const dReal* v0,
									  //const dReal* v1,
									  //const dReal* v2,
									  CDB::TRI* T,
									  dReal dist,
									  dxGeom *o1, dxGeom *o2,
									  int flags, dContactGeom *contact, int skip
									  );
		int dTriCyl				(
										const dReal* v0,const dReal* v1,const dReal* v2,
										Triangle* T,
										dxGeom *o1, dxGeom *o2,
										int flags, dContactGeom *contact, int skip
									);
IC		bool  cylinderCrossesLine(const dReal* p,const dReal* R,dReal hlz,
											 const dReal* v0,const dReal* v1,const dReal* l,dVector3 pos);
IC	float	Proj  (dxGeom* o,const dReal* normal);
IC	int		Collide(
					const dReal* v0,const dReal* v1,const dReal* v2,
					Triangle* T,
					dxGeom *o1,dxGeom *o2,
					int flags, dContactGeom *contact, int skip
					);
IC	int		CollidePlain(
					const dReal* triSideAx0,const dReal* triSideAx1,
					const dReal* triAx,
					CDB::TRI* T,
					dReal dist,
					dxGeom *o1, dxGeom *o2,
					int flags, dContactGeom *contact, int skip
					);
TRI_PRIMITIVE_COLIDE_CLASS_DECLARE(Cyl)
TRI_PRIMITIVE_COLIDE_CLASS_DECLARE(Sphere)
TRI_PRIMITIVE_COLIDE_CLASS_DECLARE(Box)
};

TRI_PRIMITIVE_COLIDE_CLASS_IMPLEMENT(Box)
TRI_PRIMITIVE_COLIDE_CLASS_IMPLEMENT(Cyl)
 TRI_PRIMITIVE_COLIDE_CLASS_IMPLEMENT(Sphere)
IC	void dcTriListCollider::VxToGlClTriState(u32 v,CDB::TRI*       T_array)
{
	//CDB::TRI*       T_array      = Level().ObjectSpace.GetStaticTris();
	xr_vector<int>::iterator LI=I+1;
	for(;E!=LI;++LI)
	{
		u32* verts=T_array[*LI].verts;
		flags8 &state  =gl_cl_tries_state[LI-B];
		if(verts[0]==v)state.set(fl_engaged_v0,TRUE);
		if(verts[1]==v)state.set(fl_engaged_v1,TRUE);
		if(verts[2]==v)state.set(fl_engaged_v2,TRUE);
	}
}
IC	void dcTriListCollider::SideToGlClTriState(u32 v0,u32 v1,CDB::TRI*       T_array)
{
	//      = Level().ObjectSpace.GetStaticTris();
	xr_vector<int>::iterator LI=I+1;
	for(;E!=LI;++LI)
	{
		u32* verts=T_array[*LI].verts;
		flags8 &state  =gl_cl_tries_state[LI-B];
		if(verts[0]==v1&&verts[1]==v0)state.set(fl_engaged_s0,TRUE);
		if(verts[1]==v1&&verts[2]==v0)state.set(fl_engaged_s1,TRUE);
		if(verts[2]==v1&&verts[0]==v0)state.set(fl_engaged_s2,TRUE);
	}
}


#endif	//__DCTRILISTCOLLIDER_INCLUDED__(