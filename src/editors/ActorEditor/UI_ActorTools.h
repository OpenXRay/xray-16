//---------------------------------------------------------------------------
#ifndef UI_ActorToolsH
#define UI_ActorToolsH

#include "eltree.hpp"
#include "mxplacemnt.hpp"
#include "../xrEProps/ItemListHelper.h"
#include "../Ecore/editor/EditObject.h"
#include "../../Include/xrRender/KinematicsAnimated.h"
#include "../Ecore/editor/UI_ToolsCustom.h"
#include "../Ecore/editor/UI_MainCommand.h"
#include "../xrEProps/PropertiesList.h"
//#include "../../xrphysics/physicsshell.h"
#include "../Ecore/editor/physicsshellholdereditorbase.h"
// refs
class TProperties;
class CEditableObject;
class PropValue;
class TfrmKeyBar;
class CBlend;
class CMotionDef;
class CMotion;

enum EEditMode{
    emObject,
    emMotion,
    emBone,
    emSurface,
    emMesh
};

// refs
class CSMotion;
static Fmatrix xform  = Fidentity;
class EngineModel :
public CPhysicsShellHolderEditorBase
{
    CMemoryWriter	m_GeometryStream;
    CMemoryWriter	m_MotionKeysStream;
    CMemoryWriter	m_MotionDefsStream;
    bool			UpdateGeometryStream	(CEditableObject* source);
    bool			UpdateMotionKeysStream	(CEditableObject* source);
    bool			UpdateMotionDefsStream	(CEditableObject* source);
public:
    float			m_fLOD;
    IRenderVisual*	m_pVisual;

    //Fmatrix			m_pobject_matrix;
    CBlend*			m_pBlend;
    struct BPPlayItem{
        AnsiString	name;
        u16			slot;
    };
    BPPlayItem		m_BPPlayItems[4];
public:
                    EngineModel			(){ m_pVisual = 0; m_fLOD=1.f; m_pBlend=0;  /* m_pobject_matrix = Fidentity;*/ }

    void			DeleteVisual		();

    void			Clear				()
    {
        DeletePhysicsShell		();
        DeleteVisual			();
        m_GeometryStream.clear	();
        m_MotionKeysStream.clear();
        m_MotionDefsStream.clear();
    }
    bool 			UpdateVisual		(CEditableObject* source, bool bUpdGeom, bool bUpdKeys, bool bUpdDefs);
    bool			IsRenderable		(){return !!m_pVisual;}
    void			PlayMotion			(LPCSTR name, u16 slot);
    void			RestoreParams		(TFormStorage* s);
    void			SaveParams			(TFormStorage* s);
    void			OnRender			();
//.    void			PlayCycle			(LPCSTR name, int part, u16 slot);
//.    void			PlayFX				(LPCSTR name, float power, u16 slot);
    void			StopAnimation		();
    void 			FillMotionList		(LPCSTR pref, ListItemsVec& items, int modeID);
    MotionID		FindMotionID		(LPCSTR name, u16 slot);
    CMotionDef*		FindMotionDef		(LPCSTR name, u16 slot);
    CMotion*		FindMotionKeys		(LPCSTR name, u16 slot);
private:
   //	virtual	Fmatrix&				   	_BCL	ObjectXFORM							()				 { return m_pobject_matrix; }
   //	virtual	Fvector&					_BCL	ObjectPosition						()				 { return m_pobject_matrix.c; }
	//virtual	IRenderVisual*				_BCL	ObjectVisual						()				 { return m_pVisual;}
    virtual	IKinematics*				_BCL	ObjectKinematics					()				 	{ if(!m_pVisual)return 0;return m_pVisual->dcast_PKinematics();}

};

class PreviewModel{
    TProperties*	m_Props;
    float			m_fSpeed;
    float			m_fSegment;
    Flags32			m_Flags;
    Fvector			m_vPosition;
    AnsiString 		m_LastObjectName;
public:
    CEditableObject*m_pObject;
public:
    enum{
        pmScroll	= (1<<0),
        force_dword = u32(-1)
    };
    enum EScrollAxis{
        saZp,
        saZn,
        saXp,
        saXn,
        saforceDWORD=u32(-1)
    };
    EScrollAxis		m_ScrollAxis;
public:
                    PreviewModel		(){m_pObject=0;m_fSpeed=5.f;m_fSegment=50.f;m_Flags.zero();m_Props=0;m_vPosition.set(0,0,0);m_ScrollAxis=saZp;}
    void			OnCreate			();
    void			OnDestroy			();
    void			Clear				();
    void			SelectObject		();
    void			SetPreferences		();
    void			Render				();
    void			Update				();
    void			RestoreParams		(TFormStorage* s);
    void			SaveParams			(TFormStorage* s);
};

class CActorTools: public CToolCustom
{
	typedef CToolCustom inherited;

    Fmatrix				m_AVTransform;
	CEditableObject*	m_pEditObject;

    bool				m_bObjectModified;
    shared_str			m_tmp_mot_refs;
    EEditMode			m_EditMode;
    AnsiString			m_CurrentMotion;
    u16					m_CurrentSlot;

