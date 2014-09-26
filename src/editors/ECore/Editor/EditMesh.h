//----------------------------------------------------
// file: StaticMesh.h
//----------------------------------------------------
#ifndef EditMeshH
#define EditMeshH

//----------------------------------------------------
// refs
class 	CSurface;
struct 	SRayPickInfo;
//struct CFrustum;
struct 	FSChunkDef;
class 	CExporter;
class	CCustomObject;

#ifdef _EDITOR
#	include "pick_defs.h"
#endif

#include <bone.h>

#pragma pack( push,1 )
const u8	vmtUV		= 0;
const u8	vmtWeight	= 1;

struct ECORE_API st_WB{                       
	u16 	bone;
	float 	weight;
			st_WB	():bone(BI_NONE),weight(0.f){;}
			st_WB	(u16 b, float w):bone(b),weight(w){;}
	void	set		(u16 b, float w){bone=b;weight=w;}
};
DEFINE_VECTOR(st_WB,WBVec,WBIt);
struct ECORE_API st_VertexWB:public WBVec{
protected:
	static bool compare_by_weight(const st_WB& a, const st_WB& b)
	{
		return a.weight > b.weight; // отсортировать по убыванию
	}
	static bool compare_by_bone(const st_WB& a, const st_WB& b)
	{
		return a.bone < b.bone; // отсортировать по возрастанию
	}
public:
	void sort_by_bone()
	{
		std::sort(begin(),end(),compare_by_bone);
	}
	void sort_by_weight()
	{
		std::sort(begin(),end(),compare_by_weight);
	}
	void prepare_weights(u32 max_influence)
	{
        sort_by_weight	();
        // delete >max_influence weights
        if (size()>max_influence) erase(begin()+max_influence,end()); 
		// accumulate weights
        float sum_weight=0;
        WBIt it;
        for (it=begin(); it!=end(); it++) sum_weight+=it->weight;
        // normalize weights
        for (it=begin(); it!=end(); it++) it->weight/=sum_weight;
//		sort_by_bone	(); // need only for export (before add vertex sort_by_bone)
	}
};
DEFINE_VECTOR(st_VertexWB,VWBVec,VWBIt);

struct ECORE_API st_VMapPt{
	int				vmap_index;	// ссылка на мапу
	int				index;		// индекс в V-мапе на uv/w
	st_VMapPt(){vmap_index=-1;index=-1;}
};
// uv's
class ECORE_API st_VMap{
    FloatVec    	vm;			// u,v - координаты или weight
public:
	shared_str		name;		// vertex uv map name
    struct{
        u8			type	:2;
        u8			polymap	:1;
        u8			dim		:2;
        u8			reserved:3;
    };
	IntVec			vindices;
	IntVec			pindices;
public:
	st_VMap			(LPCSTR nm=0, u8 t=vmtUV, bool pm=false)
    {
		type		= t;
		polymap		= pm;
		name		= nm;
		if (t==vmtUV) dim=2; else dim=1;
	}
	IC const Fvector2& getUV	(int idx) const			{VERIFY(type==vmtUV);		return (Fvector2&)vm[idx*dim];}
	IC Fvector2&    getUV		(int idx)				{VERIFY(type==vmtUV);		return (Fvector2&)vm[idx*dim];}
    IC const float&	getW		(int idx) const			{VERIFY(type==vmtWeight);	return vm[idx];}
    IC const FloatVec& getVM	()	const				{return vm;}
	IC float*		getVMdata	()						{return &*vm.begin();}
    IC float*		getVMdata	(int start)				{return &*(vm.begin()+start*dim);}
    IC int			VMdatasize	()						{return vm.size()*sizeof(float);}
    IC int*			getVIdata	()						{return &*vindices.begin();}
    IC int			VIdatasize	()						{return vindices.size()*sizeof(int);}
    IC int*			getPIdata	()						{return &*pindices.begin();}
    IC int			PIdatasize	()						{return pindices.size()*sizeof(int);}
    IC int			size		()						{return vm.size()/dim;}
    IC void			resize		(int cnt)				{vm.resize(cnt*dim);vindices.resize(cnt);if (polymap) pindices.resize(cnt); }
	IC void			appendUV	(const float u, const float v)	{vm.push_back(u);vm.push_back(v);}
	IC void			appendUV	(const Fvector2& uv)			{appendUV(uv.x,uv.y);}
	IC void			appendW		(const float w)			{vm.push_back(w);}
	IC void			appendVI	(const int vi)				{vindices.push_back(vi);}
	IC void			appendPI	(const int pi)				{VERIFY(polymap); pindices.push_back(pi);}
    IC void			copyfrom	(float* src, int cnt)	{resize(cnt); CopyMemory(&*vm.begin(),src,cnt*dim*4);}
};

