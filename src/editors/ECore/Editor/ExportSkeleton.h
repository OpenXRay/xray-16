#ifndef ExportSkeletonH
#define ExportSkeletonH

#include "PropSlimTools.h"
//#include "../../../Layers/xrRender/SkeletonCustom.h"
#include "EditMesh.h"
//---------------------------------------------------------------------------
const int clpSMX = 28, clpSMY=16, clpSMZ=28;
//---------------------------------------------------------------------------
// refs                                           
class CEditableObject;
class CSurface;
class CInifile;
extern ECORE_API BOOL g_force16BitTransformQuant;
extern ECORE_API float g_EpsSkelPositionDelta;

struct ECORE_API SSkelVert: public st_SVert{
    Fvector		tang;
    Fvector		binorm;
	SSkelVert(){
        uv.set	(0.f,0.f);
        offs.set	(0,0,0);
		norm.set	(0,1,0);
        tang.set	(1,0,0);
		binorm.set	(0,0,1);
	}
    void set(const Fvector& _o, const Fvector& _n, const Fvector2& _uv, u8 _w_cnt, const st_SVert::bone* b)
    {
        offs.set 	(_o);
        norm.set	(_n);
        uv.set		(_uv);
        VERIFY		(_w_cnt>0 && _w_cnt<=4);
        bones.resize(_w_cnt);

        for (u8 k=0; k<_w_cnt; k++)
        {
        	bones[k]=b[k];
        }
        sort_by_bone(); // need to similar 
    }
	BOOL	similar_pos(SSkelVert& V)
    {
        return offs.similar(V.offs,g_EpsSkelPositionDelta);
    }
	BOOL	similar(SSkelVert& V)
    {
    	if (bones.size()!=V.bones.size())	return FALSE;
        for (u8 k=0; k<(u8)bones.size(); k++)
        {
	        if (!bones[k].similar(V.bones[k]))
            	return FALSE;
        }
        if (!uv.similar	(V.uv,EPS_S))
        	return FALSE;

		if (!offs.similar(V.offs,g_EpsSkelPositionDelta))
        	return FALSE;

		if (!norm.similar(V.norm,g_EpsSkelPositionDelta))
        	return FALSE;

		return TRUE;
	}
};

struct ECORE_API SSkelFace{
	WORD		v[3];
};

DEFINE_VECTOR(SSkelVert,SkelVertVec,SkelVertIt);
DEFINE_VECTOR(SSkelFace,SkelFaceVec,SkelFaceIt);

class ECORE_API CSkeletonCollectorPacked
{
protected:
    SkelVertVec		m_Verts;
    SkelFaceVec		m_Faces;

    Fvector			m_VMmin, m_VMscale;
    U32Vec			m_VM[clpSMX+1][clpSMY+1][clpSMZ+1];
    Fvector			m_VMeps;

