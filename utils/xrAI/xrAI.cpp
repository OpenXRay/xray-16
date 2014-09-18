// xrAI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "../../xrcore/xr_ini.h"
#include "process.h"
#include "xrAI.h"

#include "xr_graph_merge.h"
#include "game_spawn_constructor.h"
#include "xrCrossTable.h"
//#include "path_test.h"
#include "game_graph_builder.h"
#include <mmsystem.h>
#include "spawn_patcher.h"

#pragma comment(linker,"/STACK:0x800000,0x400000")

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"IMAGEHLP.LIB")
#pragma comment(lib,"winmm.LIB")
#pragma comment(lib,"xrcdb.LIB")
#pragma comment(lib,"MagicFM.LIB")
#pragma comment(lib,"xrCore.LIB")

extern LPCSTR LEVEL_GRAPH_NAME;

extern void	xrCompiler			(LPCSTR name, bool draft_mode, bool pure_covers, LPCSTR out_name);
extern void logThread			(void *dummy);
extern volatile BOOL bClose;
extern void test_smooth_path	(LPCSTR name);
extern void test_hierarchy		(LPCSTR name);
extern void	xrConvertMaps		();
extern void	test_goap			();
extern void	smart_cover			(LPCSTR name);
extern void	verify_level_graph	(LPCSTR name, bool verbose);
//extern void connectivity_test	(LPCSTR);
extern void compare_graphs		(LPCSTR level_name);
extern void test_levels			();

static const char* h_str = 
	"The following keys are supported / required:\n"
	"-? or -h   == this help\n"
	"-f<NAME>   == compile level in gamedata/levels/<NAME>/\n"
	"-o         == modify build options\n"
	"-s         == build game spawn data\n"
	"\n"
	"NOTE: The last key is required for any functionality\n";

void Help()
{	MessageBox(0,h_str,"Command line options",MB_OK|MB_ICONINFORMATION); }

string_path INI_FILE;

extern  HWND logWindow;

extern LPCSTR GAME_CONFIG;

extern void clear_temp_folder	();

void execute	(LPSTR cmd)
{
	// Load project
	string4096 name;
	name[0]=0; 
	if (strstr(cmd,"-f"))
		sscanf	(strstr(cmd,"-f")+2,"%s",name);
	else
		if (strstr(cmd,"-s"))
			sscanf	(strstr(cmd,"-s")+2,"%s",name);
		else
			if (strstr(cmd,"-t"))
				sscanf	(strstr(cmd,"-t")+2,"%s",name);
			else
				if (strstr(cmd,"-verify"))
					sscanf	(strstr(cmd,"-verify")+xr_strlen("-verify"),"%s",name);

	if (xr_strlen(name))
		xr_strcat			(name,"\\");

	string_path			prjName;
	prjName				[0] = 0;
	bool				can_use_name = false;
	if (xr_strlen(name) < sizeof(string_path)) {
		can_use_name	= true;
		FS.update_path	(prjName,"$game_levels$",name);
	}

	FS.update_path		(INI_FILE,"$game_config$",GAME_CONFIG);
	
	if (strstr(cmd,"-f")) {
		R_ASSERT3		(can_use_name,"Too big level name",name);
		
		char			*output = strstr(cmd,"-out");
		string256		temp0;
		if (output) {
			output		+= xr_strlen("-out");
			sscanf		(output,"%s",temp0);
			_TrimLeft	(temp0);
			output		= temp0;
		}
		else
			output		= (pstr)LEVEL_GRAPH_NAME;

		xrCompiler		(prjName,!!strstr(cmd,"-draft"),!!strstr(cmd,"-pure_covers"),output);
	}
	else {
		if (strstr(cmd,"-s")) {
			if (xr_strlen(name))
				name[xr_strlen(name) - 1] = 0;
			char				*output = strstr(cmd,"-out");
			string256			temp0, temp1;
			if (output) {
				output			+= xr_strlen("-out");
				sscanf			(output,"%s",temp0);
				_TrimLeft		(temp0);
				output			= temp0;
			}
			char				*start = strstr(cmd,"-start");
			if (start) {
				start			+= xr_strlen("-start");
				sscanf			(start,"%s",temp1);
				_TrimLeft		(temp1);
				start			= temp1;
			}
			char				*no_separator_check = strstr(cmd,"-no_separator_check");
			clear_temp_folder	();
			CGameSpawnConstructor(name,output,start,!!no_separator_check);
		}
		else
			if (strstr(cmd,"-verify")) {
				R_ASSERT3			(can_use_name,"Too big level name",name);
				verify_level_graph	(prjName,!strstr(cmd,"-noverbose"));
			}
	}
}

void Startup(LPSTR     lpCmdLine)
{
	string4096 cmd;
	BOOL bModifyOptions		= FALSE;

	xr_strcpy(cmd,lpCmdLine);
	strlwr(cmd);
	if (strstr(cmd,"-?") || strstr(cmd,"-h"))			{ Help(); return; }
	if ((strstr(cmd,"-f")==0) && (strstr(cmd,"-g")==0) && (strstr(cmd,"-m")==0) && (strstr(cmd,"-s")==0) && (strstr(cmd,"-t")==0) && (strstr(cmd,"-c")==0) && (strstr(cmd,"-verify")==0) && (strstr(cmd,"-patch")==0))	{ Help(); return; }
	if (strstr(cmd,"-o"))								bModifyOptions = TRUE;

	// Give a LOG-thread a chance to startup
	InitCommonControls	();
	Sleep				(150);
	thread_spawn		(logThread,	"log-update", 1024*1024,0);
	while				(!logWindow)	Sleep		(150);
	
	u32					dwStartupTime	= timeGetTime();
	execute				(cmd);
	// Show statistic
	char				stats[256];
	extern				std::string make_time(u32 sec);
	extern				HWND logWindow;
	u32					dwEndTime = timeGetTime();
	xr_sprintf				(stats,"Time elapsed: %s",make_time((dwEndTime-dwStartupTime)/1000).c_str());
	MessageBox			(logWindow,stats,"Congratulation!",MB_OK|MB_ICONINFORMATION);

	bClose				= TRUE;
	FlushLog			();
	Sleep				(500);
}

#include "factory_api.h"

#include "quadtree.h"

Factory_Create	*create_entity	= 0;
Factory_Destroy	*destroy_entity	= 0;

void buffer_vector_test		();

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	Debug._initialize		(false);
	Core._initialize		("xrai",0);

	buffer_vector_test		();

	HMODULE					hFactory;
	LPCSTR					g_name	= "xrSE_Factory.dll";
	Log						("Loading DLL:",g_name);
	hFactory				= LoadLibrary	(g_name);
	if (0==hFactory)		R_CHK			(GetLastError());
	R_ASSERT2				(hFactory,"Factory DLL raised exception during loading or there is no factory DLL at all");

	create_entity			= (Factory_Create*)		GetProcAddress(hFactory,"_create_entity@4");	R_ASSERT(create_entity);
	destroy_entity			= (Factory_Destroy*)	GetProcAddress(hFactory,"_destroy_entity@4");	R_ASSERT(destroy_entity);

	Startup					(lpCmdLine);

	FreeLibrary				(hFactory);

	Core._destroy			();

	return					(0);
}