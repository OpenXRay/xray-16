//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "GeometryCollector.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

//------------------------------------------------------------------------------
// VCPacked
//------------------------------------------------------------------------------
VCPacked::VCPacked(const Fbox &bb, float _eps, u32 _sx, u32 _sy, u32 _sz, int apx_vertices)
{
	eps				= _eps;
	sx				= _max(_sx,1);
	sy				= _max(_sy,1);
	sz				= _max(_sz,1);
	// prepare hash table
	VM.resize		(sx*sy*sz);
            
    // Params
    VMscale.set		(bb.max.x-bb.min.x, bb.max.y-bb.min.y, bb.max.z-bb.min.z);
    VMmin.set		(bb.min);
    VMeps.set		(VMscale.x/(sx-1)/2,VMscale.y/(sy-1)/2,VMscale.z/(sz-1)/2);
    VMeps.x			= (VMeps.x<EPS_L)?VMeps.x:EPS_L;
    VMeps.y			= (VMeps.y<EPS_L)?VMeps.y:EPS_L;
    VMeps.z			= (VMeps.z<EPS_L)?VMeps.z:EPS_L;

    // Preallocate memory
    verts.reserve	(apx_vertices);

    int		_size	= VM.size();
    int		_average= (apx_vertices/_size)/2;
    for (GCHashIt it=VM.begin(); it!=VM.end(); it++)
        it->reserve	(_average);
}

u32		VCPacked::add_vert(const Fvector& V)
{
    u32 P = 0xffffffff;

    u32 clpX=sx-1, clpY=sy-1, clpZ=sz-1;

    u32 ix,iy,iz;
    ix = iFloor(float(V.x-VMmin.x)/VMscale.x*clpX);
    iy = iFloor(float(V.y-VMmin.y)/VMscale.y*clpY);
    iz = iFloor(float(V.z-VMmin.z)/VMscale.z*clpZ);

    clamp(ix,(u32)0,clpX);	
    clamp(iy,(u32)0,clpY);	
    clamp(iz,(u32)0,clpZ);

    {
	    U32Vec& vl 	= get_element(ix,iy,iz);
        for(U32It it=vl.begin();it!=vl.end(); it++)
            if( verts[*it].similar(V,eps) )	{
                P = *it;
                verts[*it].refs++;
                break;
            }
    }
    if (0xffffffff==P)
    {
        P = verts.size();
        verts.push_back(GCVertex(V));

        get_element(ix,iy,iz).push_back(P);

        u32 ixE,iyE,izE;
        ixE = iFloor(float(V.x+VMeps.x-VMmin.x)/VMscale.x*clpX);
        iyE = iFloor(float(V.y+VMeps.y-VMmin.y)/VMscale.y*clpY);
        izE = iFloor(float(V.z+VMeps.z-VMmin.z)/VMscale.z*clpZ);

        //			R_ASSERT(ixE<=clpMX && iyE<=clpMY && izE<=clpMZ);
        clamp(ixE,(u32)0,clpX);	clamp(iyE,(u32)0,clpY);	clamp(izE,(u32)0,clpZ);

        if (ixE!=ix)							get_element(ixE,iy,iz).push_back	(P);
        if (iyE!=iy)							get_element(ix,iyE,iz).push_back	(P);
        if (izE!=iz)							get_element(ix,iy,izE).push_back	(P);
        if ((ixE!=ix)&&(iyE!=iy))				get_element(ixE,iyE,iz).push_back	(P);
        if ((ixE!=ix)&&(izE!=iz))				get_element(ixE,iy,izE).push_back	(P);
        if ((iyE!=iy)&&(izE!=iz))				get_element(ix,iyE,izE).push_back	(P);
        if ((ixE!=ix)&&(iyE!=iy)&&(izE!=iz))	get_element(ixE,iyE,izE).push_back	(P);
    }
    return P;
}

void	VCPacked::clear()
{
    verts.clear_and_free	();
    for (GCHashIt it=VM.begin(); it!=VM.end(); it++)
        it->clear_and_free	();
}

//------------------------------------------------------------------------------
// GCPacked
//------------------------------------------------------------------------------
void	GCPacked::add_face(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy)
{
	GCFace T;
    T.verts	[0] 	= add_vert(v0);
    T.verts	[1] 	= add_vert(v1);
    T.verts	[2] 	= add_vert(v2);
    T.dummy			= dummy;
    faces.push_back	(T);
    validate		(T);
}

void	GCPacked::clear()
{
	GCPacked::clear	();
    faces.clear_and_free	();
}

void	GCPacked::calc_adjacency	(U32Vec& dest)
{
    dest.assign		(faces.size()*3,0xffffffff);
    // Dumb algorithm O(N^2) :)
    for (u32 f=0; f<faces.size(); f++)
    {
        for (u32 t=0; t<faces.size(); t++)
        {
            if (t==f)	continue;

            for (u32 f_e=0; f_e<3; f_e++)
            {
                u32 f1	= faces[f].verts[(f_e+0)%3];
                u32 f2	= faces[f].verts[(f_e+1)%3];
                if (f1>f2)	std::swap(f1,f2);

                for (u32 t_e=0; t_e<3; t_e++)
                {
                    u32 t1	= faces[t].verts[(t_e+0)%3];
                    u32 t2	= faces[t].verts[(t_e+1)%3];
                    if (t1>t2)	std::swap(t1,t2);

                    if (f1==t1 && f2==t2)
                    {
                        // f.edge[f_e] linked to t.edge[t_e]
                        dest[f*3+f_e]	= t;
                        break;
                    }
                }
            }
        }
    }
}

