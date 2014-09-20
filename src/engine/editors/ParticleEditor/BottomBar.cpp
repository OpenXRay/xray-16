//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "BottomBar.h"
#include "LogForm.h"
#include "ui_main.h"
#include "igame_persistent.h"
#include "environment.h"
#include "../../xrServerEntities/PropertiesListHelper.h"
#include "../xrEProps/PropertiesList.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ExtBtn"          
#pragma link "MxMenus"
#pragma link "mxPlacemnt"
#pragma resource "*.dfm"     
TfraBottomBar *fraBottomBar=0;
//---------------------------------------------------------------------------
__fastcall TfraBottomBar::TfraBottomBar(TComponent* Owner)
    : TFrame(Owner)
{
    DEFINE_INI(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ClickOptionsMenuItem(TObject *Sender)
{
    TMenuItem* mi = dynamic_cast<TMenuItem*>(Sender);
    if (mi){
        mi->Checked = !mi->Checked;
        if (mi==miDrawGrid)     			ExecCommand(COMMAND_TOGGLE_GRID);
        else if (mi==miRenderWithTextures)	psDeviceFlags.set(rsRenderTextures,mi->Checked);
        else if (mi==miMuteSounds)			psDeviceFlags.set(rsMuteSounds,mi->Checked);
        else if (mi==miLightScene)  		psDeviceFlags.set(rsLighting,mi->Checked);
        else if (mi==miRenderLinearFilter)	psDeviceFlags.set(rsFilterLinear,mi->Checked);
        else if (mi==miRenderEdgedFaces)	psDeviceFlags.set(rsEdgedFaces,mi->Checked);
        else if (mi==miFog)					psDeviceFlags.set(rsFog,mi->Checked);
        else if (mi==miRealTime)			psDeviceFlags.set(rsRenderRealTime,mi->Checked);
        else if (mi==miDrawSafeRect)		ExecCommand(COMMAND_TOGGLE_SAFE_RECT);
        else if (mi==miRenderFillPoint)		EDevice.dwFillMode 	= D3DFILL_POINT;
        else if (mi==miRenderFillWireframe)	EDevice.dwFillMode 	= D3DFILL_WIREFRAME;
        else if (mi==miRenderFillSolid)		EDevice.dwFillMode 	= D3DFILL_SOLID;
        else if (mi==miRenderShadeFlat)		EDevice.dwShadeMode	= D3DSHADE_FLAT;
        else if (mi==miRenderShadeGouraud)	EDevice.dwShadeMode	= D3DSHADE_GOURAUD;
        else if (mi==miRenderHWTransform){	HW.Caps.bForceGPU_SW = !mi->Checked; UI->Resize(); }
    }
    UI->RedrawScene();
    ExecCommand(COMMAND_UPDATE_TOOLBAR);
}
//---------------------------------------------------------------------------
void __fastcall TfraBottomBar::QualityClick(TObject *Sender)
{
    UI->SetRenderQuality((float)(((TMenuItem*)Sender)->Tag)/100);
    ((TMenuItem*)Sender)->Checked = true;
    UI->Resize();
}
//---------------------------------------------------------------------------
void __fastcall TfraBottomBar::fsStorageRestorePlacement(TObject *Sender)
{
    // fill mode
    if (miRenderFillPoint->Checked) 		EDevice.dwFillMode=D3DFILL_POINT;
    else if (miRenderFillWireframe->Checked)EDevice.dwFillMode=D3DFILL_WIREFRAME;
	else if (miRenderFillSolid->Checked)	EDevice.dwFillMode=D3DFILL_SOLID;
    // shade mode
	if (miRenderShadeFlat->Checked)			EDevice.dwShadeMode=D3DSHADE_FLAT;
    else if (miRenderShadeGouraud->Checked)	EDevice.dwShadeMode=D3DSHADE_GOURAUD;
    // hw transform
    HW.Caps.bForceGPU_SW 					= !miRenderHWTransform->Checked;

    // quality
    if 		(N200->Checked)	QualityClick(N200);
    else if (N150->Checked)	QualityClick(N150);
    else if (N125->Checked)	QualityClick(N125);
    else if (N100->Checked)	QualityClick(N100);
    else if (N75->Checked)	QualityClick(N75);
    else if (N50->Checked)	QualityClick(N50);
    else if (N25->Checked)	QualityClick(N25);

    // setup menu
    miWeather->Clear();

    TMenuItem* mi	= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption 	= "none";
    mi->OnClick 	= miWeatherClick;
    mi->Tag			= -1;
    mi->Checked		= true;
    mi->RadioItem	= true;
    miWeather->Add	(mi);
    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption 	= "-";
    miWeather->Add	(mi);
/*
    // append weathers
    CEnvironment::EnvsMapIt _I=g_pGamePersistent->Environment().WeatherCycles.begin();
    CEnvironment::EnvsMapIt _E=g_pGamePersistent->Environment().WeatherCycles.end();
    for (; _I!=_E; _I++){
        mi				= xr_new<TMenuItem>((TComponent*)0);
        mi->Caption 	= *_I->first;
        mi->OnClick 	= miWeatherClick;
	    mi->RadioItem	= true;
        miWeather->Add	(mi);
    }
*/
    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption 	= "-";
    miWeather->Add	(mi);
    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption 	= "Reload";
    mi->OnClick 	= miWeatherClick;
    mi->Tag			= -2;
    miWeather->Add	(mi);

    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption 	= "-";
    miWeather->Add	(mi);
    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption 	= "Properties...";
    mi->OnClick 	= miWeatherClick;
    mi->Tag			= -3;
    miWeather->Add	(mi);
    
    psDeviceFlags.set	(rsEnvironment,FALSE);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ebLogClick(TObject *Sender)
{
	TfrmLog::ChangeVisible();
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ebStopClick(TObject *Sender)
{
	ExecCommand(COMMAND_BREAK_LAST_OPERATION);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ebStatClick(TObject *Sender)
{
	psDeviceFlags.set(rsStatistic,!psDeviceFlags.is(rsStatistic));
    UI->RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ebOptionsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    POINT pt;
    GetCursorPos(&pt);
    pmOptions->Popup(pt.x,pt.y);
    TExtBtn* btn = dynamic_cast<TExtBtn*>(Sender); VERIFY(btn); btn->MouseManualUp();
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::pmOptionsPopup(TObject *Sender)
{
    miRenderWithTextures->Checked	= psDeviceFlags.is(rsRenderTextures);
    miLightScene->Checked			= psDeviceFlags.is(rsLighting);
    miMuteSounds->Checked			= psDeviceFlags.is(rsMuteSounds);
    miRenderLinearFilter->Checked	= psDeviceFlags.is(rsFilterLinear);
    miRenderEdgedFaces->Checked		= psDeviceFlags.is(rsEdgedFaces);
    miRealTime->Checked				= psDeviceFlags.is(rsRenderRealTime);
    miFog->Checked					= psDeviceFlags.is(rsFog);
    miDrawGrid->Checked				= psDeviceFlags.is(rsDrawGrid);
    miDrawSafeRect->Checked			= psDeviceFlags.is(rsDrawSafeRect);
    for(int i=0; i < miWeather->Count; ++i)
    {
        TMenuItem* mi = miWeather->Items[i];
        BOOL bch;
        bch  = ((EPrefs->sWeather.size()) && (0==stricmp(mi->Caption.c_str(), EPrefs->sWeather.c_str()))) ||
               (mi->Caption=="none" && EPrefs->sWeather.size()==0) ;
        mi->Checked                 = bch;
    }

}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::miWeatherClick(TObject *Sender)
{
/*
    TMenuItem* mi = dynamic_cast<TMenuItem*>(Sender);
    if (mi){
    	if (mi->Tag==0){
		    psDeviceFlags.set	(rsEnvironment,TRUE);
    	    g_pGamePersistent->Environment().SetWeather(mi->Caption.c_str());
            EPrefs->sWeather = mi->Caption.c_str();
        	mi->Checked = !mi->Checked;
        }else if (mi->Tag==-1){
		    psDeviceFlags.set	(rsEnvironment,FALSE);
    	    g_pGamePersistent->Environment().SetWeather(0);
            EPrefs->sWeather = "";
        	mi->Checked = !mi->Checked;
        }else if (mi->Tag==-2){
        	Engine.ReloadSettings();
    	    g_pGamePersistent->Environment().ED_Reload();
        }else if (mi->Tag==-3){
            TProperties* P 		= TProperties::CreateModalForm("Weather properties");
			CEnvironment& env	= g_pGamePersistent->Environment();
            PropItemVec items;   
            float ft=env.ed_from_time,tt=env.ed_to_time,sp=env.fTimeFactor;
            PHelper().CreateTime	(items,"From Time", 	&ft);
            PHelper().CreateTime	(items,"To Time",   	&tt);
            PHelper().CreateFloat	(items,"Speed",			&sp, 		1.f,10000.f,1.f,1);
            
            P->AssignItems		(items);
            if (mrOk==P->ShowPropertiesModal()){
                env.ed_from_time	= ft;
                env.ed_to_time		= tt;
                env.fTimeFactor		= sp;
            }
            TProperties::DestroyForm(P);
        }
    }
*/    
}
//---------------------------------------------------------------------------

void TfraBottomBar::RedrawBar()
{
	SPBItem* pbi 	= UI->ProgressLast();
	if (pbi){
        AnsiString 	txt;
        float 		p,m;
        pbi->GetInfo(txt,p,m);
        // status line
        if (paStatus->Caption!=txt){
	        paStatus->Caption		= txt;
    	    paStatus->Repaint		();
        }
        // progress
    	int val = fis_zero(m)?0:(int)((p/m)*100);
        if (val!=cgProgress->Progress){
			cgProgress->Progress	= val;
	        cgProgress->Repaint	();
        }
    	if (false==cgProgress->Visible) 
        	cgProgress->Visible 	= true;
    }else{
    	if (cgProgress->Visible){
            // status line
            paStatus->Caption		= "";
            paStatus->Repaint		();
	        // progress
        	cgProgress->Visible 	= false;
	        cgProgress->Progress	= 0;
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::MacroAssignClick(TObject *Sender)
{
	ExecCommand(COMMAND_ASSIGN_MACRO,((TMenuItem*)Sender)->Tag,0);
}
//---------------------------------------------------------------------------
void __fastcall TfraBottomBar::MacroClearClick(TObject *Sender)
{
	ExecCommand(COMMAND_ASSIGN_MACRO,((TMenuItem*)Sender)->Tag,xr_string(""));
}
//---------------------------------------------------------------------------
void __fastcall TfraBottomBar::MacroExecuteClick(TObject *Sender)
{
	ExecCommand(COMMAND_RUN_MACRO,((TMenuItem*)Sender)->Tag,0);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::MacroLogCommandsClick(TObject *Sender)
{
	ExecCommand(COMMAND_LOG_COMMANDS,((TMenuItem*)Sender)->Checked,0);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::MacroEditCommandListClick(TObject *Sender)
{
	ExecCommand(COMMAND_EDIT_COMMAND_LIST);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ebMacroMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
	SECommand* CMD 	= GetEditorCommands()[COMMAND_RUN_MACRO]; VERIFY(CMD);
    // fill macroses
	pmMacro->Items->Clear();
    TMenuItem* mi;
    for (u32 k=0; k<CMD->sub_commands.size(); ++k){
        SESubCommand* SUB   = CMD->sub_commands[k];
        BOOL bValid		= !xr_string(SUB->p0).empty();
        mi				= xr_new<TMenuItem>((TComponent*)0);
        mi->Caption		= AnsiString().sprintf("%d: %s",k+1,bValid?xr_string(SUB->p0).c_str():"<empty>");
        TMenuItem* e	= xr_new<TMenuItem>((TComponent*)0);
        e->Caption 		= "Execute";
        e->OnClick		= MacroExecuteClick;
        e->Enabled		= bValid;
        e->Tag			= k;
        e->ShortCut		= SUB->shortcut.hotkey;
        TMenuItem* a	= xr_new<TMenuItem>((TComponent*)0);
        a->Caption 		= "Assign";
        a->OnClick		= MacroAssignClick;
        a->Tag			= k;
        TMenuItem* c	= xr_new<TMenuItem>((TComponent*)0);
        c->Caption 		= "Clear";
        c->OnClick		= MacroClearClick;
        c->Tag			= k;
		mi->Add			(e);		
		mi->Add			(a);		
		mi->Add			(c);		
        pmMacro->Items->Add(mi);
    }
    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption		= "-";
    pmMacro->Items->Add(mi);
    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption		= "Edit Command List...";
    mi->OnClick		= MacroEditCommandListClick;
    pmMacro->Items->Add(mi);
    mi				= xr_new<TMenuItem>((TComponent*)0);
    mi->Caption		= "Log Commands";
    mi->AutoCheck	= true;
    mi->Checked		= AllowLogCommands();
    mi->OnClick		= MacroLogCommandsClick;
    pmMacro->Items->Add(mi);
	// popup menu    
    POINT pt;
    GetCursorPos	(&pt);
    pmMacro->Popup	(pt.x,pt.y);
    TExtBtn* btn 	= dynamic_cast<TExtBtn*>(Sender); VERIFY(btn); btn->MouseManualUp();
}
//---------------------------------------------------------------------------

