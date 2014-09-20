//---------------------------------------------------------------------------
#ifndef EScenePortalToolsH
#define EScenePortalToolsH

#include "ESceneCustomOTools.h"

class EScenePortalTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
    friend class 		CPortal;
protected:
	enum{
    	flDrawSimpleModel	= (1<<31),
    };
    Flags32				m_Flags;
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
						EScenePortalTool		():ESceneCustomOTool(OBJCLASS_PORTAL){m_Flags.zero();}
	// definition
    IC LPCSTR			ClassName				(){return "portal";}
    IC LPCSTR			ClassDesc				(){return "Portal";}
    IC int				RenderPriority			(){return 20;}

	virtual void 		FillProp				(LPCSTR pref, PropItemVec& items);

    virtual void		Clear					(bool bSpecific=false){inherited::Clear(bSpecific);m_Flags.zero();}
    // IO
    virtual bool   		IsNeedSave				(){return true;}
    virtual bool   		LoadStream            		(IReader&);
    virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
	virtual void   		SaveLTX            		(CInifile&, int id);
    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);
    void 				RemoveSimilar			();
    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name);
};
//---------------------------------------------------------------------------
#endif