struct st_VMapPtLst{
	u8				count;
	st_VMapPt*		pts;
};
DEFINE_VECTOR		(st_VMapPtLst,VMRefsVec,VMRefsIt);

struct ECORE_API st_SVert{
	Fvector			offs;
    Fvector			norm;
    Fvector2		uv;
    struct bone{
    public:
    	float		w;
        u16			id;
        bone		(){w=0.f;id=BI_NONE;}
        bool		similar		(const bone& b){return (id==b.id) && fsimilar(w,b.w,EPS_L);}
    };
    svector<bone,4> bones;
    protected:
        static bool compare_by_weight(const bone& a, const bone& b)
        {
            return a.w > b.w; // отсортировать по убыванию
        }
        static bool compare_by_bone(const bone& a, const bone& b)
        {
            return a.id < b.id; // отсортировать по возрастанию
        }
    public:
    	void		sort_by_weight	()	{std::sort(bones.begin(),bones.end(),compare_by_weight);}
    	void		sort_by_bone	()	{std::sort(bones.begin(),bones.end(),compare_by_bone);	}
};
// faces
struct st_Face;
struct ECORE_API st_FaceVert{
typedef	st_Face	type_face;
		int 	pindex;		// point index in PointList
		int		vmref;		// vm-ref index 
		bool	gt( const st_FaceVert &v ) const { return pindex > v.pindex;  }
		bool	eq( const st_FaceVert &v ) const { return pindex == v.pindex;  }
};
struct ECORE_API st_Face{
    st_FaceVert		pv[3];		// face vertices (P->P...)
	void EdgeVerts( u8 e, st_FaceVert &v0, st_FaceVert &v1 ) const
	{
		VERIFY( e< 3 );
		v0 = pv[ e ];
		v1 = pv[ (e+1)%3 ]; 
	}
};

// mesh options
struct ECORE_API st_MeshOptions{
	int 			m_Reserved0;
	int 			m_Reserved1;
    st_MeshOptions	(){m_Reserved0=0;m_Reserved1=0;}
};
#pragma pack( pop )

DEFINE_VECTOR		(IntVec,AdjVec,AdjIt);
DEFINE_VECTOR		(st_VMap*,VMapVec,VMapIt);
DEFINE_MAP			(CSurface*,IntVec,SurfFaces,SurfFacesPairIt);

//refs
struct st_RenderBuffer;
class CSurface;
class CSector;

#ifdef _EDITOR
	struct ECORE_API st_RenderBuffer{
		u32			dwStartVertex;
	    u32			dwNumVertex;
        ref_geom 	pGeom;
		st_RenderBuffer	(u32 sv, u32 nv):dwStartVertex(sv),dwNumVertex(nv),pGeom(0){;}
	};
	DEFINE_VECTOR(st_RenderBuffer,RBVector,RBVecIt);
	DEFINE_MAP(CSurface*,RBVector,RBMap,RBMapPairIt);
#endif

