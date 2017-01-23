//----------------------------------------------------
#ifndef SceneObjectH
#define SceneObjectH

#include "EditObject.h"
#include "customobject.h"
//----------------------------------------------------

class CSceneObject : public CCustomObject {
	shared_str		m_ReferenceName;
	CEditableObject*m_pReference;
    int				m_Version;
	void 			ReferenceChange			(PropValue* sender);
public:
	enum{
//    	flDynamic	= (1<<0),
		flFORCE32	= u32(-1)
    };
private:
	Fbox			m_TBBox;
	// options
    Flags32			m_Flags;
public:
    virtual void 	SetScale				(const Fvector& scale)
    {
    	if (m_pReference&&m_pReference->IsDynamic()){
        	ELog.Msg(mtError,"Dynamic object %s - can't scale.", Name);
        }else{
			FScale.set(scale);
            UpdateTransform();
        }
    }
protected:
	typedef CCustomObject inherited;
    int				m_iBlinkTime;
    CSurface*		m_BlinkSurf;
	void 			RenderBlink				();
public:
    // constructor/destructor methods
					CSceneObject			(LPVOID data, LPCSTR name);
	virtual 		~CSceneObject			();

	virtual void 	Select					(BOOL flag);
	void 			Construct				(LPVOID data);

    IC bool			CheckVersion			()  {return m_pReference?(m_Version==m_pReference->Version()):false;}
    // get object properties methods
	IC bool 		RefCompare				(CEditableObject *to){return m_pReference?!!(m_pReference==to):false; }
	IC bool 		RefCompare				(LPCSTR ref){return ref&&m_pReference?(strcmp(ref,m_pReference->GetName())==0):false; }
	IC CEditableObject*	GetReference		()	{return m_pReference; }
	CEditableObject*SetReference			(LPCSTR ref_name);
	CEditableObject*UpdateReference			();
	IC EditMeshVec* Meshes					() {return m_pReference?&m_pReference->Meshes():0;}
    virtual LPCSTR	RefName					() {return m_pReference?m_pReference->GetName():0;}
    virtual bool	CanAttach				() {return true;}

    // statistics methods
	IC bool 		IsDynamic     			()	{return (m_pReference?m_pReference->IsDynamic():false); }
	IC bool 		IsStatic     			()	{return (m_pReference?m_pReference->IsStatic():false); }
	IC bool 		IsMUStatic     			()	{return (m_pReference?m_pReference->IsMUStatic():false); }
    int 			GetFaceCount			();
	int 			GetVertexCount			();
    int 			GetSurfFaceCount		(const char* surf_name);

    // render methods
	virtual bool 	IsRender				();
	virtual void 	Render					(int priority, bool strictB2F);
	void 			RenderSelection			(u32 color=0x40E64646);
	void 			RenderEdge				(CEditableMesh* m=0, u32 color=0xFFC0C0C0);
	void 			RenderBones				();
	void 			RenderSingle			();

    // update methods
	virtual void 	OnFrame					();
    virtual void	OnUpdateTransform		();

    // misc
	void		    EvictObject				();

    // pick methods
    bool 			BoxPick					(const Fbox& box, SBoxPickInfoVec& pinf);
	virtual bool 	RayPick					(float& dist, const Fvector& S, const Fvector& D, SRayPickInfo* pinf=0);
	virtual void 	RayQuery				(SPickQuery& pinf);
	virtual void 	BoxQuery				(SPickQuery& pinf);
	virtual bool 	FrustumPick				(const CFrustum& frustum);
    virtual bool 	SpherePick				(const Fvector& center, float radius);

    // get orintation/bounding volume methods
	virtual bool 	GetBox					(Fbox& box);
	virtual bool 	GetUTBox				(Fbox& box);
    void 			GetFullTransformToWorld	(Fmatrix& m);
    void 			GetFullTransformToLocal	(Fmatrix& m);

	// editor integration
	virtual void	FillProp				(LPCSTR pref, PropItemVec& values);
	virtual bool 	GetSummaryInfo			(SSceneSummary* inf);

    // load/save methods
  	virtual bool 	Load					(IReader&);
	virtual void 	Save					(IWriter&);

    virtual void 	OnShowHint				(AStringVec& dest);

    void			Blink					(CSurface* surf=0);

    virtual bool	Validate				(bool bMsg);
};
//----------------------------------------------------
#endif /*_INCDEF_EditObject_H_*/


