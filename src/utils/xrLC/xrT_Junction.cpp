#include "stdafx.h"
#include "build.h"
#include "../xrLC_Light/xrLC_GlobalData.h"
#include "../xrLC_Light/xrface.h"
//#define VPUSH(a) ((a).x), ((a).y), ((a).z)

IC float	SqrDistance2Segment(const Fvector& P, const Fvector& A, const Fvector& B)
{
	// Determine t (the length of the vector from ‘a’ to ‘p’)
	Fvector c; c.sub(P,A);
	Fvector V; V.sub(B,A); 
	
	float d = V.magnitude	();
	
	V.div	(d); 
	float t = V.dotproduct	(c);
	
	// Check to see if ‘t’ is beyond the extents of the line segment
	if (t <= 0.0f)	return P.distance_to_sqr(A);
	if (t >= d)		return P.distance_to_sqr(B);
	
	// Return the point between ‘a’ and ‘b’
	// set length of V to t. V is normalized so this is easy
	Fvector	R;	R.mad		(A,V,t);
	return P.distance_to_sqr(R);
}

struct record
{
	Vertex	*E1,*E2;
	Vertex	*T;
};

static xr_vector<record>*	vecJunctions;
static xr_vector<record>*	vecEdges;

static void check(Vertex* vE1, Vertex* vE2, Vertex* vTEST)
{
	if (_sqrt(SqrDistance2Segment(vTEST->P,vE1->P,vE2->P))<0.002f)	
	{
		BOOL bWeld = FALSE;
		
		// check for duplicated errors
		if (vE1>vE2)	std::swap(vE1,vE2);
		for (u32 i=0; i<vecJunctions->size(); i++)
		{
			record&	rec = (*vecJunctions)[i];
			if (rec.T==vTEST)						return;
			if (rec.T->P.similar(vTEST->P,.002f))	bWeld = TRUE;
		}
		
		// register
		record					rec;
		rec.E1					= vE1;
		rec.E2					= vE2;
		rec.T					= vTEST;
		vecJunctions->push_back	(rec);
		
		// display
		// if (bWeld)	clMsg	("ERROR. unwelded vertex      [%3.1f,%3.1f,%3.1f]",	VPUSH(vTEST->P));
		// else			clMsg	("ERROR. T-junction at vertex [%3.1f,%3.1f,%3.1f]",	VPUSH(vTEST->P));
		//pBuild->err_tjunction.w_fvector3	(vTEST->P);
	}
}

static void edge(Vertex* vE1, Vertex* vE2)
{
	float		len	= vE1->P.distance_to(vE2->P);
	if (len<32.f)	return;

	// check for duplicated errors
	if (vE1>vE2)	std::swap(vE1,vE2);
	for (u32 i=0; i<vecEdges->size(); i++)
	{
		record&	rec = (*vecEdges)[i];
		if ((rec.E1==vE1)&&(rec.E2==vE2))		return;
	}
	
	// register
	record	rec;
	rec.E1	= vE1;
	rec.E2	= vE2;
	rec.T	= 0;
	vecEdges->push_back	(rec);
	
	// clMsg	("ERROR: too long edge        %3.1fm [%3.1f,%3.1f,%3.1f] - [%3.1f,%3.1f,%3.1f]",len,VPUSH(vE1->P),VPUSH(vE2->P));
}

void CBuild::CorrectTJunctions()
{
	Status					("Processing...");
	vecJunctions			= xr_new<xr_vector<record> > (); vecJunctions->reserve	(1024);
	vecEdges				= xr_new<xr_vector<record> > (); vecEdges->reserve		(1024);

	for (u32 I=0; I<lc_global_data()->g_faces().size(); ++I)
	{
		Face* F = lc_global_data()->g_faces()[I];

		// Iterate on edges
		for (u32 e=0; e<3; ++e)
		{
			Vertex			*vA,*vB;
			F->EdgeVerts	(e,&vA,&vB);

			// Iterate on 'vA'-adjacent faces
			for (u32 f1=0; f1!=vA->m_adjacents.size(); ++f1)
			{
				Face*	F1	= vA->m_adjacents[f1];

				// Iterate on it's edges
				for (u32 e1=0; e1<3; ++e1)
				{
					Vertex			*v1,*v2;
					F1->EdgeVerts	(e1,&v1,&v2);
					edge			(v1,v2);

					if (v1==vA && v2!=vB)		
						check(vA,vB,v2);
					else 
					if (v2==vA && v1!=vB)	
						check(vA,vB,v1);
				}
			}
			// Iterate on 'vB'-adjacent faces
			for (u32 f2=0; f2!=vB->m_adjacents.size(); ++f2)
			{
				Face*	F2	= vB->m_adjacents[f2];

				// Iterate on it's edges
				for (u32 e1=0; e1<3; ++e1)
				{
					Vertex			*v1,*v2;
					F2->EdgeVerts	(e1,&v1,&v2);
					edge			(v1,v2);
					if (v1==vB && v2!=vA)		
						check		(vA,vB,v2);
					else 
					if (v2==vB && v1!=vA)	
						check		(vA,vB,v1);
				}
			}
		}
		Progress(float(I)/float(lc_global_data()->g_faces().size()));
	}
	clMsg("*** %d junctions and %d long edges found.",vecJunctions->size(),vecEdges->size());

	xr_delete(vecJunctions);
	xr_delete(vecEdges);
}
