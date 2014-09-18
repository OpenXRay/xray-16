//----------------------------------------------------
// file: WayPoint.h
//----------------------------------------------------

#ifndef WayPointH
#define WayPointH

#include "../../xrServerEntities/LevelGameDef.h"
#include "CustomObject.h"

class CFrustum;
class CWayPoint;

struct SWPLink{          
	CWayPoint*	way_point;
    float		probability;
//			    SWPLink():way_point(0),probability(0){}
			    SWPLink(CWayPoint* wp, float pb):way_point(wp),probability(pb){}
};
DEFINE_VECTOR(SWPLink*,WPLVec,WPLIt);

class CWayPoint{
    friend class CWayObject;
    friend class TfrmPropertiesWayPoint;
    shared_str		m_Name;
    Fvector		m_vPosition;
    Flags32		m_Flags;
    BOOL		m_bSelected;
    WPLVec		m_Links;
    void		CreateLink		(CWayPoint* P, float pb);
    bool		AppendLink		(CWayPoint* P, float pb);
    bool		DeleteLink		(CWayPoint* P);
public:
                CWayPoint		(LPCSTR name);
                ~CWayPoint		();
    void    	Render      	(LPCSTR parent_name, bool bParentSelect);
    bool    	RayPick	    	(float& distance, const Fvector& S, const Fvector& D);
    bool 		FrustumPick		(const CFrustum& frustum);
    bool 		FrustumSelect	(int flag, const CFrustum& frustum);
    void		Select			(int flag);
    void 		MoveTo			(const Fvector& pos){m_vPosition.set(pos);}
    bool		AddSingleLink	(CWayPoint* P);
    bool		AddDoubleLink	(CWayPoint* P);
    bool		RemoveLink		(CWayPoint* P);
    void		InvertLink		(CWayPoint* P);
    void		Convert1Link	(CWayPoint* P);
    void		Convert2Link	(CWayPoint* P);
    WPLIt		FindLink		(CWayPoint* P);
    void		GetBox			(Fbox& bb);
};

DEFINE_VECTOR(CWayPoint*,WPVec,WPIt);

class CWayObject: public CCustomObject
{
protected:
    friend class TfrmPropertiesWayPoint;
	EWayType		m_Type;
    WPVec			m_WayPoints;
    typedef CCustomObject inherited;
    CWayPoint*		FindWayPoint	(const shared_str& nm);
	void __stdcall  FindWPByName	(LPCSTR new_name,bool& res){res=!!FindWayPoint(new_name);}
	bool __stdcall 	OnWayPointNameAfterEdit(PropValue* sender, shared_str& edit_val);
public:
					CWayObject		(LPVOID data, LPCSTR name);
    void            Construct   	(LPVOID data);
	virtual			~CWayObject		();
    void			Clear			();
    virtual bool	CanAttach		() {return true;}

    EWayType		GetType			(){return m_Type;}

	virtual void 	Select			(int flag);
	virtual bool 	RaySelect		(int flag, const Fvector& start, const Fvector& dir, bool bRayTest=false); // flag 1,0,-1 (-1 invert)
    virtual bool 	FrustumSelect	(int flag, const CFrustum& frustum);

    CWayPoint*		AppendWayPoint	();
    CWayPoint*		GetFirstSelected();
    int 			GetSelectedPoints(WPVec& lst);
    void			RemoveSelectedPoints();
    void			RemoveLink		();
    void			InvertLink		();
    void			Convert1Link	();
    void			Convert2Link	();
    bool			Add1Link		();
    bool			Add2Link		();
    // change position/orientation methods
	virtual void 	MoveTo			(const Fvector& pos, const Fvector& up);
	virtual void 	Move			(Fvector& amount);
	virtual void 	RotateParent	(Fvector& axis, float angle ){;}
	virtual void 	RotateLocal		(Fvector& axis, float angle ){;}
	virtual void 	RotatePivot		(const Fmatrix& prev_inv, const Fmatrix& current){;}
	virtual void 	Scale			(Fvector& amount){;}
	virtual void 	ScalePivot		(const Fmatrix& prev_inv, const Fmatrix& current, Fvector& amount){;}

	virtual bool    GetBox      	(Fbox& box) const;
	virtual void    Render      	(int priority, bool strictB2F);
	virtual bool    RayPick	    	(float& distance, const Fvector& S, const Fvector& D, SRayPickInfo* pinf = NULL);
    virtual bool 	FrustumPick		(const CFrustum& frustum);

  	virtual bool 	LoadStream		(IReader&);
  	virtual bool 	LoadLTX			(CInifile& ini, LPCSTR sect_name);
	virtual void 	SaveStream		(IWriter&);
  	virtual void 	SaveLTX			(CInifile& ini, LPCSTR sect_name);

    virtual bool	ExportGame		(SExportStreams* data);

	virtual void	FillProp		(LPCSTR pref, PropItemVec& items);

    virtual bool 	OnSelectionRemove();

    virtual const Fvector& GetPosition	()	const { return m_WayPoints.front()->m_vPosition; 	}
    virtual void 	SetPosition		(const Fvector& pos)	{ MoveTo(pos, Fvector().set(0,1,0) );	UpdateTransform();}
};
#endif /*_INCDEF_WayPoint_H_*/

