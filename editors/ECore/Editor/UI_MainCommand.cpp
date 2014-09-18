//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "ui_main.h"
#include "UI_ToolsCustom.h"

#include "ImageEditor.h"
#include "MinimapEditor.h"
#include "SoundEditor.h"
#include "d3dutils.h"

#include "PSLibrary.h"
#include "Library.h"
#include "LightAnimLibrary.h"

#include "ImageManager.h"
#include "SoundManager.h"
#include "ResourceManager.h"
#include "igame_persistent.h"
#include "NumericVector.h"

#include "TextForm.h"


ECommandVec 		ECommands;
BOOL 				bAllowReceiveCommand	= FALSE;
BOOL 				bAllowLogCommands		= FALSE;
TfrmText*			frmEditCommandList		= 0;
AnsiString			sCommandListText;

BOOL AllowLogCommands()
{
	return bAllowLogCommands;
}

ECommandVec&  GetEditorCommands()
{
	return 	ECommands;
}
void 	EnableReceiveCommands()
{
	bAllowReceiveCommand = TRUE;
}
SESubCommand* FindCommandByShortcut(const xr_shortcut& val)
{
    ECommandVec& cmds		= GetEditorCommands();
    for (u32 cmd_idx=0; cmd_idx<cmds.size(); cmd_idx++){
    	SECommand*& CMD		= cmds[cmd_idx];
        if (CMD&&CMD->editable){
        	VERIFY(!CMD->sub_commands.empty());
		    for (u32 sub_cmd_idx=0; sub_cmd_idx<CMD->sub_commands.size(); sub_cmd_idx++){
            	SESubCommand*& SUB_CMD = CMD->sub_commands[sub_cmd_idx];
                if (SUB_CMD->shortcut.similar(val)) return SUB_CMD;
            }
        }
    }
    return 0;
}
SECommand* FindCommandByName(LPCSTR nm)
{
    ECommandVec& cmds		= GetEditorCommands();
    for (u32 cmd_idx=0; cmd_idx<cmds.size(); cmd_idx++){
    	SECommand*& CMD		= cmds[cmd_idx];
        if (CMD&&(0==stricmp(CMD->name,nm))) return CMD;
    }
    return 0;
}
SESubCommand* FindSubCommandByName(SECommand* CMD, LPCSTR nm)
{
    VERIFY(CMD && !CMD->sub_commands.empty());
    for (u32 sub_cmd_idx=0; sub_cmd_idx<CMD->sub_commands.size(); sub_cmd_idx++){
        SESubCommand* SUB_CMD = CMD->sub_commands[sub_cmd_idx];
        if (0==stricmp(SUB_CMD->desc.c_str(),nm)) return SUB_CMD;
    }
    return 0;
}
void ParseParam(xr_string sp, CCommandVar& res)
{
    if (!sp.empty()){
    	u32 rs=0,ip=0;
        if (0==strstr(sp.c_str(),"\""))
        	rs			= sscanf(sp.c_str(),"%d",&ip); 
        if (1!=rs){
            _GetItem(sp.c_str(),1,sp,'\"');
            if (!sp.empty()) res = sp;
        }else		res = ip;
    }
}
CCommandVar		ExecCommand	(const xr_shortcut& val)
{
	SESubCommand* SUB 	= FindCommandByShortcut(val);
    CCommandVar res		= CCommandVar(u32(0));
    if (SUB)
    	res				= ExecCommand(SUB->parent->idx,SUB->p0,SUB->p1);
    return res;
}
CCommandVar 	ExecCommand	(u32 cmd, CCommandVar p1, CCommandVar p2)
{
	if (!bAllowReceiveCommand)	return 0;

	VERIFY				(cmd<ECommands.size());
    CCommandVar	res;
	SECommand*	CMD 	= ECommands[cmd];
    VERIFY				(CMD&&!CMD->command.empty());
    static int exec_level= 0;
    if (bAllowLogCommands){
    	string128 level;strcpy(level,exec_level==0?"":";");
        for(int k=0; k<exec_level; ++k) strcat(level,".");
        xr_string sp1	= p1.IsString()?xr_string(p1):xr_string("");
        xr_string sp2	= p2.IsString()?xr_string(p2):xr_string("");
        if (p1.IsString()) sp1 = ((sp1.find("\n")==sp1.npos)&&(sp1.find("\r")==sp1.npos))?sp1:xr_string("..."); 
        if (p2.IsString()) sp2 = ((sp2.find("\n")==sp2.npos)&&(sp2.find("\r")==sp2.npos))?sp2:xr_string("...");
        if (p1.IsString()&&p2.IsString()) 		Msg("%s%s (\"%s\",\"%s\")",	level,CMD->Name(),sp1.c_str(),sp2.c_str());
        else if (p1.IsInteger()&&p2.IsInteger())Msg("%s%s (%d,%d)",			level,CMD->Name(),u32(p1),u32(p2));
        else if (p1.IsInteger()&&p2.IsString()) Msg("%s%s (%d,\"%s\")",		level,CMD->Name(),u32(p1),sp2.c_str());
        else if (p1.IsString()&&p2.IsInteger()) Msg("%s%s (\"%s\",%d)",		level,CMD->Name(),sp1.c_str(),u32(p2));
    }
    exec_level++;
    res 	 			= CMD->command(p1,p2);
    exec_level--; 		VERIFY(exec_level>=0);
    return res;
}
void	RegisterCommand (u32 cmd, SECommand* cmd_impl)
{
	if (cmd>=ECommands.size()) 
    	ECommands.resize(cmd+1,0);
	SECommand*&	CMD = ECommands[cmd];
    if (CMD){
    	Msg			("RegisterCommand: command '%s' overridden by command '%s'.",*CMD->desc,*cmd_impl->desc);
    	xr_delete	(CMD);
    }
    CMD	   			= cmd_impl;
}
void	RegisterSubCommand(SECommand* cmd_impl, LPCSTR desc, CCommandVar p0, CCommandVar p1)
{
    VERIFY		(cmd_impl);
    cmd_impl->AppendSubCommand(desc,p0,p1);
}
BOOL	LoadShortcuts(CInifile* ini)
{
    for (u32 cmd_idx=0; cmd_idx<ECommands.size(); cmd_idx++){
    	SECommand*& CMD		= ECommands[cmd_idx];
        if (CMD&&CMD->editable){
		    for (u32 sub_cmd_idx=0; sub_cmd_idx<CMD->sub_commands.size(); sub_cmd_idx++){
            	SESubCommand*& SUB	 	= CMD->sub_commands[sub_cmd_idx];
                string256 nm,tmp; 	
                if (SUB->desc.size())	sprintf(nm,"%s.\"%s\"",CMD->Name(),SUB->desc.c_str());
                else   					sprintf(nm,"%s",CMD->Name());
                if (ini->line_exist("shortcuts",nm)){ 
                	LPCSTR val			= ini->r_string("shortcuts",nm);
                    int res 			= sscanf(val,"%d,%s",&SUB->shortcut.hotkey,tmp);
                    if (2==res){
                    	xr_string 		sp;
                    	_GetItem		(tmp,0,sp);
                        ParseParam		(sp,SUB->p0);
                    	_GetItem		(tmp,1,sp);
                        ParseParam		(sp,SUB->p1);
                    }
                }
            }
        }
    }
	return TRUE;
}
BOOL	SaveShortcuts(CInifile* ini)
{
    for (u32 cmd_idx=0; cmd_idx<ECommands.size(); cmd_idx++){
    	SECommand*& CMD		= ECommands[cmd_idx];
        if (CMD&&CMD->editable){
		    for (u32 sub_cmd_idx=0; sub_cmd_idx<CMD->sub_commands.size(); sub_cmd_idx++){
            	SESubCommand*& SUB = CMD->sub_commands[sub_cmd_idx];
                string256 nm,tmp; 	
                if (SUB->desc.size())	sprintf(nm,"%s.\"%s\"",CMD->Name(),SUB->desc.c_str());
                else   					sprintf(nm,"%s",CMD->Name());
                if (SUB->p0.IsString()&&SUB->p1.IsString()) 		sprintf(tmp,"%d, \"%s\",\"%s\"",SUB->shortcut.hotkey,xr_string(SUB->p0).c_str(),xr_string(SUB->p1).c_str());
                else if (SUB->p0.IsInteger()&&SUB->p1.IsInteger())	sprintf(tmp,"%d, %d,%d",		SUB->shortcut.hotkey,u32(SUB->p0),u32(SUB->p1));
                else if (SUB->p0.IsInteger()&&SUB->p1.IsString()) 	sprintf(tmp,"%d, %d,\"%s\"",	SUB->shortcut.hotkey,u32(SUB->p0),xr_string(SUB->p1).c_str());
                else if (SUB->p0.IsString()&&SUB->p1.IsInteger()) 	sprintf(tmp,"%d, \"%s\",%d",	SUB->shortcut.hotkey,xr_string(SUB->p0).c_str(),u32(SUB->p1));
                ini->w_string	("shortcuts",nm,tmp);
            }
        }
    }
	return TRUE;
}
void	ClearCommands()
{
	for (ECommandVecIt it=ECommands.begin(); it!=ECommands.end(); it++)
    	xr_delete	(*it);
	ECommands.clear	();
}

