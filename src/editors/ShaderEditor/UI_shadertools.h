//---------------------------------------------------------------------------
#ifndef UI_ShaderToolsH
#define UI_ShaderToolsH

#include "SHToolsInterface.h"
#include "../ECore/Editor/UI_ToolsCustom.h"
#include "../ECore/Editor/ui_maincommand.h"

// refs
class CEditableObject;
class CLibObject;
class IBlender;
class TProperties;

class CShaderTool: public CToolCustom
{
	typedef CToolCustom inherited;

    void				RegisterTools		();
    void				UnregisterTools		();
    void				RealUpdateProperties();
    void				RealUpdateList		();

    enum{
    	flRefreshProps 	= (1ul<<0ul),
    	flRefreshList 	= (1ul<<1ul),
    };
    Flags32				m_Flags;

    void 				PrepareLighting		();
public:
	TItemList*			m_Items;
    TProperties*		m_ItemProps;
    TProperties*		m_PreviewProps;

    DEFINE_MAP			(EToolsID,ISHTools*,ToolsMap,ToolsPairIt);
    ToolsMap			m_Tools;
    ISHTools*			m_Current;
    ISHTools*			Current				(){return m_Current;}

    void __stdcall	 	OnItemFocused		(ListItemsVec& items);
public:
						CShaderTool			();
    virtual 			~CShaderTool		();

    virtual void		Render				();
	virtual void		RenderEnvironment	() ;
    virtual void		OnFrame				();

    virtual bool		OnCreate			();
    virtual void		OnDestroy			();

    virtual bool		IfModified			();
    virtual bool		IsModified			();
    virtual void		Modified			(); 

    virtual LPCSTR		GetInfo				();
    
    virtual void		ZoomObject			(BOOL bSelOnly);

    virtual bool		Load				(LPCSTR name);
    virtual bool		Save				(LPCSTR name, bool bInternal=false);
    virtual void		Reload				();
    
    virtual void		OnDeviceCreate		();
    virtual void		OnDeviceDestroy		();

    virtual void		Clear				(){inherited::Clear();}

    virtual void		OnShowHint			(AStringVec& SS);

    virtual bool __fastcall 	MouseStart  		(TShiftState Shift){return inherited::MouseStart(Shift);return false;}
    virtual bool __fastcall 	MouseEnd    		(TShiftState Shift){inherited::MouseEnd(Shift); return false;}
    virtual void __fastcall 	MouseMove   		(TShiftState Shift){inherited::MouseMove(Shift);}

    virtual bool		Pick				(TShiftState Shift){return false;}
	virtual bool 		RayPick				(const Fvector& start, const Fvector& dir, float& dist, Fvector* pt, Fvector* n);

    virtual void		ShowProperties		(LPCSTR focused_item);
    virtual void		UpdateProperties	(BOOL bForced=false){m_Flags.set(flRefreshProps,TRUE); if (bForced) RealUpdateProperties();}
    virtual void		RefreshProperties	(){;}
    virtual void		UpdateList			(bool bForced=false){m_Flags.set(flRefreshList,TRUE); if (bForced) RealUpdateList();}
    virtual bool		GetSelectionPosition	(Fmatrix& result);

    LPCSTR				CurrentToolsName	();

    void				OnChangeEditor		(ISHTools* tools);

    void				ApplyChanges		();

    ISHTools*			FindTools			(EToolsID id);
    ISHTools*			FindTools			(TElTabSheet* sheet);

    // commands
    CCommandVar 		CommandSave			(CCommandVar p1, CCommandVar p2);
    CCommandVar 		CommandSaveBackup	(CCommandVar p1, CCommandVar p2);
    CCommandVar 		CommandReload		(CCommandVar p1, CCommandVar p2);
    CCommandVar 		CommandClear		(CCommandVar p1, CCommandVar p2);

    CCommandVar 		CommandUpdateList	(CCommandVar p1, CCommandVar p2);
};
extern CShaderTool*&	STools;
//---------------------------------------------------------------------------
#endif
