#ifndef ESceneCustomOToolsH
#define ESceneCustomOToolsH

#include "ESceneCustomMTools.h"

struct SRayPickInfo;
struct mesh_build_data;
class ESceneCustomOTool: public ESceneToolBase
{
	typedef ESceneToolBase inherited;
protected:
	ObjectList			m_Objects;
    
	bool 				OnLoadSelectionAppendObject(CCustomObject* obj);
	bool 				OnLoadAppendObject		(CCustomObject* obj);
public:
						ESceneCustomOTool		(ObjClassID cls);
	virtual				~ESceneCustomOTool		();
                                               
	virtual	bool		AllowEnabling    		(){return true;}
    virtual BOOL 		AllowMouseStart			();

    // snap 
	virtual ObjectList* GetSnapList				();
    virtual void		UpdateSnapList			();
    
	// selection manipulate
	virtual int			RaySelect				(int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly);
	virtual int			FrustumSelect			(int flag, const CFrustum& frustum);
	virtual void 		SelectObjects           (bool flag);
	virtual void 		InvertSelection         ();
	virtual void		RemoveSelection         ();
	virtual int 		SelectionCount          (bool testflag);
	virtual void		ShowObjects				(bool flag, bool bAllowSelectionFlag=false, bool bSelFlag=true);

    virtual void		Clear					(bool bSpecific=false)=0;       

    // validation
    virtual bool		Valid					();
    virtual bool		Validate				(bool);

    // events
	virtual void		OnDeviceCreate			();
	virtual void		OnDeviceDestroy			();
	virtual void		OnSynchronize			();
	virtual void		OnSceneUpdate			();
    virtual void		OnObjectRemove			(CCustomObject* O, bool bDeleting);

    virtual void 		OnBeforeObjectChange	(CCustomObject* O){inherited::OnBeforeObjectChange(O);}
	virtual void		OnFrame					();
    virtual void		OnRender				(int priority, bool strictB2F);

    // IO
    virtual bool   		IsNeedSave				()=0;

    virtual bool   		LoadStream            		(IReader&);
    virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
    virtual void   		SaveLTX            		(CInifile&, int id);

    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);

    virtual bool		Export          		(LPCSTR path);
    virtual bool		ExportGame         		(SExportStreams* F);
    virtual bool		ExportStatic			(SceneBuilder* B, bool b_selected_only);
    virtual bool 		GetStaticCformData		( mesh_build_data &data, bool b_selected_only );
    // properties
    virtual void		FillProp          		(LPCSTR pref, PropItemVec& items);

    // utils
	virtual bool 		GetSummaryInfo			(SSceneSummary* inf);
    virtual void		GetBBox 				(Fbox& bb, bool bSelOnly);

    //--------------------------------------------------------------------------
    // object part
    //--------------------------------------------------------------------------
    IC ObjectList&		GetObjects				(){return m_Objects;}
    IC int				ObjCount				(){return m_Objects.size();}

	virtual BOOL 		_AppendObject			(CCustomObject* object);
	virtual BOOL 		_RemoveObject			(CCustomObject* object);

    // pick function
    virtual BOOL 		RayPick					(CCustomObject*& object, float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf);
    virtual BOOL 		FrustumPick				(ObjectList& lst, const CFrustum& frustum);
    virtual BOOL 		SpherePick				(ObjectList& lst, const Fvector& center, float radius);
    virtual int 		GetQueryObjects			(ObjectList& lst, int iSel, int iVis, int iLock);

    virtual CCustomObject* FindObjectByName		(LPCSTR name, CCustomObject* pass=0);

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name)=0;

    virtual int			MultiRenameObjects		();
	void				OnSelected				(CCustomObject* object);
	virtual const CCustomObject* LastSelected			() const;
};
#endif // ESceneCustomOToolsH

