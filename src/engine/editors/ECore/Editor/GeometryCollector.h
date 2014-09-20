#ifndef GeometryCollectorH
#define GeometryCollectorH
//---------------------------------------------------------------------------

struct ECORE_API GCVertex{
	Fvector				pos;
    u32					refs;
    					GCVertex	(const Fvector& p){pos=p;refs=1;}
	bool				similar		(const GCVertex& v, float eps=EPS){return pos.similar(v.pos);}
};

struct ECORE_API GCFace{
	u32 				verts[3];
    bool				valid;
    u32					dummy;
};

class ECORE_API VCPacked
{
protected:
	DEFINE_VECTOR(U32Vec,GCHash,GCHashIt);

    xr_vector<GCVertex>	verts;

    GCHash				VM;
    Fvector				VMmin, VMscale;
    Fvector				VMeps;
    float				eps;
    u32					sx,sy,sz;

    IC U32Vec&			get_element	(u32 ix, u32 iy, u32 iz)	{VERIFY((ix<sx)&&(iy<sy)&&(iz<sz)); return VM[iz*sy*sx+iy*sx+ix];}
public:
    					VCPacked	(const Fbox &bb, float eps=EPS, u32 clpSX=24, u32 clpSY=16, u32 clpSZ=24, int apx_vertices=5000);
	virtual				~VCPacked	()	{ clear();					}
    virtual void		clear		();

    u32					add_vert	(const Fvector& V);

    GCVertex*			getV		()	{ return &*verts.begin();	}
    size_t				getVS		()	{ return verts.size();		}
    xr_vector<GCVertex>&Vertices	()	{ return verts;				}

    void				getHASH_size(u32& x, u32& y, u32& z){x=sx;y=sy;z=sz;}
    U32Vec&				getHASH_elem(u32 ix, u32 iy, u32 iz){return get_element(ix,iy,iz);}
};

class ECORE_API GCPacked: public VCPacked
{
    xr_vector<GCFace>	faces;
    void				validate   	(GCFace& F)
    {
		if ((F.verts[0]==F.verts[1])||(F.verts[0]==F.verts[2])||(F.verts[1]==F.verts[2]))	F.valid=false;
        else																				F.valid=true;
    }
public:
    					GCPacked	(const Fbox &bb, float eps=EPS, u32 clpMX=24, u32 clpMY=16, u32 clpMZ=24, int apx_vertices=5000, int apx_faces=5000):
			                        VCPacked(bb,eps,clpMX,clpMY,clpMZ,apx_vertices){faces.reserve(apx_faces);}
	virtual				~GCPacked	()	{ clear();					}
    virtual void		clear		();

    xr_vector<GCFace>&	Faces		()	{ return faces;				}

	void				add_face	(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy=0);

    GCFace*				getF		()	{ return &*faces.begin();	}
    size_t				getFS		()	{ return faces.size();		}

    void				calc_adjacency(U32Vec& dest);
};

#endif
