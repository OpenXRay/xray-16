//---------------------------------------------------------------------------

#ifndef ESceneWallmarkToolsH
#define ESceneWallmarkToolsH

#include "ESceneCustomMTools.H"

class ESceneWallmarkTool: public ESceneToolBase
{
	typedef ESceneToolBase inherited;
public:    
	DEFINE_VECTOR		(FVF::LIT,LITVertVec,LITVertVecIt);
    struct wm_slot;
	struct wallmark 
	{
    	enum{
        	flSelected	= (1<<0)
        };
        wm_slot*		parent;
        float			w,h,r;
        Flags8			flags;
        Fbox			bbox;
		Fsphere			bounds;
		LITVertVec		verts;
        wallmark		(){flags.zero();parent=0;w=0;h=0;r=0;}
	};
	DEFINE_VECTOR		(wallmark*,WMVec,WMVecIt);
	struct wm_slot
	{
		shared_str		sh_name;
		shared_str		tx_name;
		ref_shader		shader;
		WMVec			items;
						wm_slot	(shared_str sh, shared_str tx)		{sh_name=sh;tx_name=tx;shader.create(*sh_name,*tx_name);items.reserve(256);}
	};
	DEFINE_VECTOR		(wm_slot*,WMSVec,WMSVecIt);
	WMSVec				marks;
	WMVec				pool;
private:
	ref_geom			hGeom;

	Fvector				sml_normal;
	CFrustum			sml_clipper;
	sPoly				sml_poly_dest;
	sPoly				sml_poly_src;

	CDB::Collector 		sml_collector;
	xr_vector<u32>		sml_adjacency;

	wm_slot*			FindSlot				(shared_str sh_name, shared_str tx_name);
	wm_slot*			AppendSlot				(shared_str sh_name, shared_str tx_name);

    void 				RecurseTri				(u32 t, Fmatrix &mView, wallmark &W);
	void 				BuildMatrix				(Fmatrix &mView, float inv_w, float inv_h, float angle, const Fvector& from);
	BOOL 				AddWallmark_internal	(const Fvector& S, const Fvector& D, shared_str s, shared_str t, float w, float h, float r);

	void				RefiningSlots			();
private:
	wallmark*			wm_allocate				();
	void				wm_destroy				(wallmark*	W);
private:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
	enum{
    	flDrawWallmark	= (1<<0),
        flAxisAlign 	= (1<<1)
    };
    Flags32				m_Flags;

    float				m_MarkWidth;
    float				m_MarkHeight;
    float				m_MarkRotate;
    shared_str			m_ShName;
    shared_str			m_TxName;

    int					ObjectCount				();
public:
						ESceneWallmarkTool    	();
	virtual        	 	~ESceneWallmarkTool 	();

	virtual	bool		AllowEnabling    		(){return true;}
    virtual BOOL 		AllowMouseStart			(){return true;}

    virtual void		OnObjectRemove			(CCustomObject* O, bool bDeleting){}
    virtual	void		UpdateSnapList			(){}
	virtual ObjectList*	GetSnapList				(){return 0;}

	// selection manipulate
	virtual int			RaySelect				(int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly);
	virtual int	   		FrustumSelect			(int flag, const CFrustum& frustum);
	virtual void   		SelectObjects           (bool flag);
	virtual void   		InvertSelection         ();
	virtual void		RemoveSelection         ();
	virtual int    		SelectionCount          (bool testflag);
	virtual void		ShowObjects				(bool flag, bool bAllowSelectionFlag=false, bool bSelFlag=true){}

    virtual void		Clear					(bool bOnlyNodes=false); 

	// definition
    IC LPCSTR			ClassName				(){return "wallmark";}
    IC LPCSTR			ClassDesc				(){return "Wallmark";}
    IC int				RenderPriority			(){return 10;}

    // validation
    virtual bool   		Valid					();
    virtual bool		Validate				(bool full_test);
    virtual bool   		IsNeedSave				();

    // events
	virtual void   		OnFrame					();
    virtual void   		OnRender				(int priority, bool strictB2F);

    // IO
    virtual bool   		LoadStream            		(IReader&);
    virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
    virtual void   		SaveLTX            		(CInifile&, int id);
    virtual bool		can_use_inifile			() {return false;}
    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);
    virtual bool   		Export          		(LPCSTR fn);
	virtual bool 		ExportStatic			(SceneBuilder* B, bool b_selected_only);
	virtual void 		GetStaticDesc			(int& v_cnt, int& f_cnt, bool b_selected_only, bool b_cform);

	// device dependent funcs    
	virtual void		OnDeviceCreate			();
	virtual void		OnDeviceDestroy			();

	virtual void		OnSynchronize			();

	virtual bool 		GetSummaryInfo			(SSceneSummary* inf){return false;} 

	// properties
    virtual void		FillProp          		(LPCSTR pref, PropItemVec& items);

    // utils
    virtual void		GetBBox 				(Fbox& bb, bool bSelOnly);
	BOOL 				AddWallmark				(const Fvector& start, const Fvector& dir);
	BOOL 				MoveSelectedWallmarkTo	(const Fvector& start, const Fvector& dir);
};
#endif