class ECORE_API CEditableMesh {
	friend class MeshExpUtility;
	friend class CEditableObject;
    friend class CSectorItem;
    friend class CSector;
    friend class CPortalUtils;
    friend class SceneBuilder;
    friend class CExportSkeleton;
    friend class CExportObjectOGF;
    friend class TfrmEditLibrary;
	friend class CExporter;
	friend class CXRayObjectExport;
	friend class CXRaySkinExport;

	shared_str			m_Name;

    CEditableObject*	m_Parent;

    void            GenerateCFModel		();
	void 			GenerateRenderBuffers();
    void			UnloadCForm     	();
#ifdef _EDITOR
	void 			UnloadRenderBuffers	();
#endif
public:
	static 			BOOL m_bDraftMeshMode;
    void 			GenerateFNormals	();
    void 			GenerateVNormals	(const Fmatrix* parent_xform);
    void            GenerateSVertices	(u32 influence);
	void 			GenerateAdjacency	();

    bool			IsGeneratedSVertices(u32 influence)		{return (m_SVertices && (m_SVertInfl==influence));}
    
    void			UnloadFNormals   	(bool force=false);
    void			UnloadVNormals   	(bool force=false);
    void			UnloadSVertices  	(bool force=false);
    void			UnloadAdjacency  	(bool force=false);
IC  Fvector*	    Vertices			()					{ return m_Vertices; }	//
IC	st_Face*	    Faces				()					{ return m_Faces; }   // + some array size!!!
IC  SurfFaces	    &Surfaces			()					{ return m_SurfFaces; }
private:
    // internal variables
	enum{
		flVisible	= (1<<0),
		flLocked	= (1<<1),
		flSGMask	= (1<<2),
	};
	Flags8			m_Flags;
public:
	st_MeshOptions	m_Ops;
protected:
	Fbox			m_Box;

	int				m_FNormalsRefs;
	int				m_VNormalsRefs;
	int				m_AdjsRefs;
	int				m_SVertRefs;

    u32 			m_SVertInfl;
    
    u32				m_VertCount;
    u32				m_FaceCount;
    
    Fvector*	    m_Vertices;	// |
    AdjVec*			m_Adjs;    	// + some array size!!!
	u32*			m_SmoothGroups;		// |
    Fvector*		m_FaceNormals;	// |
    Fvector*		m_VertexNormals;	// | *3
    st_SVert*		m_SVertices;// | *3
    st_Face*	    m_Faces;    // + some array size!!!
    SurfFaces	    m_SurfFaces;
    VMapVec		    m_VMaps;
    VMRefsVec	    m_VMRefs;

#ifdef _EDITOR
    CDB::MODEL*		m_CFModel;
	RBMap*			m_RenderBuffers;
#endif

	void 			FillRenderBuffer		(IntVec& face_lst, int start_face, int num_face, const CSurface* surf, LPBYTE& data);

	void 			RecurseTri				(int id);


	// mesh optimize routine
	bool 			OptimizeFace			(st_Face& face);
public:
	void			RecomputeBBox			();
	void 			OptimizeMesh			(BOOL NoOpt);
public:
	                CEditableMesh			(CEditableObject* parent){m_Parent=parent;Construct();}
	                CEditableMesh			(CEditableMesh* source,CEditableObject* parent){m_Parent=parent;Construct();CloneFrom(source);}
	virtual         ~CEditableMesh			();
	void			Construct				();
    void			Clear					();

	IC void			SetName					(LPCSTR name){m_Name=name;}
	IC shared_str	Name					(){return m_Name;}
	void            GetBox					(Fbox& box){box.set(m_Box);}
	CSurface*		GetSurfaceByFaceID		(u32 fid);
	void			GetFaceTC				(u32 fid, const Fvector2* tc[3]);
	void			GetFacePT				(u32 fid, const Fvector* pt[3]);
	IC BOOL 		Visible					(){return m_Flags.is(flVisible); }
	IC void 		Show					(BOOL bVisible){m_Flags.set(flVisible,bVisible);}

    // mesh modify routine
	void            CloneFrom				(CEditableMesh *source);
	void            Transform				(const Fmatrix& parent);

