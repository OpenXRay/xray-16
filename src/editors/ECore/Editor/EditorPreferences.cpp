//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "PropertiesList.h"
#include "ui_main.h"
#include "ui_toolscustom.h"
//---------------------------------------------------------------------------
CCustomPreferences* EPrefs=0;
//---------------------------------------------------------------------------

CCustomPreferences::CCustomPreferences()
{
	// view
    view_np				= 0.1f;
    view_fp				= 1500.f;
    view_fov			= deg2rad(60.f);
	// fog    
    fog_color			= 0x00555555;
    fog_fogness			= 0.9;
    // camera
    cam_fly_speed		= 5.0f;
    cam_fly_alt			= 1.8f;
    cam_sens_rot		= 0.6f;
    cam_sens_move		= 0.6f;
	// tools mouse
    tools_sens_move		= 0.3f;
    tools_sens_rot		= 0.3f;
    tools_sens_scale	= 0.3f;
	tools_show_move_axis =false;
    // box pick
    bp_lim_depth		= TRUE;
    bp_cull				= TRUE;
    bp_depth_tolerance	= 0.1f;
    // snap
    snap_angle			= deg2rad(5.f);
    snap_move			= 0.1f;
    snap_moveto			= 0.5f;
    // grid
    grid_cell_size		= 1.f;
    grid_cell_count		= 100;
    // scene
    scene_undo_level	= 125;
    scene_recent_count	= 10;
    scene_clear_color	= DEFAULT_CLEARCOLOR;
    // objects
    object_flags.zero	();
}
//---------------------------------------------------------------------------

CCustomPreferences::~CCustomPreferences()
{
}

void CCustomPreferences::ApplyValues()
{
	Tools->m_MoveSnap		= snap_move;
	Tools->m_MoveSnapTo		= snap_moveto;
	Tools->m_RotateSnapAngle= snap_angle;

    EDevice.m_Camera.SetViewport(view_np, view_fp, rad2deg(view_fov));
    Tools->SetFog	(fog_color,fog_fogness);

    UI->m_MouseSM	= 0.2f*tools_sens_move*tools_sens_move;
    UI->m_MouseSR	= 0.02f*tools_sens_rot*tools_sens_rot;
    UI->m_MouseSS	= 0.02f*tools_sens_scale*tools_sens_scale;

    EDevice.m_Camera.SetSensitivity	(cam_sens_move, cam_sens_rot);
    EDevice.m_Camera.SetFlyParams	(cam_fly_speed, cam_fly_alt);

    ExecCommand		(COMMAND_UPDATE_GRID);
}
//---------------------------------------------------------------------------

void __stdcall CCustomPreferences::OnClose	()
{
	ApplyValues	();	
}
//---------------------------------------------------------------------------


void CheckValidate(ShortcutValue*, const xr_shortcut& new_val, bool& result)
{
	result 					= true; 
    ECommandVec& cmds		= GetEditorCommands();
    for (u32 cmd_idx=0; cmd_idx<cmds.size(); cmd_idx++){
    	SECommand*& CMD		= cmds[cmd_idx];
        if (CMD&&CMD->editable){
        	VERIFY(!CMD->sub_commands.empty());
		    for (u32 sub_cmd_idx=0; sub_cmd_idx<CMD->sub_commands.size(); sub_cmd_idx++){
            	SESubCommand*& SUB_CMD = CMD->sub_commands[sub_cmd_idx];
                if (SUB_CMD->shortcut.similar(new_val)){ result = false; return;}
            }
        }
    }
}

void CCustomPreferences::OnKeyboardCommonFileClick(ButtonValue* B, bool& bModif, bool&)
{
    bModif = false;
    xr_string fn;
	switch(B->btn_num){
    case 0:
        if(EFS.GetOpenName("$import$", fn, false, NULL, 6)){
            CInifile* 	I 	= xr_new<CInifile>(fn.c_str(), TRUE, TRUE, TRUE);
		    LoadShortcuts	(I);
            xr_delete		(I);
            m_ItemProps->RefreshForm();
        }
    break;
    case 1:
        if(EFS.GetSaveName("$import$", fn, NULL, 6)){
		    CInifile* 	I 	= xr_new<CInifile>(fn.c_str(), FALSE, TRUE, TRUE);
		    SaveShortcuts	(I);
            xr_delete		(I);
        }
    break;
	}
}

