//---------------------------------------------------------------------------

#ifndef EShapeH
#define EShapeH

#include "ShapeData.h"
#include "CustomObject.h"
//---------------------------------------------------------------------------
enum eShapeUsage{eShapeCommon=0, eShapeLevelBound};

class CEditShape: public CCustomObject, CShapeData
{
	typedef CCustomObject inherited;
private:
// bounds
	Fbox			m_Box;
	Fsphere			m_Sphere;
	void			ComputeBounds	( );
public:
	u8				m_shape_type;
	u32				m_DrawTranspColor;
	u32				m_DrawEdgeColor;

	void			SetDrawColor	(u32 transp, u32 edge){m_DrawTranspColor=transp;m_DrawEdgeColor=edge;}
	void			ApplyScale		();
	void			add_sphere		(const Fsphere& S);
	void			add_box			(const Fmatrix& B);
    const shape_def&get_shape		(int idx){R_ASSERT(idx<(int)shapes.size());return shapes[idx];}
	virtual void	FillProp		(LPCSTR pref, PropItemVec& values);
    
protected:
	virtual void 	SetScale		(const Fvector& val);
    virtual void	OnUpdateTransform();
public:
					CEditShape 		(LPVOID data, LPCSTR name);
	void 			Construct		(LPVOID data);
	virtual 		~CEditShape		();
    virtual bool	CanAttach		() {return true;}
    
    // pick functions
	virtual bool 	RayPick		(float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf = NULL);
    virtual bool 	FrustumPick	(const CFrustum& frustum);

    // placement functions
	virtual bool 	GetBox		(Fbox& box) const;

    // change position/orientation methods
//	virtual void 	Scale		(Fvector& amount){;}

    // file system function
  	virtual bool 		LoadStream			(IReader&);
  	virtual bool 		LoadLTX				(CInifile& ini, LPCSTR sect_name);
	virtual void 		SaveStream			(IWriter&);
  	virtual void 		SaveLTX				(CInifile& ini, LPCSTR sect_name);

    // render utility function
	virtual void 	Render		(int priority, bool strictB2F);
	virtual void 	OnFrame		();

    // tools
    void			Attach		(CEditShape* from);
    void			Detach		();

    ShapeVec&		GetShapes	(){return shapes;}


    virtual void	OnDetach	();
    // events
    virtual void    OnShowHint  (AStringVec& dest);
};

#endif
