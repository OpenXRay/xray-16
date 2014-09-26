#ifndef GeometryPartExtractorH
#define GeometryPartExtractorH

#include "../ECore/Editor/GeometryCollector.h"
#include "../ECore/Editor/ExportSkeleton.h"
#include "Scene.h"

struct 	SBFace;
class 	CSurface;
DEFINE_VECTOR		(SBFace*,SBFaceVec,SBFaceVecIt);

struct SBFace{
// internal
    int 			vert_id[3];
    bool			marked;
    SBFaceVec		adjs;
// geom
    Fvector			o[3];
	Fvector			n[3];
    Fvector2		uv[3];
    int				bone_id;
    CSurface*		surf;
public:		
					SBFace	(CSurface* _surf, const Fvector2* _uv[3]):surf(_surf),marked(false),bone_id(-1)
	{
    	vert_id[0]	= -1;
    	vert_id[1]	= -1;
    	vert_id[2]	= -1;
        uv[0]		= *_uv[0];
        uv[1]		= *_uv[1];
        uv[2]		= *_uv[2];
        n[0].set	(0,0,0);
        n[1].set	(0,0,0);
        n[2].set	(0,0,0);
    }
    float			CalcArea()
    {
    	Fvector V0,V1;
        V0.sub		(o[1],o[0]);
        V1.sub		(o[2],o[0]);
        float sqm0	= V0.square_magnitude();
        float sqm1	= V1.square_magnitude();
        return		0.5f*_sqrt(sqm0*sqm1-_sqr(V0.dotproduct(V1)));
    }
};

struct SBBone
{
	AnsiString		mtl;
	AnsiString		name;
	AnsiString		parent;
	Fvector			offset;
    u32				f_cnt;
    float			area;
				    SBBone				(AnsiString _nm, AnsiString _parent, AnsiString _mtl, u32 _f_cnt, float _area)
                    					:name(_nm),parent(_parent),mtl(_mtl),f_cnt(_f_cnt),area(_area)
    {
    	offset.set	(0,0,0);
    }
};
DEFINE_VECTOR		(SBBone,SBBoneVec,SBBoneVecIt);
DEFINE_VECTOR		(SBFaceVec,SBAdjVec,SBAdjVecIt);

struct SBPart: public CExportSkeletonCustom
{
	SBFaceVec		m_Faces;
    SBBoneVec		m_Bones;

    Fbox			m_BBox;
    Fobb			m_OBB;

    SBPart*			m_Reference;
    
    Fvector			m_RefOffset;
    Fvector			m_RefRotate;

    bool 			m_bValid;
public:
					SBPart				()			{m_Reference=0;m_bValid=true;}
	bool			Valid				()			{return m_bValid;}
    virtual bool 	Export				(IWriter& F, u8 infl);
	void			append_face			(SBFace* F);
    void			use_face			(SBFace* F, u32& cnt, u32 bone_id, float& area);
    void			recurse_fragment	(SBFace* F, u32& cnt, u32 bone_id, u32 max_faces, float& area);
    bool			prepare				(SBAdjVec& adjs, u32 bone_face_min);
};
DEFINE_VECTOR		(SBPart*,SBPartVec,SBPartVecIt);
//----------------------------------------------------

class CGeomPartExtractor{
protected:
	VCPacked* 		m_Verts;
    SBFaceVec 		m_Faces;       
    SBAdjVec		m_Adjs;
    SBPartVec 		m_Parts;

    u32 			m_PerBoneFaceCountMin;
public:
					CGeomPartExtractor	();
                	~CGeomPartExtractor	(){Clear();}
	// init & clean up
	void			Initialize			(const Fbox& bb, float eps, u32 per_bone_face_count_min);
    void			Clear				();
	// I/O routines
	void			AppendFace			(CSurface* surf, const Fvector* v, const Fvector* n, const Fvector2* uvs[3]);
    SBPartVec&		GetParts			(){return m_Parts;}
    // utilities
    BOOL			Process				();
};

#endif //GeometryPartExtractorH
