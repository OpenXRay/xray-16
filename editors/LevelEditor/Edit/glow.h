//----------------------------------------------------
// file: Glow.h
//----------------------------------------------------

#ifndef GlowH
#define GlowH

#include "../ECore/Editor/tlsprite.h"
#include "customobject.h"
class CFrustum;

class CGlow : public CCustomObject
{
	typedef CCustomObject inherited ;
    CTLSprite       m_RenderSprite;
public:
	enum EFlags{
    	gfFixedSize	= (1<<0),
    	gfForce_u8	= u8(-1)
    };
    ref_shader     	m_GShader;
    shared_str		m_ShaderName;
    shared_str		m_TexName;
    float           m_fRadius;
    Flags8			m_Flags;

    bool			m_bDefLoad;
    void 			ShaderChange	(PropValue* value);
protected:
    virtual const Fvector& GetScale		() const	{ FScale.set(m_fRadius,m_fRadius,m_fRadius); return FScale;	}
	virtual void 	SetScale		(const Fvector& sc){float v=m_fRadius; if (!fsimilar(FScale.x,sc.x)) v=sc.x; if (!fsimilar(FScale.y,sc.y)) v=sc.y; if (!fsimilar(FScale.z,sc.z)) v=sc.z; FScale.set(v,v,v); m_fRadius=v; UpdateTransform();}
public:
	                CGlow       (LPVOID data, LPCSTR name);
    void            Construct   (LPVOID data);
	virtual         ~CGlow      ();
    virtual bool	CanAttach	() {return true;}

    void			Compile		();

	virtual void    Render      (int priority, bool strictB2F);
	virtual bool    RayPick     ( float& distance,	const Fvector& start,	const Fvector& direction,
		                          SRayPickInfo* pinf = NULL );
    virtual bool 	FrustumPick	( const CFrustum& frustum );

  	virtual bool 		LoadStream			(IReader&);
  	virtual bool 		LoadLTX				(CInifile& ini, LPCSTR sect_name);
	virtual void 		SaveStream			(IWriter&);
  	virtual void 		SaveLTX				(CInifile& ini, LPCSTR sect_name);

	virtual bool    GetBox      ( Fbox& box ) const;
    virtual void	FillProp	( LPCSTR pref, PropItemVec& values );

    // device dependent routine
	virtual void 	OnDeviceCreate 	();
	virtual void 	OnDeviceDestroy	();

	virtual bool 	GetSummaryInfo(SSceneSummary* inf);
};

#endif /*_INCDEF_Glow_H_*/