    RTokenVec			m_BoneParts;

	void __stdcall 		OnMotionTypeChange		(PropValue* sender);

	void __stdcall  	OnChangeTransform		(PropValue* sender);
	void __stdcall  	OnMotionNameChange		(PropValue* sender);

	void  __stdcall  	OnBoxAxisClick			(ButtonValue* sender, bool& bModif, bool& bSafe);
	void  __stdcall  	OnCylinderAxisClick		(ButtonValue* sender, bool& bModif, bool& bSafe);
    
	void __stdcall  	OnMotionEditClick		(ButtonValue* sender, bool& bModif, bool& bSafe);
	void __stdcall  	OnMotionControlClick	(ButtonValue* sender, bool& bModif, bool& bSafe);
	void __stdcall  	OnExportImportRefsClick	(ButtonValue* sender, bool& bModif, bool& bSafe);

	void __stdcall  	OnMarksControlClick12		(ButtonValue* sender, bool& bModif, bool& bSafe);
	void __stdcall  	OnMarksControlClick34		(ButtonValue* sender, bool& bModif, bool& bSafe);

    void __stdcall  	OnObjectItemFocused		(ListItemsVec& items);

    void __stdcall 		OnBoneShapeClick  		(ButtonValue* sender, bool& bModif, bool& bSafe);
    void __stdcall 		OnBoneCreateDeleteClick (ButtonValue* sender, bool& bModif, bool& bSafe);
    void __stdcall 		OnBoneEditClick			(ButtonValue* sender, bool& bModif, bool& bSafe);
    void __stdcall 		OnBoneFileClick			(ButtonValue* sender, bool& bModif, bool& bSafe);
    void __stdcall 		OnBoneLimitsChange		(PropValue* sender);
    
	void __stdcall  	OnJointTypeChange		(PropValue* sender);
	void __stdcall  	OnShapeTypeChange		(PropValue* sender);

	void __stdcall  	OnMotionRefsChange		(PropValue* sender);
    
	void __stdcall  	OnBindTransformChange	(PropValue* V);

    SMotionVec 			appended_motions;
protected:
	// flags
    enum{
    	flRefreshProps 		= (1<<0),
    	flRefreshSubProps 	= (1<<1),
    	flRefreshShaders 	= (1<<2),
    	flUpdateGeometry 	= (1<<3),
    	flUpdateMotionKeys 	= (1<<4),
    	flUpdateMotionDefs	= (1<<5),
        flReadOnlyMode 		= (1<<6),
    };
    Flags32				m_Flags;
    
    void				RefreshSubProperties	(){m_Flags.set(flRefreshSubProps,TRUE);}
    void				RefreshShaders			(){m_Flags.set(flRefreshShaders,TRUE);}

    void __stdcall  	PMMotionItemClick		(TObject *Sender);
    
    void				RealUpdateProperties	();

    void				PrepareLighting			();
public:
	EngineModel			m_RenderObject;
    PreviewModel		m_PreviewObject;

    TProperties*		m_Props;
    TItemList*			m_ObjectItems;

    TfrmKeyBar* 		m_KeyBar;
// undo part
protected:
    #pragma pack( push,1 )
    struct UndoItem {
        char m_FileName[MAX_PATH];
    };
    #pragma pack( pop )
	xr_deque<UndoItem> 	m_UndoStack;
	xr_deque<UndoItem> 	m_RedoStack;
public:
	void 				UndoClear			();
	void 				UndoSave			();
	bool 				Undo				();
	bool 				Redo				();
public:
						CActorTools			();
    virtual 			~CActorTools		();

    virtual void		Render				();
	virtual void		RenderEnvironment	();
    virtual void		OnFrame				();

    virtual bool		OnCreate			();
    virtual void		OnDestroy			();

    ICF bool			ReadOnly			(){return m_Flags.is(flReadOnlyMode);}
    
    virtual bool		IfModified			();
    virtual bool		IsModified			(){return m_bObjectModified;}
    virtual void		Modified			(); 
    void __stdcall 		OnItemModified		(void); 

    virtual LPCSTR		GetInfo				();
    
    virtual void		ZoomObject			(BOOL bSelOnly);

    virtual bool		Load				(LPCSTR name);
    virtual bool		Save				(LPCSTR name, bool bInternal=false);
    virtual void		Reload				();
    
    virtual void		OnDeviceCreate		();
    virtual void		OnDeviceDestroy		();

    virtual void		Clear				();

    virtual void		OnShowHint			(AStringVec& SS);

    virtual bool __fastcall 	MouseStart  		(TShiftState Shift);
    virtual bool __fastcall 	MouseEnd    		(TShiftState Shift);
    virtual void __fastcall 	MouseMove   		(TShiftState Shift);

    virtual bool		Pick				(TShiftState Shift);
	virtual bool 		RayPick				(const Fvector& start, const Fvector& dir, float& dist, Fvector* pt, Fvector* n);

