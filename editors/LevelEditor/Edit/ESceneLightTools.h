#ifndef ESceneLightToolsH
#define ESceneLightToolsH

#include "ESceneCustomOTools.H"
#include "xr_efflensflare.h"

class CEditFlare: public CLensFlare{
public:
					CEditFlare();
  	void 			Load(IReader& F);
	void 			Save(IWriter& F);
    void			Render();
    void			DeleteShaders();
    void			CreateShaders();
};

class ESceneLightTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
    friend class 		SceneBuilder;
    friend class 		CLight;
protected:
	enum{
    	flShowSun			= (1<<31),
        flShowControlName	= (1<<30),
    };
    Flags32				m_Flags;
	// hemisphere
    u32					m_HemiControl;
    // sun
    Fvector2			m_SunShadowDir;

    // run time
    xr_vector<CLight*> 	frame_light;
	void 				AppendFrameLight		(CLight* L);
protected:
    // light control
	int					lcontrol_last_idx;
	RTokenVec			lcontrols;
    void __stdcall  	OnControlAppendClick		(ButtonValue* sender, bool& bDataModified, bool& bSafe);
    void __stdcall  	OnControlRenameRemoveClick	(ButtonValue* sender, bool& bDataModified, bool& bSafe);
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
						ESceneLightTool 	   	();
	virtual        	 	~ESceneLightTool		();

    virtual void		Clear					(bool bSpecific=false);

	// definition
    IC LPCSTR			ClassName				(){return "light";}
    IC LPCSTR			ClassDesc				(){return "Light";}
    IC int				RenderPriority			(){return 10;}

    // IO
    virtual bool   		IsNeedSave				(){return true;}
    virtual bool   		LoadStream            		(IReader&);
	virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
    virtual void   		SaveLTX            		(CInifile&, int id);
    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);

    // utils
    virtual bool		Validate				(bool full_build);
    
    virtual void		BeforeRender			();
    virtual void		OnRender				(int priority, bool strictB2F);
    virtual void		AfterRender				();

	void 				SelectLightsForObject	(CCustomObject* obj);
    
	virtual void 		FillProp				(LPCSTR pref, PropItemVec& items);

    AnsiString			GenLightControlName		();
    xr_rtoken*   		FindLightControl		(int id);
    RTokenVecIt	   		FindLightControlIt		(LPCSTR name);
    xr_rtoken*   		FindLightControl		(LPCSTR name){RTokenVecIt it = FindLightControlIt(name); return it!=lcontrols.end()?it:0;}
    void				AppendLightControl		(LPCSTR name, u32* idx=0);
    void				RemoveLightControl		(LPCSTR name);

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name);
};
#endif // ESceneCustomOToolsH

