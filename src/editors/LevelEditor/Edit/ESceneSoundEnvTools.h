//---------------------------------------------------------------------------
#ifndef ESceneSoundEnvToolsH
#define ESceneSoundEnvToolsH

#include "ESceneCustomOTools.h"

class ESceneSoundEnvTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
						ESceneSoundEnvTool		():ESceneCustomOTool(OBJCLASS_SOUND_ENV){;}
	// definition
    IC LPCSTR			ClassName				(){return "sound_env";}
    IC LPCSTR			ClassDesc				(){return "Sound Environment";}
    IC int				RenderPriority			(){return 10;}

    virtual void		Clear					(bool bSpecific=false);
    // IO
    virtual bool   		IsNeedSave				(){return inherited::IsNeedSave();}
    virtual bool   		LoadStream            	(IReader&);
	virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            	(IWriter&);
    virtual void   		SaveLTX            		(CInifile&, int id);
    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name);
};
//---------------------------------------------------------------------------
#endif