void	TUI::ClearCommands ()
{
	::ClearCommands	();
}

//------------------------------------------------------------------------------
// UI Commands
//------------------------------------------------------------------------------
CCommandVar	TUI::CommandRenderFocus(CCommandVar p1, CCommandVar p2)
{
    if (((TForm*)m_D3DWindow->Owner)->Visible&&m_bReady)
        m_D3DWindow->SetFocus();
    return 1;
}
CCommandVar	TUI::CommandBreakLastOperation(CCommandVar p1, CCommandVar p2)
{
    if (mrYes==ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Are you sure to break current action?")){
        NeedBreak	();
        ELog.Msg	(mtInformation,"Execution canceled.");
    }
    return 1;
}
CCommandVar 	TUI::CommandRenderResize(CCommandVar p1, CCommandVar p2)
{
    if (psDeviceFlags.is(rsDrawSafeRect)){
        int w=m_D3DPanel->Width,h=m_D3DPanel->Height,w_2=w/2,h_2=h/2;
        Irect rect;
        if ((0.75f*float(w))>float(h)) 	rect.set(w_2-1.33f*float(h_2),0,1.33f*h,h);
        else                   			rect.set(0,h_2-0.75f*float(w_2),w,0.75f*w);
        m_D3DWindow->Left  	= rect.x1;
        m_D3DWindow->Top  	= rect.y1;
        m_D3DWindow->Width 	= rect.x2;
        m_D3DWindow->Height	= rect.y2;
    }else{
        m_D3DWindow->Left  	= 0;
        m_D3DWindow->Top  	= 0;
        m_D3DWindow->Width 	= m_D3DPanel->Width;
        m_D3DWindow->Height	= m_D3DPanel->Height;
    }
    UI->RedrawScene		();
    return 1;
}