    u16				VPack(SSkelVert& V);
public:
    u32 			invalid_faces;
public:
    CSkeletonCollectorPacked	(const Fbox &bb, int apx_vertices=5000, int apx_faces=5000);
    bool 			check      	(SSkelFace& F){
		if ((F.v[0]==F.v[1]) || (F.v[0]==F.v[2]) || (F.v[1]==F.v[2])) return false;
        for (SkelFaceIt f_it=m_Faces.begin(); f_it!=m_Faces.end(); f_it++){
            // Test for 6 variations
            if ((f_it->v[0]==F.v[0]) && (f_it->v[1]==F.v[1]) && (f_it->v[2]==F.v[2])) return false;
            if ((f_it->v[0]==F.v[0]) && (f_it->v[2]==F.v[1]) && (f_it->v[1]==F.v[2])) return false;
            if ((f_it->v[2]==F.v[0]) && (f_it->v[0]==F.v[1]) && (f_it->v[1]==F.v[2])) return false;
            if ((f_it->v[2]==F.v[0]) && (f_it->v[1]==F.v[1]) && (f_it->v[0]==F.v[2])) return false;
            if ((f_it->v[1]==F.v[0]) && (f_it->v[0]==F.v[1]) && (f_it->v[2]==F.v[2])) return false;
            if ((f_it->v[1]==F.v[0]) && (f_it->v[2]==F.v[1]) && (f_it->v[0]==F.v[2])) return false;
        }
        return true;
    }
	bool add_face	(SSkelVert& v0, SSkelVert& v1, SSkelVert& v2)
    {
		if (v0.offs.similar(v1.offs,EPS) || v0.offs.similar(v2.offs,EPS) || v1.offs.similar(v2.offs,EPS)){
			ELog.Msg(mtError,"Degenerate face found. Removed.");
            invalid_faces++;
            return false;
        }
        SSkelFace F;
        F.v[0]	= VPack(v0);
        F.v[1]	= VPack(v1);
        F.v[2]	= VPack(v2);
        if (check(F)){ 
        	m_Faces.push_back	(F);
	        return 				true;
        }else{	
        	ELog.Msg(mtError,"Duplicate face found. Removed.");
            invalid_faces++;
            return false;
        }
    }
    SkelVertVec& 	getV_Verts()	{return m_Verts;}
    SkelFaceVec& 	getV_Faces()	{return m_Faces;}
    SSkelVert*		getVert()		{ return &m_Verts[0];/*.begin();*/	}
    int				getVS()			{ return m_Verts.size();	}
//    SSkelFace&		getFace(int id)	{ return VERIFY(id<m_Faces.size()); m_Faces[id];	}
    int				getTS() 		{ return m_Faces.size();	}
};
//----------------------------------------------------

class ECORE_API CExportSkeletonCustom{
protected:
    struct ECORE_API SSplit: public CSkeletonCollectorPacked{
    	shared_str		m_Shader;
        shared_str		m_Texture;
        u16 			m_PartID;
        Fbox			m_Box;
        U16Vec			m_UsedBones;

        // Progressive
		ArbitraryList<VIPM_SWR>	m_SWR;// The records of the collapses.
	    u32				m_SkeletonLinkType;
    public:
        SSplit			(CSurface* surf, const Fbox& bb, u16 part);

        bool			valid()
        {
        	if (m_Verts.empty()) return false;
        	if (m_Faces.empty()) return false;
            return true;
        }
		void 			MakeProgressive				();
        void			MakeStripify				();
		void 			CalculateTB					();
		void 			OptimizeTextureCoordinates	();

        void 			Save			(IWriter& F);

        void			ComputeBounding	()
        {
            // calculate BBox
            m_Box.invalidate	();
            for (SkelVertIt v_it=m_Verts.begin(); v_it!=m_Verts.end(); v_it++){
                SSkelVert& pV 	= *v_it;
                m_Box.modify(pV.offs);
            }
        }
    };
	DEFINE_VECTOR		(SSplit,SplitVec,SplitIt);
	SplitVec			m_Splits;
    Fbox 				m_Box;
//----------------------------------------------------
    int     			FindSplit			(shared_str shader, shared_str texture, u16 part_id);
    void				ComputeBounding	()
    {
        m_Box.invalidate();
        for (SplitIt it=m_Splits.begin(); it!=m_Splits.end(); it++){
            it->ComputeBounding	();
            m_Box.merge			(it->m_Box);
        }
    }
public:
    virtual bool    	Export				(IWriter& F, u8 infl)=0;
};


class ECORE_API CExportSkeleton: public CExportSkeletonCustom{
	CEditableObject*	m_Source;
    bool				PrepareGeometry		(u8 influence);
public:
						CExportSkeleton		(CEditableObject* object);
    virtual bool    	Export				(IWriter& F, u8 infl);
    virtual bool    	ExportGeometry		(IWriter& F, u8 infl);
    virtual bool    	ExportMotions		(IWriter& F);

    virtual bool    	ExportMotionKeys	(IWriter& F);
    virtual bool    	ExportMotionDefs	(IWriter& F);
    bool                ExportAsSimple		(IWriter& F);
};

void ECORE_API 			ComputeOBB_RAPID	(Fobb &B, FvectorVec& V, u32 t_cnt);
void ECORE_API 			ComputeOBB_WML		(Fobb &B, FvectorVec& V);

#endif