void CCustomPreferences::FillProp(PropItemVec& props)
{
    PHelper().CreateFlag32	(props,"Objects\\Library\\Discard Instance",	&object_flags, 	epoDiscardInstance);
    PHelper().CreateFlag32	(props,"Objects\\Skeleton\\Draw Joints",		&object_flags, 	epoDrawJoints);
    PHelper().CreateFlag32	(props,"Objects\\Skeleton\\Draw Bone Axis",		&object_flags, 	epoDrawBoneAxis);
    PHelper().CreateFlag32	(props,"Objects\\Skeleton\\Draw Bone Names",	&object_flags, 	epoDrawBoneNames);
    PHelper().CreateFlag32	(props,"Objects\\Skeleton\\Draw Bone Shapes",	&object_flags, 	epoDrawBoneShapes);
    PHelper().CreateFlag32	(props,"Objects\\Show\\Hint",					&object_flags, 	epoShowHint);
    PHelper().CreateFlag32	(props,"Objects\\Show\\Pivot",					&object_flags, 	epoDrawPivot);
    PHelper().CreateFlag32	(props,"Objects\\Show\\Animation Path",			&object_flags, 	epoDrawAnimPath);
    PHelper().CreateFlag32	(props,"Objects\\Show\\LOD",					&object_flags, 	epoDrawLOD);
    PHelper().CreateFlag32	(props,"Objects\\Loading\\Deffered Loading RB",	&object_flags, 	epoDeffLoadRB);
    PHelper().CreateFlag32	(props,"Objects\\Loading\\Deffered Loading CF",	&object_flags, 	epoDeffLoadCF);
    PHelper().CreateFlag32	(props,"Objects\\GroupObject\\Select ingroup",	&object_flags, 	epoSelectInGroup);

    PHelper().CreateU32		(props,"Scene\\Common\\Recent Count", 		    &scene_recent_count,0, 		25);
    PHelper().CreateU32		(props,"Scene\\Common\\Undo Level", 		    &scene_undo_level,	0, 		125);
    PHelper().CreateFloat	(props,"Scene\\Grid\\Cell Size", 	           	&grid_cell_size,	0.1f,	10.f);
    PHelper().CreateU32		(props,"Scene\\Grid\\Cell Count", 	           	&grid_cell_count,	10, 	1000);

    PHelper().CreateBOOL	(props,"Tools\\Box Pick\\Limited Depth",		&bp_lim_depth);
    PHelper().CreateBOOL	(props,"Tools\\Box Pick\\Back Face Culling",	&bp_cull);
    PHelper().CreateFloat	(props,"Tools\\Box Pick\\Depth Tolerance",		&bp_depth_tolerance,0.f, 	10000.f);
    PHelper().CreateFloat	(props,"Tools\\Sens\\Move",			          	&tools_sens_move);
    PHelper().CreateBOOL	(props,"Tools\\Sens\\ShowMoveAxis",				&tools_show_move_axis);
    
    PHelper().CreateFloat	(props,"Tools\\Sens\\Rotate",		          	&tools_sens_rot);
    PHelper().CreateFloat	(props,"Tools\\Sens\\Scale",		          	&tools_sens_scale);
    PHelper().CreateAngle	(props,"Tools\\Snap\\Angle",		          	&snap_angle,		0, 		PI_MUL_2);
    PHelper().CreateFloat	(props,"Tools\\Snap\\Move",			          	&snap_move, 		0.01f,	1000.f);
    PHelper().CreateFloat	(props,"Tools\\Snap\\Move To", 		          	&snap_moveto,		0.01f,	1000.f);


    PHelper().CreateFloat	(props,"Viewport\\Camera\\Move Sens",		    &cam_sens_move);
    PHelper().CreateFloat	(props,"Viewport\\Camera\\Rotate Sens",		    &cam_sens_rot);
    PHelper().CreateFloat	(props,"Viewport\\Camera\\Fly Speed",		    &cam_fly_speed, 	0.01f, 	100.f);
    PHelper().CreateFloat	(props,"Viewport\\Camera\\Fly Altitude",	    &cam_fly_alt, 		0.f, 	1000.f);
    PHelper().CreateColor	(props,"Viewport\\Fog\\Color",				    &fog_color	);
    PHelper().CreateFloat	(props,"Viewport\\Fog\\Fogness",			    &fog_fogness, 		0.f, 	100.f);
    PHelper().CreateFloat	(props,"Viewport\\Near Plane",				    &view_np, 			0.01f,	10.f);
    PHelper().CreateFloat	(props,"Viewport\\Far Plane", 				    &view_fp,			10.f, 	10000.f);
    PHelper().CreateAngle	(props,"Viewport\\FOV",		  				    &view_fov,			deg2rad(0.1f), deg2rad(170.f));
    PHelper().CreateColor	(props,"Viewport\\Clear Color",		           	&scene_clear_color	);
    
    ButtonValue* B = PHelper().CreateButton	(props,"Keyboard\\Common\\File","Load,Save", 0);
    B->OnBtnClickEvent.bind	(this,&CCustomPreferences::OnKeyboardCommonFileClick);
    ECommandVec& cmds		= GetEditorCommands();
    for (u32 cmd_idx=0; cmd_idx<cmds.size(); cmd_idx++){
    	SECommand*& CMD		= cmds[cmd_idx];
        if (CMD&&CMD->editable){
        	VERIFY(!CMD->sub_commands.empty());
		    for (u32 sub_cmd_idx=0; sub_cmd_idx<CMD->sub_commands.size(); sub_cmd_idx++){
            	SESubCommand*& SUB_CMD = CMD->sub_commands[sub_cmd_idx];
                string128 nm; 		sprintf(nm,"%s%s%s",CMD->Desc(),!SUB_CMD->desc.empty()?"\\":"",SUB_CMD->desc.c_str());
                ShortcutValue* V 	= PHelper().CreateShortcut(props,PrepareKey("Keyboard\\Shortcuts",nm), &SUB_CMD->shortcut);
                V->OnValidateResultEvent.bind(CheckValidate);
            }
        }
    }
}

