//---------------------------------------------------------------------------
#ifndef ESceneShapeToolsH
#define ESceneShapeToolsH

#include "ESceneCustomOTools.h"

class ESceneShapeTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
						ESceneShapeTool			():ESceneCustomOTool(OBJCLASS_SHAPE){;}
	// definition
    IC LPCSTR			ClassName				(){return "shape";}
    IC LPCSTR			ClassDesc				(){return "Shape";}
    IC int				RenderPriority			(){return 20;}

    virtual void		Clear					(bool bSpecific=false){inherited::Clear(bSpecific);}
    // IO
    virtual bool   		IsNeedSave				(){return inherited::IsNeedSave();}
    virtual bool   		LoadStream            		(IReader&);
    virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
	virtual void   		SaveLTX            		(CInifile&, int id);
    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name);
    		void		OnEditLevelBounds		(bool recalc);

    virtual void    	OnActivate  			();
    virtual void    	OnDeactivate			();
};
//---------------------------------------------------------------------------
#endif
