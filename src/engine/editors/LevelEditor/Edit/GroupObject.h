#ifndef GroupObjectH
#define GroupObjectH

#include "CustomObject.h"
//----------------------------------------------------
class CGroupObject: public CCustomObject
{
    typedef CCustomObject inherited;

    struct SGroupObjectItem
    {
    	CCustomObject*	pObject;
        SGroupObjectItem():pObject(NULL){}

        bool operator == (const CCustomObject* obj){return obj==pObject;}
    };
    typedef xr_list<SGroupObjectItem> ObjectsInGroup;
    ObjectsInGroup		m_ObjectsInGroup;


    bool			LL_AppendObject		(CCustomObject* object);
    bool			AppendObjectLoadCB	(CCustomObject* object);
    shared_str		m_ReferenceName_;
	void 			ReferenceChange		(PropValue* sender);
	void __stdcall  OnFreezeAllClick	(ButtonValue* sender, bool& bModif, bool& bSafe);
    void			ClearInternal			(ObjectsInGroup& v);
public:
    void			Clear1				(){ClearInternal(m_ObjectsInGroup);};
	bool 			UpdateReference		(bool bForceReload);
public:
					CGroupObject	(LPVOID data, LPCSTR name);
	void 			Construct		(LPVOID data);
	virtual 		~CGroupObject	();
    bool			CanUngroup		(bool bMsg);
    void			GroupObjects	(ObjectList& lst);
    void			UngroupObjects	();
    u32				GetObjects		(ObjectList& lst);
    IC int			ObjectInGroupCount(){return m_ObjectsInGroup.size();}

    bool			SetReference	(LPCSTR nm);
	virtual LPCSTR  RefName			(){return m_ReferenceName_.size()?m_ReferenceName_.c_str():"group";}
	bool    		RefCompare		(LPCSTR nm){return m_ReferenceName_.equal(nm);}
	void 			SetRefName		(LPCSTR nm);
	void			UpdatePivot		(LPCSTR nm, bool center);
	virtual bool 	GetBox			(Fbox& box) const;
    virtual bool	CanAttach		() {return false;}

    virtual void 	NumSetPosition	(const Fvector& pos);
	virtual void 	NumSetRotation	(const Fvector& rot);
    virtual void 	NumSetScale		(const Fvector& scale);
	virtual void 	MoveTo			(const Fvector& pos, const Fvector& up);
	virtual void 	Move			(Fvector& amount);
	virtual void 	RotateParent	(Fvector& axis, float angle );
	virtual void 	RotateLocal		(Fvector& axis, float angle );
	virtual void 	Scale			(Fvector& amount );

	virtual void 	Render			(int priority, bool strictB2F);

	virtual bool 	RayPick			(float& dist, const Fvector& start,const Fvector& dir, SRayPickInfo* pinf=NULL);
    virtual bool 	FrustumPick		(const CFrustum& frustum);

    virtual void 	OnUpdateTransform();
	virtual void 	OnFrame			();
	virtual void 	Select			(int  flag);

  	virtual bool 		LoadStream			(IReader&);
  	virtual bool 		LoadLTX				(CInifile& ini, LPCSTR sect_name);
	virtual void 		SaveStream			(IWriter&);
  	virtual void 		SaveLTX				(CInifile& ini, LPCSTR sect_name);

    virtual bool	ExportGame		(SExportStreams* data);

    // device dependent routine
	virtual void 	OnDeviceCreate 	();
	virtual void 	OnDeviceDestroy	();

    virtual void    OnShowHint      (AStringVec& dest);
    virtual void 	OnObjectRemove	(const CCustomObject* object);
	virtual void 	OnSceneUpdate	();

	virtual void	FillProp		(LPCSTR pref, PropItemVec& items);
};
//----------------------------------------------------
#endif /* _INCDEF_GroupObject_H_ */