//------------------------------------------------------------------------------
// Common Commands
//------------------------------------------------------------------------------
CCommandVar CommandInitialize(CCommandVar p1, CCommandVar p2)
{
	CCommandVar res		= TRUE;
    Engine.Initialize	();
    // make interface
    //----------------
    EPrefs->OnCreate		();
    if (UI->OnCreate((TD3DWindow*)(u32)p1,(TPanel*)(u32)p2))
    {
        ExecCommand		(COMMAND_CREATE_SOUND_LIB);	R_ASSERT(SndLib);
        SndLib->OnCreate();
        LALib.OnCreate	();
        Lib.OnCreate	();
        BOOL bWeather = psDeviceFlags.is(rsEnvironment);
        psDeviceFlags.set(rsEnvironment, FALSE);
        g_pGamePersistent= xr_new<IGame_Persistent>();

        if (Tools->OnCreate())
        {
        	EPrefs->Load();
            EDevice.seqAppStart.Process(rp_AppStart);
            ExecCommand	(COMMAND_RESTORE_UI_BAR);
            ExecCommand	(COMMAND_REFRESH_UI_BAR);
            ExecCommand	(COMMAND_CLEAR);
            ExecCommand	(COMMAND_RENDER_FOCUS);
            ExecCommand	(COMMAND_CHANGE_ACTION, etaSelect);
            ExecCommand	(COMMAND_RENDER_RESIZE);
/*
            if(bWeather && EPrefs->sWeather.size() )
            {
                psDeviceFlags.set(rsEnvironment, TRUE);
                g_pGamePersistent->Environment().SetWeather(EPrefs->sWeather, true);

            }
*/
        }else{
        	res			= FALSE;
        }
    }else{
        res 			= FALSE;
    }
    return res;
}             
CCommandVar 	CommandDestroy(CCommandVar p1, CCommandVar p2)
{
    ExecCommand			(COMMAND_SAVE_UI_BAR);
    EPrefs->OnDestroy	();
    ExecCommand			(COMMAND_CLEAR);
    EDevice.seqAppEnd.Process(rp_AppEnd);
    xr_delete			(g_pGamePersistent);
    LALib.OnDestroy		();
    Tools->OnDestroy	();
    SndLib->OnDestroy	();
    xr_delete			(SndLib);
    Lib.OnDestroy		();
    UI->OnDestroy		();
    Engine.Destroy		();
    return				TRUE;
}             
CCommandVar 	CommandQuit(CCommandVar p1, CCommandVar p2)
{
    UI->Quit			();
    return				TRUE;
}             
CCommandVar 	CommandEditorPrefs(CCommandVar p1, CCommandVar p2)
{
    EPrefs->Edit		();
    return				TRUE;
}             
CCommandVar 	CommandChangeAction(CCommandVar p1, CCommandVar p2)
{
     Tools->SetAction	(ETAction(u32(p1)));
    return				TRUE;
}             
CCommandVar 	CommandChangeAxis(CCommandVar p1, CCommandVar p2)
{
    Tools->SetAxis	(ETAxis(u32(p1)));
    return				TRUE;
}

