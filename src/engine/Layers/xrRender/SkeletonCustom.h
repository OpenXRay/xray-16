//---------------------------------------------------------------------------
#ifndef SkeletonCustomH
#define SkeletonCustomH

#include		"fhierrarhyvisual.h"
#include		"../../xrEngine/bone.h"
#include		"../../Include/xrRender/Kinematics.h"

// consts
extern	xrCriticalSection	UCalc_Mutex			;

// refs
class	 CKinematics;
class	 CInifile;
class	 CBoneData;
struct	SEnumVerticesCallback;

// MT-locker
struct	UCalc_mtlock	{
	UCalc_mtlock()		{ UCalc_Mutex.Enter(); }
	~UCalc_mtlock()		{ UCalc_Mutex.Leave(); }
};

#pragma warning(push)
#pragma warning(disable:4275)
class  CSkeletonWallmark : public intrusive_base // 4+4+4+12+4+16+16 = 60 + 4 = 64
{
#pragma warning(pop)
	CKinematics*		m_Parent;		// 4
	const Fmatrix*		m_XForm;		// 4
	ref_shader			m_Shader;		// 4
	Fvector3			m_ContactPoint;	// 12		model space
	float				m_fTimeStart;	// 4
public:
#ifdef DEBUG
	u32					used_in_render;	
#endif
	Fsphere				m_LocalBounds;	// 16		model space
	struct WMFace{
		Fvector3		vert	[3];
		Fvector2		uv		[3];
		u16				bone_id	[3][2];
		float			weight	[3];
	};
	DEFINE_VECTOR		(WMFace,WMFacesVec,WMFacesVecIt);
	WMFacesVec			m_Faces;		// 16 
public:
	Fsphere				m_Bounds;		// 16		world space
public:									
						CSkeletonWallmark	(CKinematics* p,const Fmatrix* m, ref_shader s, const Fvector& cp, float ts):
						m_Parent(p),m_XForm(m),m_Shader(s),m_fTimeStart(ts),m_ContactPoint(cp)
						{
#ifdef DEBUG
						used_in_render = u32(-1);
#endif
						
						}
						~CSkeletonWallmark	()
#ifdef DEBUG
							;
#else
							{}
#endif

	IC CKinematics*		Parent				(){return m_Parent;}
	IC u32				VCount				(){return m_Faces.size()*3;}
	IC bool				Similar				(ref_shader& sh, const Fvector& cp, float eps){return (m_Shader==sh)&&m_ContactPoint.similar(cp,eps);}
	IC float			TimeStart			(){return m_fTimeStart;}
	IC const Fmatrix*	XFORM				(){return m_XForm;}
	IC const Fvector3&	ContactPoint		(){return m_ContactPoint;}
	IC ref_shader		Shader				(){return m_Shader;}
};
DEFINE_VECTOR(intrusive_ptr<CSkeletonWallmark>,SkeletonWMVec,SkeletonWMVecIt);

// sanity check
#ifdef DEBUG
	struct dbg_marker{
		BOOL*			lock;
		dbg_marker		(BOOL* b)	{lock=b; VERIFY(*lock==FALSE); *lock=TRUE;}
		~dbg_marker		()			{*lock=FALSE;}
	};
#	define _DBG_SINGLE_USE_MARKER	dbg_marker	_dbg_marker(&dbg_single_use_marker)
#else
#	define _DBG_SINGLE_USE_MARKER
#endif

class 	CKinematics: public FHierrarhyVisual, public IKinematics
{
	typedef FHierrarhyVisual	inherited;
	friend class				CBoneData;
	friend class				CSkeletonX;
public: 
#ifdef DEBUG
	BOOL						dbg_single_use_marker;
#endif
			void				Bone_Calculate		(CBoneData* bd, Fmatrix* parent);
			void				CLBone				(const CBoneData* bd, CBoneInstance &bi, const Fmatrix *parent, u8 mask_channel = (1<<0));

			void				BoneChain_Calculate	(const CBoneData* bd, CBoneInstance &bi,u8 channel_mask, bool ignore_callbacks);
			void				Bone_GetAnimPos		(Fmatrix& pos,u16 id, u8 channel_mask, bool ignore_callbacks);