void CCustomPreferences::Edit()
{
    // fill prop
	PropItemVec props;

    FillProp						(props);

	m_ItemProps->AssignItems		(props);
    m_ItemProps->ShowPropertiesModal();

    // save changed options
    Save							();
}
//---------------------------------------------------------------------------

void CCustomPreferences::Load(CInifile* I)
{
    psDeviceFlags.flags		= R_U32_SAFE	("editor_prefs","device_flags",	psDeviceFlags.flags);
    psSoundFlags.flags		= R_U32_SAFE	("editor_prefs","sound_flags",	psSoundFlags.flags)

    Tools->m_Settings.flags	= R_U32_SAFE	("editor_prefs","tools_settings",Tools->m_Settings.flags);
    
    view_np				= R_FLOAT_SAFE	("editor_prefs","view_np"			,view_np		 	);
    view_fp				= R_FLOAT_SAFE	("editor_prefs","view_fp"			,view_fp		 	);
    view_fov			= R_FLOAT_SAFE	("editor_prefs","view_fov"			,view_fov			);

    fog_color			= R_U32_SAFE	("editor_prefs","fog_color"			,fog_color			);
    fog_fogness			= R_FLOAT_SAFE	("editor_prefs","fog_fogness"		,fog_fogness	 	);

    cam_fly_speed		= R_FLOAT_SAFE	("editor_prefs","cam_fly_speed"		,cam_fly_speed		);
    cam_fly_alt			= R_FLOAT_SAFE	("editor_prefs","cam_fly_alt"		,cam_fly_alt	 	);
    cam_sens_rot		= R_FLOAT_SAFE	("editor_prefs","cam_sens_rot"		,cam_sens_rot		);
    cam_sens_move		= R_FLOAT_SAFE	("editor_prefs","cam_sens_move"		,cam_sens_move		);

    tools_sens_move		= R_FLOAT_SAFE	("editor_prefs","tools_sens_move"	,tools_sens_move  	);
    tools_sens_rot		= R_FLOAT_SAFE	("editor_prefs","tools_sens_rot"	,tools_sens_rot		);
    tools_sens_scale	= R_FLOAT_SAFE	("editor_prefs","tools_sens_scale"	,tools_sens_scale	);
	tools_show_move_axis= R_BOOL_SAFE	("editor_prefs","tools_show_move_axis"	,tools_show_move_axis);
    
    bp_lim_depth		= R_BOOL_SAFE	("editor_prefs","bp_lim_depth"		,bp_lim_depth		);
    bp_cull				= R_BOOL_SAFE	("editor_prefs","bp_lim_depth"		,bp_cull		  	);
    bp_depth_tolerance	= R_FLOAT_SAFE	("editor_prefs","tools_sens_rot"	,bp_depth_tolerance	);

    snap_angle			= R_FLOAT_SAFE	("editor_prefs","snap_angle"		,snap_angle			);
    snap_move			= R_FLOAT_SAFE	("editor_prefs","snap_move"			,snap_move			);
    snap_moveto			= R_FLOAT_SAFE	("editor_prefs","snap_moveto"		,snap_moveto	   	);

    grid_cell_size		= R_FLOAT_SAFE	("editor_prefs","grid_cell_size"	,grid_cell_size		);
    grid_cell_count		= R_U32_SAFE	("editor_prefs","grid_cell_count"	,grid_cell_count   	);

    scene_undo_level	= R_U32_SAFE	("editor_prefs","scene_undo_level"	,scene_undo_level	);
    scene_recent_count	= R_U32_SAFE	("editor_prefs","scene_recent_count",scene_recent_count	);
    scene_clear_color	= R_U32_SAFE	("editor_prefs","scene_clear_color"	,scene_clear_color	);

    object_flags.flags	= R_U32_SAFE	("editor_prefs","object_flags"		,object_flags.flags );

	// read recent list    
    for (u32 i=0; i<scene_recent_count; i++){
    	shared_str fn  	= R_STRING_SAFE	("editor_prefs",AnsiString().sprintf("recent_files_%d",i).c_str(),shared_str("") );
        if (fn.size())
        {
        	AStringIt it =   std::find(scene_recent_list.begin(), scene_recent_list.end(), fn.c_str() ) ;
            if (it==scene_recent_list.end())
	        	scene_recent_list.push_back(*fn);
        }
    }
    sWeather = R_STRING_SAFE	("editor_prefs", "weather", shared_str("") );
    // load shortcuts
    LoadShortcuts		(I);

    UI->LoadSettings	(I);
}

