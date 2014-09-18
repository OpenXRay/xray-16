//---------------------------------------------------------------------------
#ifndef ESceneObjectToolsH
#define ESceneObjectToolsH

#include "ESceneCustomOTools.h"

class ESceneObjectTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();

    enum{
    	flAppendRandomUpdateProps		= (1<<27),
    	flAppendRandomScaleProportional	= (1<<28),
    	flAppendRandom					= (1<<29),
    	flAppendRandomScale				= (1<<30),
    	flAppendRandomRotation			= (1<<31),
    };
    Flags32				m_Flags;
	bool 				ExportBreakableObjects	(SExportStreams* F);
	bool 				ExportClimableObjects	(SExportStreams* F);

	TProperties* 		m_Props;
    void 				OnChangeAppendRandomFlags(PropValue* prop);
//----------------------------------------------------
public:
    Fvector				m_AppendRandomMinScale;
    Fvector				m_AppendRandomMaxScale;
    Fvector				m_AppendRandomMinRotation;
    Fvector				m_AppendRandomMaxRotation;
    shared_str			m_AppendRandomObjectsStr;
    RStringVec			m_AppendRandomObjects;
public:
						ESceneObjectTool		();

	virtual	bool		AllowEnabling    		(){return false;}

    virtual bool		Validate				(bool full_build);

	virtual void		OnFrame					();

	// definition
    IC LPCSTR			ClassName				(){return "scene_object";}
    IC LPCSTR			ClassDesc				(){return "Scene Object";}
    IC int				RenderPriority			(){return 1;}

    bool				GetBox					(Fbox& bb);

    virtual void		Clear					(bool bSpecific=false);
    // IO
    virtual bool   		IsNeedSave				(){return true;}

    virtual bool   		LoadStream            		(IReader&);
    virtual bool   		LoadLTX            		(CInifile&);
    virtual void   		SaveStream            		(IWriter&);
    virtual void   		SaveLTX            		(CInifile&, int id);

    virtual bool		LoadSelection      		(IReader&);
    virtual void		SaveSelection      		(IWriter&);

    // append random
    void				FillAppendRandomProperties	(bool bUpdateOnly=false);
    void				ActivateAppendRandom		(BOOL val){m_Flags.set(flAppendRandom,val);}
    BOOL				IsAppendRandomActive		(){return m_Flags.is(flAppendRandom);}
    BOOL				IsAppendRandomScaleActive	(){return m_Flags.is(flAppendRandomScale);}
    BOOL				IsAppendRandomRotationActive(){return m_Flags.is(flAppendRandomRotation);}
    BOOL				IsAppendRandomScaleProportional(){return m_Flags.is(flAppendRandomScaleProportional);}

    // tools
    virtual bool		ExportGame         		(SExportStreams* F);
    virtual void		GetStaticDesc			(int& v_cnt, int& f_cnt, bool b_selected_only, bool b_cform);

    virtual CCustomObject* CreateObject			(LPVOID data, LPCSTR name);

	virtual void 		HighlightTexture		(LPCSTR tex_name, bool allow_ratio, u32 t_width, u32 t_height, BOOL mark);
};
//---------------------------------------------------------------------------
#endif