	virtual	void				BuildBoneMatrix		( const CBoneData* bd, CBoneInstance &bi, const Fmatrix *parent, u8 mask_channel = (1<<0) );
	virtual void				OnCalculateBones	(){}
	
public:
	dxRender_Visual*				m_lod;
protected:
	SkeletonWMVec				wallmarks;
	u32							wm_frame;

	xr_vector<dxRender_Visual*>	children_invisible	;

	// Globals
    CInifile*					pUserData;
	CBoneInstance*				bone_instances;	// bone instances
	vecBones*					bones;			// all bones	(shared)
	u16							iRoot;			// Root bone index

	// Fast search
	accel*						bone_map_N;		// bones  associations	(shared)	- sorted by name
	accel*						bone_map_P;		// bones  associations	(shared)	- sorted by name-pointer

	BOOL						Update_Visibility		;
	u32							UCalc_Time				;
	s32							UCalc_Visibox			;

    Flags64						visimask;
    
	CSkeletonX*					LL_GetChild				(u32 idx);

	// internal functions
	virtual CBoneData*			CreateBoneData			(u16 ID){return xr_new<CBoneData>(ID);}
	virtual void				IBoneInstances_Create	();
	virtual void				IBoneInstances_Destroy	();
	void						Visibility_Invalidate	()	{ Update_Visibility=TRUE; };
	void						Visibility_Update		()	;

    void						LL_Validate				();
public:
	UpdateCallback				Update_Callback;
	void*						Update_Callback_Param;
public:
	// wallmarks
	void						AddWallmark			(const Fmatrix* parent, const Fvector3& start, const Fvector3& dir, ref_shader shader, float size);
	void						CalculateWallmarks	();
	void						RenderWallmark		(intrusive_ptr<CSkeletonWallmark> wm, FVF::LIT* &verts);
	void						ClearWallmarks		();
public:
				
				bool			PickBone			(const Fmatrix &parent_xform, IKinematics::pick_result &r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id);
	virtual		void			EnumBoneVertices	(SEnumVerticesCallback &C, u16 bone_id);
public:
								CKinematics			();
	virtual						~CKinematics		();

	// Low level interface
				u16				_BCL	LL_BoneID			(LPCSTR  B);
				u16				_BCL	LL_BoneID			(const shared_str& B);
				LPCSTR			_BCL	LL_BoneName_dbg		(u16 ID);

				CInifile*		_BCL	LL_UserData			()						{return pUserData;}
				accel*					LL_Bones			()						{return bone_map_N;}
	ICF			CBoneInstance&	_BCL	LL_GetBoneInstance	(u16 bone_id)			{	VERIFY(bone_id<LL_BoneCount()); VERIFY(bone_instances); return bone_instances[bone_id];	}
	ICF const	CBoneInstance&	_BCL	LL_GetBoneInstance	(u16 bone_id) const		{	VERIFY(bone_id<LL_BoneCount()); VERIFY(bone_instances); return bone_instances[bone_id];	}
	CBoneData&					_BCL	LL_GetData			(u16 bone_id)
    {
    	VERIFY(bone_id<LL_BoneCount());
        VERIFY(bones);
        CBoneData& bd =  *((*bones)[bone_id]) ;
        return bd;
    }

