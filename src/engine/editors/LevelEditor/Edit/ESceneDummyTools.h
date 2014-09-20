//---------------------------------------------------------------------------
#ifndef ESceneDummyToolsH
#define ESceneDummyToolsH

#include "ESceneCustomOTools.h"

class ESceneDummyTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
						ESceneDummyTool			():ESceneCustomOTool(OBJCLASS_DUMMY){;}
                        
    virtual void		Clear					(bool bSpecific=false){inherited::Clear(bSpecific);}
	// definition
    IC LPCSTR			ClassName				(){THROW; return 0;}
    IC LPCSTR			ClassDesc				(){THROW; return 0;}
    IC int				RenderPriority			(){return -1; }

    virtual bool   		IsNeedSave				(){return false;}

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name){return 0;};
};
//---------------------------------------------------------------------------
#endif
