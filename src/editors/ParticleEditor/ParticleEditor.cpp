#include "stdafx.h"
#pragma hdrstop
#include "splash.h"
#include "LogForm.h"
#include "main.h"
#include "ui_particlemain.h"
#include "UI_ParticleTools.h"
//---------------------------------------------------------------------------
USEFORM("Splash.cpp", frmSplash);
USEFORM("main.cpp", frmMain);
USEFORM("BottomBar.cpp", fraBottomBar); /* TFrame: File Type */
USEFORM("TopBar.cpp", fraTopBar); /* TFrame: File Type */
USEFORM("LeftBar.cpp", fraLeftBar); /* TFrame: File Type */
USEFORM("ItemPropFormUnit.cpp", fmItemProp);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
//    try{
        if (!Application->Handle){ 
            Application->CreateHandle	(); 
            Application->Icon->Handle 	= LoadIcon(MainInstance, "MAINICON"); 
			Application->Title 			= "Loading...";
        } 
        frmSplash 				= xr_new<TfrmSplash>((TComponent*)0);
        frmSplash->Show			();
        frmSplash->Repaint		();
        frmSplash->SetStatus	("Core initializing...");

    	Core._initialize		("particle",ELogCallback);

        Application->Initialize	();
                                       
        frmSplash->SetStatus	("Loading...");

// startup create
        Tools					= xr_new<CParticleTool>();
        UI						= xr_new<CParticleMain>();
        UI->RegisterCommands	();

	Application->Title 		= UI->EditorDesc();
        TfrmLog::CreateLog		();

	Application->CreateForm(__classid(TfrmMain), &frmMain);
		frmMain->SetHInst		(hInst);

	xr_delete(frmSplash);

	Application->Run		();

        TfrmLog::DestroyLog		();

	UI->ClearCommands		();
        xr_delete			(Tools);
        xr_delete			(UI);

    	Core._destroy			();
//    }
//    catch (Exception &exception)
//    {
//           Application->ShowException(&exception);
//    }
    return 0;
}
//---------------------------------------------------------------------------




