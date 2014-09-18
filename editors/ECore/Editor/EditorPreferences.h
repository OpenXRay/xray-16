//---------------------------------------------------------------------------

#ifndef EditorPreferencesH
#define EditorPreferencesH
//---------------------------------------------------------------------------
// refs
class TProperties;

//---------------------------------------------------------------------------
enum{
    epoDrawPivot		= (1<<0),
    epoDrawAnimPath		= (1<<1),
    epoDrawJoints		= (1<<2),
    epoDrawBoneAxis		= (1<<3),
    epoDrawBoneNames	= (1<<4),
    epoDrawBoneShapes	= (1<<5),
    epoShowHint			= (1<<6),
    epoDrawLOD			= (1<<7),
    epoDiscardInstance	= (1<<8),
    epoDeffLoadRB		= (1<<9),
    epoDeffLoadCF		= (1<<10),
    epoSelectInGroup    = (1<<11),
};
class ECORE_API CCustomPreferences
{
private:	// User declarations
    TProperties*	m_ItemProps;
public:
	// view
    float 			view_np;
    float 			view_fp;
    float 			view_fov;
	// fog    
    u32 			fog_color;
    float			fog_fogness;
    // camera
    float			cam_fly_speed;
    float			cam_fly_alt;
    float			cam_sens_rot;
    float			cam_sens_move;
	// tools mouse
    float			tools_sens_rot;
    float			tools_sens_move;
    float			tools_sens_scale;
    BOOL			tools_show_move_axis;
    // box pick
    BOOL			bp_lim_depth;
    BOOL			bp_cull;
    float			bp_depth_tolerance;
    // snap
    float			snap_angle;
    float			snap_move;
    float			snap_moveto;
    // grid
    float			grid_cell_size;
    u32 			grid_cell_count;
    // scene
    u32				scene_undo_level;
    u32				scene_recent_count;
    u32				scene_clear_color;
    AStringVec 		scene_recent_list;
    // objects
    Flags32			object_flags;
    shared_str      sWeather;
protected:
	void 			OnKeyboardCommonFileClick	(ButtonValue* value, bool& bModif, bool& bSafe);
	void 	__stdcall  OnClose	();
    void			ApplyValues			();

    virtual void 	Load				(CInifile*);
    virtual void 	Save				(CInifile*);
public:				// User declarations
    				CCustomPreferences	();
    virtual 		~CCustomPreferences	();

    void			OnCreate			();
    void			OnDestroy			();

    virtual void	FillProp          	(PropItemVec& items);

    void			Edit				();

    void 			Load				();
    void 			Save				();
    
    void 			AppendRecentFile	(LPCSTR name);
    LPCSTR 			FirstRecentFile		(){return scene_recent_list.empty()?"":scene_recent_list.front().c_str();}
};
//---------------------------------------------------------------------------
#define R_FLOAT_SAFE(S,L,D)	I->line_exist(S,L)?I->r_float(S,L):D;
#define R_U32_SAFE(S,L,D) 	I->line_exist(S,L)?I->r_u32(S,L):D;
#define R_BOOL_SAFE(S,L,D) 	I->line_exist(S,L)?I->r_bool(S,L):D;
#define R_STRING_SAFE(S,L,D)I->line_exist(S,L)?I->r_string_wb(S,L):D;
//---------------------------------------------------------------------------
extern ECORE_API CCustomPreferences* 	EPrefs;
//---------------------------------------------------------------------------

#endif
