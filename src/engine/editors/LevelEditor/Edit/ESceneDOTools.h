//----------------------------------------------------
#ifndef ESceneDOToolsH
#define ESceneDOToolsH

#include "..\..\Layers\xrRender\DetailFormat.h"
#include "../ECore/Editor/EDetailModel.h"
#include "../ECore/Editor/Library.h"
#include "customobject.h"
#include "..\..\Layers\xrRender\DetailManager.h"
#include "Custom2DProjector.h"
#include "ESceneCustomMTools.H"

class CFrustum;
class CEditableObject;
class Shader;

struct SIndexDist{
    u32 	index;
    float 	dist;
    float	dens[4];
    float	cnt[4];
	SIndexDist(){ZeroMemory(this,sizeof(SIndexDist));}
};
DEFINE_SVECTOR		(SIndexDist,4,SIndexDistVec,SIndexDistIt);

DEFINE_MAP			(u32,DOVec,ColorIndexMap,ColorIndexPairIt);

#define DETAIL_SLOT_SIZE_2 	DETAIL_SLOT_SIZE*0.5f
#define DETAIL_SLOT_RADIUS	DETAIL_SLOT_SIZE*0.7071f

class EDetailManager:
	public CDetailManager,
    public ESceneToolBase
//	,public pureDeviceCreate,
//	public pureDeviceDestroy
{
	friend class TfrmDOShuffle;
	typedef ESceneToolBase inherited;

	enum{
    	flBaseTextureDraw		= (1<<31),
        flBaseTextureBlended	= (1<<30),
        flSlotBoxesDraw			= (1<<29),
        flObjectsDraw			= (1<<28),
    };
    Flags32				m_Flags;

    enum{
    	flRTGenerateBaseMesh	= (1<<0)
    };
    Flags32				m_RTFlags;

    ObjectList			m_SnapObjects;

    Fbox				m_BBox;

	IC u32				toSlotX			(float x)	{return (x/DETAIL_SLOT_SIZE+0.5f)+dtH.offs_x;}
	IC u32				toSlotZ			(float z)	{return (z/DETAIL_SLOT_SIZE+0.5f)+dtH.offs_z;}
	IC float			fromSlotX		(int x)		{return (x-dtH.offs_x)*DETAIL_SLOT_SIZE+DETAIL_SLOT_SIZE_2;}
	IC float			fromSlotZ		(int z)		{return (z-dtH.offs_z)*DETAIL_SLOT_SIZE+DETAIL_SLOT_SIZE_2;}

    void				UpdateSlotBBox	(int x, int z, DetailSlot& slot);

    void				GetSlotRect		(Frect& rect, int sx, int sz);
    void				GetSlotTCRect	(Irect& rect, int sx, int sz);
    u8					GetRandomObject	(u32 color_index);
    u8					GetObject		(ColorIndexPairIt& CI, u8 id);

	void 				CalcClosestCount(int part, const Fcolor& C, SIndexDistVec& best);
	void 				FindClosestIndex(const Fcolor& C, SIndexDistVec& best);

    DetailSlot&			GetSlot			(u32 sx, u32 sz);

    void __stdcall 		OnDensityChange		(PropValue* prop);
    void __stdcall 		OnBaseTextureChange	(PropValue* prop);
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
// render part -----------------------------------------------------------------
    void 				InitRender				();
    void				RenderTexture			(float alpha);
    void				InvalidateCache			();
// render part -----------------------------------------------------------------
public:
    ColorIndexMap		m_ColorIndices;
	U8Vec				m_Selected;
    CCustom2DProjector	m_Base;

    void				SaveColorIndices		(IWriter&);
    bool				LoadColorIndices		(IReader&);
public:
						EDetailManager			();
    virtual 			~EDetailManager			();

	virtual	bool		AllowEnabling    		(){return true;}
    virtual BOOL 		AllowMouseStart			(){return true;}

    // snap 
	virtual ObjectList* GetSnapList				(){return &m_SnapObjects;}
    virtual void		UpdateSnapList			(){};

	// selection manipulate
	virtual int			RaySelect				(int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly);
	virtual int			FrustumSelect			(int flag, const CFrustum& frustum);
	virtual void		SelectObjects           (bool flag);
	virtual void		InvertSelection         ();
	virtual void		RemoveSelection         (){}
	virtual int			SelectionCount          (bool testflag);
	virtual void		ShowObjects				(bool flag, bool bAllowSelectionFlag=false, bool bSelFlag=true){}

    virtual void		Clear					(bool bSpecific=false);

	// definition
    IC LPCSTR			ClassName				(){return "detail_object";}
    IC LPCSTR			ClassDesc				(){return "Detail Objects";}
    IC int				RenderPriority			(){return 10;}

    // validation
    virtual bool		Valid					(){return dtSlots||objects.size()||m_Base.Valid()||m_SnapObjects.size();}
    virtual bool		Validate				(bool){return true;}

    // events
	virtual void		OnDeviceCreate			();
	virtual void		OnDeviceDestroy			();
	virtual void		OnSynchronize			();
    virtual void		OnObjectRemove			(CCustomObject* O, bool bDeleting);
	virtual void		OnSceneUpdate			();
	virtual void		OnFrame					();
    virtual void		OnRender				(int priority, bool strictB2F);

    // IO
    virtual bool   		IsNeedSave				()				{return Valid();}
    virtual bool		LoadStream         		(IReader&);
    virtual bool		LoadLTX            		(CInifile&);
    virtual void		SaveStream         		(IWriter&);
    virtual void		SaveLTX            		(CInifile&, int id);
    virtual bool		can_use_inifile			()				{return false;}


    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);
    virtual bool		Export          		(LPCSTR fn);

	// properties
    virtual void		FillProp          		(LPCSTR pref, PropItemVec& items);

    // utils
	virtual bool 		GetSummaryInfo			(SSceneSummary* inf);
    virtual void		GetBBox 				(Fbox& bb, bool bSelOnly){}
    
    // other
    bool				UpdateHeader			();
    bool				UpdateSlots  			();
    bool				UpdateSlotObjects		(int x, int z);
    bool				UpdateObjects			(bool bUpdateTex, bool bUpdateSelectedOnly);
    bool				Initialize				();
    bool				Reinitialize			();
    void				InvalidateSlots			();

    EDetail*			AppendDO				(LPCSTR name, bool bTestUnique=true);
    bool				RemoveDO				(LPCSTR name);
    int					RemoveDOs				();
    DetailIt			FindDOByNameIt			(LPCSTR name);
    EDetail*			FindDOByName			(LPCSTR name);

    void				RemoveColorIndices		();
	void				AppendIndexObject		(u32 color,LPCSTR name,bool bTestUnique=true);
    EDetail*			FindObjectInColorIndices(u32 index, LPCSTR name);
    void				ExportColorIndices		(LPCSTR fname);
    bool				ImportColorIndices		(LPCSTR fname);

    void				ClearColorIndices		();
    void				ClearSlots				();
    void				ClearBase				();

};
#endif /*_INCDEF_DetailObjects_H_*/

