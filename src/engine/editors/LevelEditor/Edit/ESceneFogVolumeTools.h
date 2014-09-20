//---------------------------------------------------------------------------
#ifndef ESceneFogVolumeToolsH
#define ESceneFogVolumeToolsH

#include "ESceneCustomOTools.h"
class EFogVolume;

enum{
	fvEmitter 	= 0,
    fvOcclusion	= 1,
};

class ESceneFogVolumeTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
protected:
	u32					m_group_counter;
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
						ESceneFogVolumeTool		():ESceneCustomOTool(OBJCLASS_FOG_VOL){;}
	// definition
    IC LPCSTR			ClassName				(){return "fog_volume";}
    IC LPCSTR			ClassDesc				(){return "Scene fog volumes";}
    IC int				RenderPriority			(){return 11;}

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

	void				GroupSelected			();
	void				UnGroupCurrent			();
    void				RegisterGroup			(u32 group);
    void				Selected				(EFogVolume* fv);
};

#include "CustomObject.h"
#include "EShape.h"

class EFogVolume: public CEditShape
{
	typedef CCustomObject inherited;
    void __stdcall 		OnChangeEnvs	(PropValue* prop);
public:
	u8					m_volumeType;
    u32					m_group_id;
    shared_str			m_volume_profile;

						EFogVolume		(LPVOID data, LPCSTR name);
	void 				Construct		(LPVOID data);
	virtual				~EFogVolume		();
    virtual bool		CanAttach		() {return true;}
    virtual void		OnUpdateTransform();

  	virtual bool 		LoadStream		(IReader&);
  	virtual bool 		LoadLTX			(CInifile& ini, LPCSTR sect_name);
	virtual void 		SaveStream		(IWriter&);
  	virtual void 		SaveLTX			(CInifile& ini, LPCSTR sect_name);

	virtual void		FillProp		(LPCSTR pref, PropItemVec& values);
	virtual bool 		GetSummaryInfo	(SSceneSummary* inf);
	virtual void 		OnSceneUpdate	();
protected:
	virtual void 		Select			(int  flag);

};

#endif