void CCustomPreferences::Save(CInifile* I)
{
    I->w_u32	("editor_prefs","device_flags",		psDeviceFlags.flags	);
    I->w_u32	("editor_prefs","sound_flags",		psSoundFlags.flags	);

    I->w_u32	("editor_prefs","tools_settings",	Tools->m_Settings.flags	);

    I->w_float	("editor_prefs","view_np",			view_np			);
    I->w_float	("editor_prefs","view_fp",			view_fp			);
    I->w_float	("editor_prefs","view_fov",			view_fov		);

    I->w_u32	("editor_prefs","fog_color",		fog_color		);
    I->w_float	("editor_prefs","fog_fogness",		fog_fogness		);

    I->w_float	("editor_prefs","cam_fly_speed",	cam_fly_speed	);
    I->w_float	("editor_prefs","cam_fly_alt",		cam_fly_alt		);
    I->w_float	("editor_prefs","cam_sens_rot",		cam_sens_rot	);
    I->w_float	("editor_prefs","cam_sens_move",	cam_sens_move	);

    I->w_float	("editor_prefs","tools_sens_rot",	tools_sens_rot	);
    I->w_float	("editor_prefs","tools_sens_move",	tools_sens_move	);
    I->w_float	("editor_prefs","tools_sens_scale",	tools_sens_scale);
	I->w_bool	("editor_prefs","tools_show_move_axis",tools_show_move_axis);
    
    I->w_bool	("editor_prefs","bp_lim_depth",		bp_lim_depth	);
    I->w_bool	("editor_prefs","bp_lim_depth",		bp_cull			);
    I->w_float	("editor_prefs","bp_depth_tolerance",bp_depth_tolerance	);

    I->w_float	("editor_prefs","snap_angle",		snap_angle		);
    I->w_float	("editor_prefs","snap_move",		snap_move		);
    I->w_float	("editor_prefs","snap_moveto",		snap_moveto		);

    I->w_float	("editor_prefs","grid_cell_size",	grid_cell_size	);
    I->w_u32	("editor_prefs","grid_cell_count",	grid_cell_count	);

    I->w_u32	("editor_prefs","scene_undo_level",		scene_undo_level	);
    I->w_u32	("editor_prefs","scene_recent_count",	scene_recent_count	);
    I->w_u32	("editor_prefs","scene_clear_color",	scene_clear_color 	);

    I->w_u32	("editor_prefs","object_flags",		object_flags.flags);

    for (AStringIt it=scene_recent_list.begin(); it!=scene_recent_list.end(); it++){
    	AnsiString L; L.sprintf("recent_files_%d",it-scene_recent_list.begin());
    	AnsiString V; V.sprintf("\"%s\"",it->c_str());
		I->w_string("editor_prefs",L.c_str(),V.c_str());
    }
    I->w_string("editor_prefs","weather",   sWeather.c_str() );
    // load shortcuts
    SaveShortcuts		(I);
    UI->SaveSettings	(I);
}

