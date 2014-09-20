//---------------------------------------------------------------------------
#ifndef ESceneGlowToolsH
#define ESceneGlowToolsH

#include "ESceneCustomOTools.h"

class ESceneGlowTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
    friend class 		CGlow;
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
	enum{
    	flDrawCross		= (1<<30),
    	flTestVisibility= (1<<31),
    };
    Flags32				m_Flags;
public:
						ESceneGlowTool			():ESceneCustomOTool(OBJCLASS_GLOW){;}
	// definition
    IC LPCSTR			ClassName				(){return "glow";}
    IC LPCSTR			ClassDesc				(){return "Glow";}
    IC int				RenderPriority			(){return 20;}

    void 				FillProp				(LPCSTR pref, PropItemVec& items);

    virtual void		Clear					(bool bSpecific=false){inherited::Clear(bSpecific); m_Flags.zero();};
    // IO
    virtual bool   		IsNeedSave				(){return true;}
    virtual bool   		LoadStream            		(IReader&);
    virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
	virtual void   		SaveLTX            		(CInifile&, int id);
    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name);
};
//---------------------------------------------------------------------------
#endif
