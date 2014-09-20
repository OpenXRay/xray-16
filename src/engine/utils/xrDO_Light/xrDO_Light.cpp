// xrAI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "process.h"

#include "../xrlc_light/xrlc_light.h"
//#pragma comment(linker,"/STACK:0x800000,0x400000")

#pragma comment(lib,"comctl32.lib")
//#pragma comment(lib,"d3dx9.lib")
//#pragma comment(lib,"IMAGEHLP.LIB")
#pragma comment(lib,"winmm.LIB")
#pragma comment(lib,"xrCDB.lib")
#pragma comment(lib,"xrCore.lib")
#pragma comment(lib,"xrLC_Light.lib")
//#pragma comment(lib,"FreeImage.lib")



extern void logThread			(void *dummy);
extern volatile BOOL bClose;

static const char* h_str = 
	"The following keys are supported / required:\n"
	"-? or -h	== this help\n"
	"-f<NAME>	== compile level in gamedata\\levels\\<NAME>\\\n"
	"-o			== modify build options\n"
	"\n"
	"NOTE: The last key is required for any functionality\n";

void Help()
{	MessageBox(0,h_str,"Command line options",MB_OK|MB_ICONINFORMATION); }

void Startup(LPSTR     lpCmdLine)
{
	char cmd[512],name[256];
//	BOOL bModifyOptions		= FALSE;
	bool bNet				= false;
	xr_strcpy(cmd,lpCmdLine);
	strlwr(cmd);
	if (strstr(cmd,"-?") || strstr(cmd,"-h"))			{ Help(); return; }
	if (strstr(cmd,"-f")==0)							{ Help(); return; }
//	if (strstr(cmd,"-o"))								bModifyOptions = TRUE;
	if ( strstr(cmd,"-net") )						
														bNet = true;
	// Give a LOG-thread a chance to startup
	InitCommonControls	();
	thread_spawn		(logThread,	"log-update", 1024*1024,0);
	Sleep				(150);
	
	// Load project
	name[0]=0; sscanf	(strstr(cmd,"-f")+2,"%s",name);

	extern  HWND logWindow;
	string256			temp;
	xr_sprintf			(temp, "%s - Detail Compiler", name);
	SetWindowText		(logWindow, temp);

	//FS.update_path	(name,"$game_levels$",name);
	FS.get_path			("$level$")->_set	(name);

	CTimer				dwStartupTime; dwStartupTime.Start();

	xrCompileDO			(bNet);

	// Show statistic
	char	stats[256];
	extern	std::string make_time(u32 sec);
	xr_sprintf				(stats,"Time elapsed: %s",make_time((dwStartupTime.GetElapsed_ms())/1000).c_str());

	if (!strstr(cmd,"-silent"))
		MessageBox		(logWindow,stats,"Congratulation!",MB_OK|MB_ICONINFORMATION);

	bClose				= TRUE;
	Sleep				(500);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// Initialize debugging
	Debug._initialize	(false);
	Core._initialize	("xrDO");
	Startup				(lpCmdLine);
	
	return 0;
}
