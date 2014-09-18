//---------------------------------------------------------------------------
#ifndef EScenePSToolsH
#define EScenePSToolsH

#include "ESceneCustomOTools.h"

class EScenePSTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
						EScenePSTool			():ESceneCustomOTool(OBJCLASS_PS){;}
	// definition
    IC LPCSTR			ClassName				(){return "ps";}
    IC LPCSTR			ClassDesc				(){return "Particle System";}
    IC int				RenderPriority			(){return 30;}

    virtual void		Clear					(bool bSpecific=false){inherited::Clear(bSpecific);}
    // IO
    virtual bool   		IsNeedSave				(){return inherited::IsNeedSave();}
    virtual bool   		LoadStream            		(IReader&);
    virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
	virtual void   		SaveLTX            		(CInifile&, int id);
    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);
    virtual bool		ExportGame         		(SExportStreams* F);

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name);
};
//---------------------------------------------------------------------------
#endif