    virtual void		ShowProperties		(LPCSTR focused_item){;}
    virtual void		UpdateProperties	(BOOL bForced=FALSE){m_Flags.set(flRefreshProps,TRUE); if (bForced) RealUpdateProperties();}
    virtual void		RefreshProperties	(){;}
    
	void				GetStatTime			(float& a, float& b, float& c);

    bool				IsEngineMode		();
    void 				SelectPreviewObject	(bool bClear);
    void				SetPreviewObjectPrefs();

    void				SelectListItem		(LPCSTR pref, LPCSTR name, bool bVal, bool bLeaveSel, bool bExpand);

	void 				ShowClipMaker		();
    bool				Import				(LPCSTR path, LPCSTR name);
    bool				ExportOBJ			(LPCSTR name);
    bool				ExportOGF			(LPCSTR name);
    bool				ExportOMF			(LPCSTR name);
    bool				ExportDM			(LPCSTR name);
    bool 				ExportCPP			(LPCSTR name);
    bool				SaveMotions			(LPCSTR name, bool bSelOnly);
    bool				AppendMotion		(LPCSTR fn);
    bool				RemoveMotion		(LPCSTR name);
    void 				WorldMotionRotate	(const Fvector& R);
    void				MakePreview			();

	bool __stdcall  	OnBoneNameAfterEdit	(PropValue* sender, shared_str& edit_val);
    void __stdcall 		OnBoneModified		(void);
    void __stdcall 		OnObjectModified	(void);
    void __stdcall 		OnMotionDefsModified(void); 
    void 				OnMotionKeysModified(void); 
	void 				OnGeometryModified	(void);

    bool				IsVisualPresent		(){return m_RenderObject.IsRenderable();}

    CEditableObject*	CurrentObject		(){return m_pEditObject;}
    void				SetCurrentMotion	(LPCSTR name, u16 slot);
    CSMotion*			GetCurrentMotion	();       
    CSMotion*			FindMotion			(LPCSTR name);
    LPCSTR 				ExtractMotionName	(LPCSTR full_name, LPCSTR prefix=MOTIONS_PREFIX);
    u16 				ExtractMotionSlot	(LPCSTR full_name, LPCSTR prefix=MOTIONS_PREFIX);
    xr_string			BuildMotionPref		(u16 slot, LPCSTR prefix=MOTIONS_PREFIX);
	void				FillObjectProperties(PropItemVec& items, LPCSTR pref, ListItem* sender);
	void				FillSurfaceProperties(PropItemVec& items, LPCSTR pref, ListItem* sender);
	void				FillMotionProperties(PropItemVec& items, LPCSTR pref, ListItem* sender);
    void				FillBoneProperties	(PropItemVec& items, LPCSTR pref, ListItem* sender);
    void				PlayMotion			();
    void				StopMotion			();
    void				PauseMotion			();
    bool				RenameMotion		(LPCSTR old_name, LPCSTR new_name);
	void 				RemoveMarksChannel	(bool b12);
	void 				AddMarksChannel		(bool b12);
    void				OptimizeMotions		();
    void				MakeThumbnail		();
    bool				BatchConvert		(LPCSTR fn);


    void				PhysicsSimulate			( );
    void 				PhysicsStopSimulate		( );
    void				CreatePhysicsWorld		( );
    void				DestroyPhysicsWorld		( );

    virtual bool		GetSelectionPosition	(Fmatrix& result);




    // commands
	CCommandVar			CommandClear		(CCommandVar p1, CCommandVar p2);
	CCommandVar 		CommandLoad			(CCommandVar p1, CCommandVar p2);
    CCommandVar         CommandSaveBackup	(CCommandVar p1, CCommandVar p2);
    CCommandVar         CommandSave			(CCommandVar p1, CCommandVar p2);
    CCommandVar         CommandImport		(CCommandVar p1, CCommandVar p2);
    CCommandVar         CommandExportDM		(CCommandVar p1, CCommandVar p2);
    CCommandVar         CommandExportOBJ	(CCommandVar p1, CCommandVar p2);
    CCommandVar         CommandExportOGF	(CCommandVar p1, CCommandVar p2);
    CCommandVar         CommandExportOMF	(CCommandVar p1, CCommandVar p2);
    CCommandVar 		CommandExportCPP	(CCommandVar p1, CCommandVar p2);
	CCommandVar 		CommandUndo			(CCommandVar p1, CCommandVar p2);
	CCommandVar 		CommandRedo			(CCommandVar p1, CCommandVar p2);
	CCommandVar 		CommandOptimizeMotions(CCommandVar p1, CCommandVar p2);
    CCommandVar 		CommandMakeThumbnail(CCommandVar p1, CCommandVar p2);
    CCommandVar			CommandBatchConvert	(CCommandVar p1, CCommandVar p2);
};

extern CActorTools*&	ATools;

#define SURFACES_PREFIX "Surfaces"
#define BONES_PREFIX 	"Bones"
#define MOTIONS_PREFIX 	"Motions"
#define OBJECT_PREFIX 	"Object"
//---------------------------------------------------------------------------
#endif

