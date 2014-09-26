#ifndef UI_LevelMainH
#define UI_LevelMainH

#include "../ECore/Editor/ui_main.h"
#include "../ECore/Editor/UI_MainCommand.h"

enum {
	COMMAND_EXTFIRST_EXT = COMMAND_MAIN_LAST-1,

	COMMAND_CHANGE_TARGET,
	COMMAND_ENABLE_TARGET,
	COMMAND_READONLY_TARGET,
	COMMAND_SHOW_TARGET,

	COMMAND_SHOW_OBJECTLIST,
    COMMAND_MULTI_RENAME_OBJECTS,

	COMMAND_REFRESH_SOUND_ENVS,
    COMMAND_REFRESH_SOUND_ENV_GEOMETRY,
    
	COMMAND_CLEAN_LIBRARY,
    COMMAND_LIBRARY_EDITOR,
    COMMAND_LANIM_EDITOR,
    COMMAND_FILE_MENU,
	COMMAND_CLEAR_DEBUG_DRAW,
    COMMAND_IMPORT_COMPILER_ERROR,
    COMMAND_EXPORT_COMPILER_ERROR,
	COMMAND_VALIDATE_SCENE,
    COMMAND_RELOAD_OBJECTS,

	COMMAND_CUT,
	COMMAND_COPY,
	COMMAND_PASTE,
	COMMAND_LOAD_SELECTION,
	COMMAND_SAVE_SELECTION,
    COMMAND_LOAD_LEVEL_PART, 
    COMMAND_UNLOAD_LEVEL_PART,   
    
    COMMAND_CLEAR_SCENE_SUMMARY,
    COMMAND_COLLECT_SCENE_SUMMARY,
	COMMAND_SHOW_SCENE_SUMMARY,			
	COMMAND_EXPORT_SCENE_SUMMARY,
    COMMAND_SCENE_HIGHLIGHT_TEXTURE,

	COMMAND_OPTIONS,
	COMMAND_BUILD,

	COMMAND_MAKE_GAME,
    COMMAND_MAKE_DETAILS,
	COMMAND_MAKE_HOM,
    COMMAND_MAKE_SOM,
    COMMAND_MAKE_AIMAP,

	COMMAND_INVERT_SELECTION_ALL,
	COMMAND_SELECT_ALL,
	COMMAND_DESELECT_ALL,
	COMMAND_DELETE_SELECTION,
	COMMAND_HIDE_UNSEL,
	COMMAND_HIDE_SEL,
	COMMAND_HIDE_ALL,

    COMMAND_SET_SNAP_OBJECTS,
    COMMAND_ADD_SEL_SNAP_OBJECTS,
	COMMAND_DEL_SEL_SNAP_OBJECTS,
    COMMAND_CLEAR_SNAP_OBJECTS,
	COMMAND_SELECT_SNAP_OBJECTS,
	COMMAND_REFRESH_SNAP_OBJECTS,

    COMMAND_LOAD_FIRSTRECENT,

    COMMAND_SHOWCONTEXTMENU,
    COMMAND_SHOW_CLIP_EDITOR,
};
//------------------------------------------------------------------------------

class CLevelMain: public TUI{
	typedef TUI inherited;
    
    virtual void 	RealUpdateScene			();
    virtual void 	RealQuit				();
public:
	CInifile*		m_rt_object_props;
    C3DCursor*   	m_Cursor;
public:
    				CLevelMain 				();
    virtual 		~CLevelMain				();

    void			store_rt_flags			(const CCustomObject* CO);
    void			restore_rt_flags		(CCustomObject* CO);

    virtual LPSTR	GetCaption				();

    virtual void 	ResetStatus				();
    virtual void 	SetStatus				(LPSTR s, bool bOutLog=true);
    virtual void	ProgressDraw			();
    virtual void 	OutCameraPos			();
    virtual void 	OutUICursorPos			();
    virtual void 	OutGridSize				();
    virtual void 	OutInfo					();

    virtual LPCSTR	EditorName				(){return "level";}
    virtual LPCSTR	EditorDesc				(){return "Level Editor";}

    void 			ShowContextMenu			(int cls);
	bool 			PickGround				(Fvector& hitpoint, const Fvector& start, const Fvector& direction, int bSnap=1, Fvector* hitnormal=0);
	bool 			SelectionFrustum		(CFrustum& frustum);

    virtual bool 	ApplyShortCut			(WORD Key, TShiftState Shift);
    virtual bool 	ApplyGlobalShortCut		(WORD Key, TShiftState Shift);

    // commands
	virtual	void	RegisterCommands		(); 

    virtual void	SaveSettings			(CInifile*);
    virtual void	LoadSettings			(CInifile*);
};    
extern CLevelMain*&	LUI;

#endif //UI_MainCommandH