void CCustomPreferences::Load()
{
	string_path			fn;
	INI_NAME			(fn);
    CInifile* I			= xr_new<CInifile>(fn, TRUE, TRUE, TRUE);
    Load				(I);
    xr_delete			(I);
    ApplyValues			();
}
void CCustomPreferences::Save()
{
	string_path			fn;
	INI_NAME			(fn);
    CInifile* I 		= xr_new<CInifile>(fn, FALSE, TRUE, TRUE);
    I->set_override_names(TRUE);
	Save				(I);
	xr_delete			(I);
}

void CCustomPreferences::AppendRecentFile(LPCSTR name)
{
    for (AStringIt it=scene_recent_list.begin(); it!=scene_recent_list.end(); it++){
    	if (*it==name){
        	scene_recent_list.erase	(it);
            break;
        }
    }
	scene_recent_list.insert(scene_recent_list.begin(),name);
	while (scene_recent_list.size()>=EPrefs->scene_recent_count) 
    	scene_recent_list.pop_back();

    ExecCommand				(COMMAND_REFRESH_UI_BAR);
}
//---------------------------------------------------------------------------

void CCustomPreferences::OnCreate()
{
	Load				();
	m_ItemProps 		= TProperties::CreateModalForm("Editor Preferences",false,0,0,TOnCloseEvent(this,&CCustomPreferences::OnClose),TProperties::plItemFolders|TProperties::plFullSort); //TProperties::plFullExpand TProperties::plFullSort TProperties::plNoClearStore|TProperties::plFolderStore|
}
//---------------------------------------------------------------------------

void CCustomPreferences::OnDestroy()
{
    TProperties::DestroyForm(m_ItemProps);
    Save				();
}
//---------------------------------------------------------------------------