	virtual	const IBoneData&_BCL	GetBoneData(u16 bone_id) const
	{
		VERIFY(bone_id<LL_BoneCount());
        VERIFY(bones);
        CBoneData& bd =  *((*bones)[bone_id]) ;
        return bd;
	}
	CBoneData*	_BCL	LL_GetBoneData		(u16 bone_id)
	{
		
		VERIFY(bone_id<LL_BoneCount());
        VERIFY(bones);
		u32	sz = sizeof(vecBones);
		u32	sz1=  sizeof(((*bones)[bone_id])->children);
		Msg("sz: %d",sz);
		Msg("sz1: %d",sz1);
        CBoneData* bd =  ((*bones)[bone_id]) ;
        return bd;
	}
	u16						_BCL	LL_BoneCount		()	const			{	return u16(bones->size());										}
	u16								LL_VisibleBoneCount	()					{	u64 F=visimask.flags&((u64(1)<<u64(LL_BoneCount()))-1); return (u16)btwCount1(F); }
	ICF Fmatrix&			_BCL	LL_GetTransform		(u16 bone_id)		{	return LL_GetBoneInstance(bone_id).mTransform;					}
	ICF const Fmatrix&		_BCL	LL_GetTransform		(u16 bone_id) const	{	return LL_GetBoneInstance(bone_id).mTransform;					}
	ICF Fmatrix&					LL_GetTransform_R	(u16 bone_id)		{	return LL_GetBoneInstance(bone_id).mRenderTransform;			}	// rendering only
	Fobb&							LL_GetBox			(u16 bone_id)		{	VERIFY(bone_id<LL_BoneCount());	return (*bones)[bone_id]->obb;	}
	const Fbox&				_BCL	GetBox				()const				{	return vis.box ;}
	void							LL_GetBindTransform (xr_vector<Fmatrix>& matrices);
    int 							LL_GetBoneGroups 	(xr_vector<xr_vector<u16> >& groups);

	u16						_BCL	LL_GetBoneRoot		()					{	return iRoot;													}
	void							LL_SetBoneRoot		(u16 bone_id)		{	VERIFY(bone_id<LL_BoneCount());	iRoot=bone_id;					}

    BOOL					_BCL	LL_GetBoneVisible	(u16 bone_id)		{	VERIFY(bone_id<LL_BoneCount()); return visimask.is(u64(1)<<bone_id);	}
	void							LL_SetBoneVisible	(u16 bone_id, BOOL val, BOOL bRecursive);
	u64						_BCL	LL_GetBonesVisible	()					{	return visimask.get();	}
	void							LL_SetBonesVisible	(u64 mask);

	// Main functionality
	virtual void					CalculateBones				(BOOL bForceExact	=	FALSE);		// Recalculate skeleton
	void							CalculateBones_Invalidate	();
	void							Callback					(UpdateCallback C, void* Param)		{	Update_Callback	= C; Update_Callback_Param	= Param;	}

	//	Callback: data manipulation
	virtual void					SetUpdateCallback(UpdateCallback pCallback) {Update_Callback = pCallback;}
	virtual void					SetUpdateCallbackParam(void* pCallbackParam) {Update_Callback_Param = pCallbackParam;}

	virtual UpdateCallback			GetUpdateCallback() { return Update_Callback;}
	virtual void*					GetUpdateCallbackParam() { return Update_Callback_Param;}

	// debug
#ifdef DEBUG
	void							DebugRender			(Fmatrix& XFORM);
protected:
	virtual shared_str		_BCL	getDebugName()	{ return dbg_name; }
public:
#endif

	// General "Visual" stuff
    virtual void					Copy				(dxRender_Visual *pFrom);
	virtual void					Load				(const char* N, IReader *data, u32 dwFlags);
	virtual void 					Spawn				();
	virtual void					Depart				();
    virtual void 					Release				();

	virtual	IKinematicsAnimated*dcast_PKinematicsAnimated() { return 0;	}
	virtual IRenderVisual*	_BCL dcast_RenderVisual() { return this; }
	virtual IKinematics*	_BCL dcast_PKinematics()  { return this; }
//	virtual	CKinematics*		dcast_PKinematics	()				{ return this;	}

	virtual u32					mem_usage			(bool bInstance)
	{
		u32 sz					= sizeof(*this);
		sz						+= bone_instances?bone_instances->mem_usage():0;
		if (!bInstance){
//			sz					+= pUserData?pUserData->mem_usage():0;
			for (vecBonesIt b_it=bones->begin(); b_it!=bones->end(); b_it++)
				sz				+= sizeof(vecBones::value_type)+(*b_it)->mem_usage();
		}
		return sz;
	}
private:
	bool						m_is_original_lod;

};
IC CKinematics* PCKinematics		(dxRender_Visual* V)		{ return V?(CKinematics*)V->dcast_PKinematics():0; }
//---------------------------------------------------------------------------
#endif
