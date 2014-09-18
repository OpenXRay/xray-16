//---------------------------------------------------------------------------
#ifndef UI_ActorMainH
#define UI_ActorMainH
//---------------------------------------------------------------------------
#include "../Ecore/editor/ui_main.h"

enum {
	COMMAND_EXTFIRST_EXT = COMMAND_MAIN_LAST-1,

    COMMAND_MAKE_PREVIEW,
    COMMAND_IMPORT,
    COMMAND_EXPORT_OGF,
    COMMAND_EXPORT_OMF,
    COMMAND_EXPORT_DM,
    COMMAND_EXPORT_OBJ,
    COMMAND_EXPORT_CPP,
    COMMAND_BATCH_CONVERT,
    COMMAND_PREVIEW_OBJ_PREF,
    COMMAND_SELECT_PREVIEW_OBJ,
	COMMAND_SHOW_CLIPMAKER,
    COMMAND_OPTIMIZE_MOTIONS,
    COMMAND_MAKE_THUMBNAIL,
    COMMAND_CHANGE_TARGET,

    COMMAND_FILE_MENU,
    COMMAND_LOAD_FIRSTRECENT,
};
//------------------------------------------------------------------------------

class CActorMain: public TUI{
	typedef TUI inherited;

    virtual void 	RealUpdateScene			();
    virtual void 	RealQuit				();
public:
    				CActorMain 				();
    virtual 		~CActorMain				();

    virtual LPSTR	GetCaption				();

    virtual void 	ResetStatus				();
    virtual void 	SetStatus				(LPSTR s, bool bOutLog);
    virtual void	ProgressDraw			();
    virtual void 	OutCameraPos			();
    virtual void 	OutUICursorPos			();
    virtual void 	OutGridSize				();
    virtual void 	OutInfo					();

    virtual LPCSTR	EditorName				(){return "actor";}
    virtual LPCSTR	EditorDesc				(){return "Actor Editor";}

    virtual bool 	ApplyShortCut			(WORD Key, TShiftState Shift);
    virtual bool 	ApplyGlobalShortCut		(WORD Key, TShiftState Shift);

    // commands
	virtual	void	RegisterCommands		(); 
};    
extern CActorMain*&	AUI;

class CAEPreferences :public CCustomPreferences
{
typedef CCustomPreferences inherited;
public:
					CAEPreferences():bAlwaysShowKeyBar12(FALSE),bAlwaysShowKeyBar34(FALSE){}
    BOOL							bAlwaysShowKeyBar12;
    BOOL							bAlwaysShowKeyBar34;

    virtual void 	Load			(CInifile*);
    virtual void 	Save			(CInifile*);
    virtual void	FillProp        (PropItemVec& items);

};
//---------------------------------------------------------------------------
#endif