CCommandVar 	CommandSimulate(CCommandVar p1, CCommandVar p2)
{

	Tools->Simulate();

    
    return				TRUE;
}

CCommandVar 	CommandUseSimulatePositions(CCommandVar p1, CCommandVar p2)
{

   Tools->UseSimulatePositions();

    return				TRUE;
}



CCommandVar 	CommandSetSettings(CCommandVar p1, CCommandVar p2)
{
	Tools->SetSettings(p1,p2);
    return				TRUE;
}             
CCommandVar 	CommandSoundEditor(CCommandVar p1, CCommandVar p2)
{
    TfrmSoundLib::EditLib(AnsiString("Sound Editor"));
    return				TRUE;
}
CCommandVar 	CommandSyncSounds(CCommandVar p1, CCommandVar p2)
{
    if (ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Are you sure to synchronize sounds?")==mrYes)
        SndLib->RefreshSounds(true);
    return				TRUE;
}
CCommandVar 	CommandImageEditor(CCommandVar p1, CCommandVar p2)
{
    TfrmImageLib::EditLib(AnsiString("Image Editor"));
    return				TRUE;
}

CCommandVar 	CommandMinimapEditor(CCommandVar p1, CCommandVar p2)
{
    TTMinimapEditor::Show   ();
    return				    TRUE;
}

CCommandVar 	CommandCheckTextures(CCommandVar p1, CCommandVar p2)
{
    TfrmImageLib::ImportTextures();
    return				TRUE;
}
CCommandVar 	CommandRefreshTextures(CCommandVar p1, CCommandVar p2)
{
    if (ELog.DlgMsg(mtConfirmation,TMsgDlgButtons() << mbYes << mbNo,"Are you sure to synchronize textures?")==mrYes)
        ImageLib.RefreshTextures(0);
    return				TRUE;
}
CCommandVar 	CommandReloadTextures(CCommandVar p1, CCommandVar p2)
{
    EDevice.ReloadTextures();
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandChangeSnap(CCommandVar p1, CCommandVar p2)
{
    ((TExtBtn*)(u32)p1)->Down = !((TExtBtn*)(u32)p1)->Down;
    return				TRUE;
}
CCommandVar 	CommandUnloadTextures(CCommandVar p1, CCommandVar p2)
{
    EDevice.UnloadTextures();
    return				TRUE;
}
CCommandVar 	CommandEvictObjects(CCommandVar p1, CCommandVar p2)
{
    Lib.EvictObjects	();
    return				TRUE;
}
CCommandVar 	CommandEvictTextures(CCommandVar p1, CCommandVar p2)
{
    EDevice.Resources->Evict();
    return				TRUE;
}
CCommandVar 	CommandCheckModified(CCommandVar p1, CCommandVar p2)
{
    return 		Tools->IsModified();
}
CCommandVar 	CommandExit(CCommandVar p1, CCommandVar p2)
{
    return 		Tools->IfModified();
}
CCommandVar 	CommandShowProperties(CCommandVar p1, CCommandVar p2)
{
	if(p1.IsString())
    {
    	xr_string SSS = p1;
    	Tools->ShowProperties(SSS.c_str());
	}else
    	Tools->ShowProperties(NULL);
    return				TRUE;
}
CCommandVar 	CommandUpdateProperties(CCommandVar p1, CCommandVar p2)
{
    Tools->UpdateProperties(p1);
    return				TRUE;
}
CCommandVar 	CommandRefreshProperties(CCommandVar p1, CCommandVar p2)
{
    Tools->RefreshProperties();
    return				TRUE;
}
CCommandVar 	CommandZoomExtents(CCommandVar p1, CCommandVar p2)
{
    Tools->ZoomObject	(p1);
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandToggleRenderWire(CCommandVar p1, CCommandVar p2)
{
    if (EDevice.dwFillMode!=D3DFILL_WIREFRAME)	EDevice.dwFillMode 	= D3DFILL_WIREFRAME;
    else 										EDevice.dwFillMode 	= D3DFILL_SOLID;
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandToggleSafeRect(CCommandVar p1, CCommandVar p2)
{
    psDeviceFlags.set	(rsDrawSafeRect,!psDeviceFlags.is(rsDrawSafeRect));
    ExecCommand			(COMMAND_RENDER_RESIZE);
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandToggleGrid(CCommandVar p1, CCommandVar p2)
{
    psDeviceFlags.set(rsDrawGrid,!psDeviceFlags.is(rsDrawGrid));
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandUpdateGrid(CCommandVar p1, CCommandVar p2)
{
    DU_impl.UpdateGrid		(EPrefs->grid_cell_count,EPrefs->grid_cell_size);
    UI->OutGridSize		();
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandGridNumberOfSlots(CCommandVar p1, CCommandVar p2)
{
    if (p1)				EPrefs->grid_cell_count += 2;
    else				EPrefs->grid_cell_count -= 2;
    ExecCommand			(COMMAND_UPDATE_GRID);
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandGridSlotSize(CCommandVar p1, CCommandVar p2)
{
    float step = 1.f;
    float val = EPrefs->grid_cell_size;
    if (p1){
        if (val<1) step/=10.f;
        EPrefs->grid_cell_size += step;
    }else{
        if (fsimilar(val,1.f)||(val<1)) step/=10.f;
        EPrefs->grid_cell_size -= step;
    }
    ExecCommand			(COMMAND_UPDATE_GRID);
    UI->RedrawScene		();
    return				TRUE;
}
CCommandVar 	CommandCreateSoundLib(CCommandVar p1, CCommandVar p2)
{
    SndLib		= xr_new<CSoundManager>();
    return				TRUE;
}
CCommandVar 	CommandMuteSound(CCommandVar p1, CCommandVar p2)
{
    SndLib->MuteSounds	(p1);
    return				TRUE;
}
CCommandVar CommandMoveCameraTo(CCommandVar p1, CCommandVar p2)
{
    Fvector pos					= EDevice.m_Camera.GetPosition();
    if (NumericVectorRun		("Move to",&pos,3))
        EDevice.m_Camera.Set		(EDevice.m_Camera.GetHPB(),pos);
    return 						TRUE;
}

CCommandVar 	ExecuteCommandList(LPCSTR text)
{
	CCommandVar	res		= TRUE;
    IReader F			((void*)text,xr_strlen(text));
    while (!F.eof()){
        xr_string 			line, cmd, params, sp1, sp2;
        F.r_string			(line);
        line				= _Trim(line);
        if (!line.empty()){
            if (line[0]==';' || (line[0]=='/'&&line[1]=='/')) continue;
            _GetItem			(line.c_str(),0,cmd,'(');
            _GetItem			(line.c_str(),1,params,'(');
            _GetItem			(params.c_str(),0,params,')');
            _GetItem			(params.c_str(),0,sp1,',');
            _GetItem			(params.c_str(),1,sp2,',');
            // parse cmd
            xr_string 			cmd_name, sub_cmd_name;
            _GetItem			(cmd.c_str(),0,cmd_name,'.');
            _GetItem			(cmd.c_str(),1,sub_cmd_name,'.');
            
            SECommand* CMD 		= FindCommandByName(cmd_name.c_str()); 
            if (CMD){
                SESubCommand* SUB= FindSubCommandByName(CMD,sub_cmd_name.c_str());
                if (!sub_cmd_name.empty()&&!SUB){
                    ELog.DlgMsg	(mtError,"Can't find sub-command: '%s'",sub_cmd_name.c_str());
	                res			= FALSE;
                    break;
                }
                // parse params
                CCommandVar p1,p2;
                if (SUB){
                	p1			= SUB->p0;
                    p2 			= SUB->p1;
                }
                ParseParam		(sp1,p1);
                ParseParam		(sp2,p2);
                // execute command
                if (FALSE==ExecCommand(CMD->idx,p1,p2)){	
                    ELog.DlgMsg	(mtError,"Can't execute command: '%s'",cmd.c_str());
	                res			= FALSE;
                    break;
                }
            }else{
                ELog.DlgMsg		(mtError,"Can't find command: '%s'",cmd.c_str());
                res				= FALSE;
                break;
            }
        }
    }
    return				res;
}

bool 	OnRunExecuteListClick(LPCSTR txt)
{
	ExecuteCommandList		(txt);
    return true;
}

CCommandVar 	CommandExecuteCommandList(CCommandVar _p1, CCommandVar _p2)
{
    xr_string 	cmds_text			= _p1;
    return  	ExecuteCommandList	(cmds_text.c_str());
}

bool __stdcall OnCloseCommandListEditor()
{
	frmEditCommandList	= 0;
    return 		true;
}

CCommandVar 	CommandEditCommandList(CCommandVar _p1, CCommandVar _p2)
{
    if (NULL==frmEditCommandList){
    	frmEditCommandList	= TfrmText::CreateForm(sCommandListText,"Execute command list",0,0,"Run",OnRunExecuteListClick,OnCloseCommandListEditor);
        return TRUE;
    }
    return FALSE;
}

CCommandVar 	CommandLogCommands(CCommandVar _p1, CCommandVar _p2)
{
	bAllowLogCommands	= _p1;
    return 				TRUE;
}
CCommandVar 	CommandRunMacro(CCommandVar p1, CCommandVar p2)
{
	xr_string fn;
	if (p1.IsString()){
		fn 				= xr_string(p1); 
	    IReader* F 		= FS.r_open(fn.c_str());
        if (NULL==F) F	= FS.r_open(_import_,fn.c_str());
        if (F){
            ExecCommand	(COMMAND_EXECUTE_COMMAND_LIST,xr_string((LPCSTR)F->pointer()));
            FS.r_close 	(F);
            return 	   	TRUE;
        }
    }else{
        SECommand* CMD 	= GetEditorCommands()[COMMAND_RUN_MACRO]; VERIFY(CMD);
        u32 num 		= p1; VERIFY(num<CMD->sub_commands.size());
        SESubCommand* SUB=CMD->sub_commands[num];
        fn 				= xr_string(SUB->p0);
        return ExecCommand(COMMAND_RUN_MACRO,fn,p2);
    }
    return 				FALSE;
}
CCommandVar 	CommandAssignMacro(CCommandVar p1, CCommandVar p2)
{
    xr_string fn 		= p2.IsString()?xr_string(p2):xr_string(""); 
    if (p2.IsString()){
        if (0==fn.find(FS.get_path(_import_)->m_Path))
            fn 			= xr_string(fn.c_str()+xr_strlen(FS.get_path(_import_)->m_Path));
	    ECommands[COMMAND_RUN_MACRO]->sub_commands[p1]->p0 = fn;
	    return 			TRUE;
    }else{
    	if (EFS.GetOpenName(_import_,fn,false,NULL,2))
        	return 		ExecCommand	(COMMAND_ASSIGN_MACRO,p1,fn);
    }
    return FALSE;
}

void TUI::RegisterCommands()
{
	REGISTER_CMD_S		(COMMAND_INITIALIZE,			CommandInitialize);
	REGISTER_CMD_S		(COMMAND_DESTROY,        		CommandDestroy);
	REGISTER_CMD_SE		(COMMAND_EXIT,               	"Exit",					CommandExit,		true);
	REGISTER_CMD_S		(COMMAND_QUIT,           		CommandQuit);
	REGISTER_CMD_SE		(COMMAND_EDITOR_PREF,    		"Editor Preference",	CommandEditorPrefs, false);


    REGISTER_CMD_SE	(COMMAND_SIMULATE,  			"Simulate",      		CommandSimulate, true);
    REGISTER_CMD_SE	(COMMAND_USE_SIMULATE_POSITIONS,"Use Simulate Positions",CommandUseSimulatePositions, true);

	REGISTER_SUB_CMD_SE	(COMMAND_CHANGE_ACTION,  		"Change Action",      	CommandChangeAction,false);
	   	APPEND_SUB_CMD	("Select",						etaSelect,	0);
    	APPEND_SUB_CMD	("Add",							etaAdd,		0);
    	APPEND_SUB_CMD	("Move",						etaMove,	0);
    	APPEND_SUB_CMD	("Rotate",						etaRotate,	0);
    	APPEND_SUB_CMD	("Scale",						etaScale,	0);
    REGISTER_SUB_CMD_END;
	REGISTER_SUB_CMD_SE	(COMMAND_CHANGE_AXIS,    		"Change Axis",			CommandChangeAxis,	false);
        APPEND_SUB_CMD	("X",					        etAxisX,	0);
        APPEND_SUB_CMD	("Y",					        etAxisY,	0);
        APPEND_SUB_CMD	("Z",					        etAxisZ,	0);
        APPEND_SUB_CMD	("ZX",					        etAxisZX,	0);
    REGISTER_SUB_CMD_END;
	REGISTER_CMD_S	    (COMMAND_SET_SETTINGS,			CommandSetSettings);
	REGISTER_CMD_S	    (COMMAND_SOUND_EDITOR,   		CommandSoundEditor);
	REGISTER_CMD_S	    (COMMAND_SYNC_SOUNDS,    		CommandSyncSounds);
    REGISTER_CMD_S	    (COMMAND_IMAGE_EDITOR,   		CommandImageEditor);    REGISTER_CMD_S	    (COMMAND_MINIMAP_EDITOR,   		CommandMinimapEditor);
	REGISTER_CMD_S	    (COMMAND_CHECK_TEXTURES,     	CommandCheckTextures);
	REGISTER_CMD_S	    (COMMAND_REFRESH_TEXTURES,   	CommandRefreshTextures);	
	REGISTER_CMD_S	    (COMMAND_RELOAD_TEXTURES,    	CommandReloadTextures);
	REGISTER_CMD_S	    (COMMAND_CHANGE_SNAP,        	CommandChangeSnap);
    REGISTER_CMD_S	    (COMMAND_UNLOAD_TEXTURES,    	CommandUnloadTextures);
    REGISTER_CMD_S	    (COMMAND_EVICT_OBJECTS,      	CommandEvictObjects);
    REGISTER_CMD_S	    (COMMAND_EVICT_TEXTURES,     	CommandEvictTextures);
    REGISTER_CMD_S	    (COMMAND_CHECK_MODIFIED,     	CommandCheckModified);
	REGISTER_CMD_SE	    (COMMAND_SHOW_PROPERTIES,    	"Show Properties",		CommandShowProperties, false);
	REGISTER_CMD_S	    (COMMAND_UPDATE_PROPERTIES,  	CommandUpdateProperties);
	REGISTER_CMD_S	    (COMMAND_REFRESH_PROPERTIES, 	CommandRefreshProperties);
    REGISTER_SUB_CMD_SE (COMMAND_ZOOM_EXTENTS,     		"Zoom",					CommandZoomExtents,false);
    	APPEND_SUB_CMD	("Extent",						0,0);
    	APPEND_SUB_CMD	("Selected",					1,0);
    REGISTER_SUB_CMD_END;
	REGISTER_CMD_SE	    (COMMAND_MOVE_CAMERA_TO,        "Scene\\Move Camera To",CommandMoveCameraTo,false);
    REGISTER_CMD_SE	    (COMMAND_TOGGLE_RENDER_WIRE,	"Toggle Wireframe",		CommandToggleRenderWire,			false);
    REGISTER_CMD_C	    (COMMAND_RENDER_FOCUS,       	this,TUI::CommandRenderFocus);
	REGISTER_CMD_CE	    (COMMAND_BREAK_LAST_OPERATION,	"Break Last Operation",	this,TUI::CommandBreakLastOperation,false);
    REGISTER_CMD_SE	    (COMMAND_TOGGLE_SAFE_RECT,   	"Toggle Safe Rect",		CommandToggleSafeRect,false);
	REGISTER_CMD_C	    (COMMAND_RENDER_RESIZE,      	this,TUI::CommandRenderResize);
    REGISTER_CMD_SE	    (COMMAND_TOGGLE_GRID,        	"Toggle Grid",			CommandToggleGrid,false);
	REGISTER_CMD_S	    (COMMAND_UPDATE_GRID,        	CommandUpdateGrid);
    REGISTER_CMD_S	    (COMMAND_GRID_NUMBER_OF_SLOTS,	CommandGridNumberOfSlots);
    REGISTER_SUB_CMD_SE (COMMAND_GRID_SLOT_SIZE,     	"Change Grid Size",		CommandGridSlotSize,false);
    	APPEND_SUB_CMD	("Decrease",					0,0);
    	APPEND_SUB_CMD	("Increase",					1,0);
    REGISTER_SUB_CMD_END;
    REGISTER_CMD_S	    (COMMAND_CREATE_SOUND_LIB,   	CommandCreateSoundLib);
    REGISTER_CMD_S	    (COMMAND_MUTE_SOUND,         	CommandMuteSound);
    REGISTER_CMD_S	    (COMMAND_EDIT_COMMAND_LIST, 	CommandEditCommandList);
    REGISTER_CMD_S	    (COMMAND_EXECUTE_COMMAND_LIST, 	CommandExecuteCommandList);
    REGISTER_CMD_S	    (COMMAND_LOG_COMMANDS, 			CommandLogCommands);
    REGISTER_SUB_CMD_SE (COMMAND_RUN_MACRO,     		"Run Macro",			CommandRunMacro,false);
    	APPEND_SUB_CMD	("Slot #1",						xr_string(""),0);
    	APPEND_SUB_CMD	("Slot #2",						xr_string(""),0);
    	APPEND_SUB_CMD	("Slot #3",						xr_string(""),0);
    	APPEND_SUB_CMD	("Slot #4",						xr_string(""),0);
    	APPEND_SUB_CMD	("Slot #5",						xr_string(""),0);
    	APPEND_SUB_CMD	("Slot #6",						xr_string(""),0);
    	APPEND_SUB_CMD	("Slot #7",						xr_string(""),0);
    	APPEND_SUB_CMD	("Slot #8",						xr_string(""),0);
    REGISTER_SUB_CMD_END;
    REGISTER_CMD_S	    (COMMAND_ASSIGN_MACRO, 			CommandAssignMacro);
}                                                                        

//---------------------------------------------------------------------------
bool TUI::ApplyShortCut(WORD Key, TShiftState Shift)
{
	VERIFY(m_bReady);

    if (ApplyGlobalShortCut(Key,Shift))	return true;

    if (Key==VK_ESCAPE){		ExecCommand	(COMMAND_CHANGE_ACTION, etaSelect); return true;}

    xr_shortcut SC; 
    SC.key						= Key;
    SC.ext.assign				(u8((Shift.Contains(ssShift)?xr_shortcut::flShift:0)|
    							 (Shift.Contains(ssCtrl) ?xr_shortcut::flCtrl:0)|
                                 (Shift.Contains(ssAlt)  ?xr_shortcut::flAlt:0)));
	SESubCommand* SUB 			= FindCommandByShortcut(SC);

    if (!SUB||SUB->parent->global_shortcut) 			return false;

    return 						ExecCommand(SC);
}
//---------------------------------------------------------------------------

bool TUI::ApplyGlobalShortCut(WORD Key, TShiftState Shift)
{
	VERIFY(m_bReady);

    if (Key==VK_OEM_3){			ExecCommand	(COMMAND_RENDER_FOCUS); return true;}

    xr_shortcut SC; 
    SC.key						= Key;
    SC.ext.assign				(u8((Shift.Contains(ssShift)?xr_shortcut::flShift:0)|
    							 (Shift.Contains(ssCtrl) ?xr_shortcut::flCtrl:0)|
                                 (Shift.Contains(ssAlt)  ?xr_shortcut::flAlt:0)));
	SESubCommand* SUB 			= FindCommandByShortcut(SC);

    if (!SUB||!SUB->parent->global_shortcut) 			return false;

    return						ExecCommand(SUB->parent->idx,SUB->p0,SUB->p1);
}
//---------------------------------------------------------------------------