	IC CEditableObject*	Parent				(){ return m_Parent;	}
    IC u32				GetFCount			(){ return m_FaceCount;	}
    IC const st_Face*	GetFaces			(){ return m_Faces;		}
	IC const u32*		GetSmoothGroups		(){ return m_SmoothGroups;	}
    IC const Fvector*	GetVertices			(){ return m_Vertices;		}
    IC u32				GetVCount			(){ return m_VertCount;	}
	IC const VMapVec&	GetVMaps			(){ return m_VMaps;		}
	IC const VMRefsVec&	GetVMRefs			(){ return m_VMRefs;	}
	IC const SurfFaces&	GetSurfFaces		(){ return m_SurfFaces;	}
    IC const Fvector*	GetFNormals			(){ VERIFY(0!=m_FaceNormals); return m_FaceNormals;	}
    IC const Fvector*	GetVNormals			(){ VERIFY(0!=m_VertexNormals); return m_VertexNormals;	}
    IC const st_SVert*	GetSVertices		(){ VERIFY(0!=m_SVertices);return m_SVertices;	}
	    
    // pick routine
	bool            RayPick					(float& dist, const Fvector& start, const Fvector& dir, const Fmatrix& inv_parent, SRayPickInfo* pinf = NULL);
#ifdef _EDITOR
	void            RayQuery				(SPickQuery& pinf);
	void            RayQuery				(const Fmatrix& parent, const Fmatrix& inv_parent, SPickQuery& pinf);
	void            BoxQuery				(const Fmatrix& parent, const Fmatrix& inv_parent, SPickQuery& pinf);
    bool 			BoxPick					(const Fbox& box, const Fmatrix& inv_parent, SBoxPickInfoVec& pinf);
	bool            FrustumPick				(const CFrustum& frustum, const Fmatrix& parent);
    void            FrustumPickFaces		(const CFrustum& frustum, const Fmatrix& parent, U32Vec& fl);
    bool			CHullPickMesh			(PlaneVec& pl, const Fmatrix& parent);
	void 			GetTiesFaces			(int start_id, U32Vec& fl, float fSoftAngle, bool bRecursive);
#endif

    // render routine
	void 			Render					(const Fmatrix& parent, CSurface* S);
	void 			RenderSkeleton			(const Fmatrix& parent, CSurface* S);
	void            RenderList				(const Fmatrix& parent, u32 color, bool bEdge, IntVec& fl);
	void 			RenderSelection			(const Fmatrix& parent, CSurface* s, u32 color);
	void 			RenderEdge				(const Fmatrix& parent, CSurface* s, u32 color);

    // statistics methods
    int 			GetFaceCount			(bool bMatch2Sided=true, bool bIgnoreOCC=true);
	int 			GetVertexCount			(){return m_VertCount;}
    int 			GetSurfFaceCount		(CSurface* surf, bool bMatch2Sided=true);
    float			CalculateSurfaceArea	(CSurface* surf, bool bMatch2Sided);
    float			CalculateSurfacePixelArea(CSurface* surf, bool bMatch2Sided);

    // IO - routine
    void			SaveMesh				(IWriter&);
	bool 			LoadMesh				(IReader&);

    // debug
    void			DumpAdjacency			();

	// convert
#ifdef _MAX_EXPORT
    void			FlipFaces				();
	TriObject*		ExtractTriObject		(INode *node, int &deleteIt);
	bool			Convert					(INode *node);
	bool			Convert					(CExporter* exporter);
#endif

	int				FindSimilarUV			(st_VMap* vmap, Fvector2& _uv);
	int				FindSimilarWeight		(st_VMap* vmap, float _w);
	int				FindVMapByName			(VMapVec& vmaps, const char* name, u8 t, bool polymap);
	void			RebuildVMaps			();

    bool			Validate				();
};
//----------------------------------------------------
#endif /*_INCDEF_EditableMesh_H_*/

