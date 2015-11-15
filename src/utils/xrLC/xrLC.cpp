// xrLC.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "math.h"
#include "build.h"
#include "Common/FSMacros.hpp"
#include "utils/xrLC_Light/xrLC_GlobalData.h"
#include "utils/xrLCUtil/LevelCompilerLoggerWindow.hpp"

//#pragma comment(linker,"/STACK:0x800000,0x400000")
//#pragma comment(linker,"/HEAP:0x70000000,0x10000000")

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"IMAGEHLP.LIB")
#pragma comment(lib,"winmm.LIB")
#pragma comment(lib,"xrCDB.lib")
#pragma comment(lib,"FreeImage.lib")
#pragma comment(lib,"xrCore.lib")
#pragma comment(lib,"xrLC_Light.lib")
#pragma comment(lib, "xrLCUtil.lib")

#define PROTECTED_BUILD

#ifdef PROTECTED_BUILD
#   define TRIVIAL_ENCRYPTOR_ENCODER
#   define TRIVIAL_ENCRYPTOR_DECODER
#   include "xrEngine/trivial_encryptor.h"
#   undef TRIVIAL_ENCRYPTOR_ENCODER
#   undef TRIVIAL_ENCRYPTOR_DECODER
#endif // PROTECTED_BUILD

CBuild* pBuild      = NULL;
u32     version     = 0;
ILevelCompilerLogger& Logger = LevelCompilerLoggerWindow();

CThread::LogFunc ProxyMsg = cdecl_cast(
    [](const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger.clMsgV(format, args);
        va_end(args);    
    }
);

CThreadManager::ReportStatusFunc ProxyStatus = cdecl_cast(
    [](const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        Logger.StatusV(format, args);
        va_end(args);    
    }
);

CThreadManager::ReportProgressFunc ProxyProgress = cdecl_cast(
    [](float progress)
    { Logger.Progress(progress); }
);


static const char* h_str = 
    "The following keys are supported / required:\n"
    "-? or -h   == this help\n"
    "-o         == modify build options\n"
    "-nosun     == disable sun-lighting\n"
    "-f<NAME>   == compile level in GameData\\Levels\\<NAME>\\\n"
    "\n"
    "NOTE: The last key is required for any functionality\n";

void Help()
{
    MessageBox(0,h_str,"Command line options",MB_OK|MB_ICONINFORMATION);
}

typedef int __cdecl xrOptions(b_params* params, u32 version, bool bRunBuild);

void Startup(LPSTR     lpCmdLine)
{   
    create_global_data();
    char cmd[512];
    BOOL bModifyOptions     = FALSE;

    xr_strcpy(cmd,lpCmdLine);
    strlwr(cmd);
    if (strstr(cmd,"-?") || strstr(cmd,"-h"))           { Help(); return; }
    if (strstr(cmd,"-f")==0)                            { Help(); return; }
    if (strstr(cmd,"-o"))                               bModifyOptions  = TRUE;
    if (strstr(cmd,"-gi"))                              g_build_options.b_radiosity     = TRUE;
    if (strstr(cmd,"-noise"))                           g_build_options.b_noise         = TRUE;
    if (strstr(cmd,"-net"))                             g_build_options.b_net_light     = TRUE;
    VERIFY( lc_global_data() );
    lc_global_data()->b_nosun_set                       ( !!strstr(cmd,"-nosun") );
    //if (strstr(cmd,"-nosun"))                         b_nosun         = TRUE;
    char name[256];
    *name = 0;
    sscanf(strstr(cmd, "-f")+2, "%s", name);
    string256 temp;
    xr_sprintf(temp, "%s - Levels Compiler", name);
    Logger.Initialize(temp);
    // Faster FPU 
    SetPriorityClass        (GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
    /*
    u32 dwMin           = 1800*(1024*1024);
    u32 dwMax           = 1900*(1024*1024);
    if (0==SetProcessWorkingSetSize(GetCurrentProcess(),dwMin,dwMax))
    {
        clMsg("*** Failed to expand working set");
    };
    */
    
    // Load project
    string_path             prjName;
    FS.update_path          (prjName,"$game_levels$",strconcat(sizeof(prjName),prjName,name,"\\build.prj"));
    string256               phaseName;
    Logger.Phase(strconcat(sizeof(phaseName), phaseName, "Reading project [", name, "]..."));

    string256 inf;
    IReader*    F           = FS.r_open(prjName);
    if (NULL==F){
        xr_sprintf              (inf,"Build failed!\nCan't find level: '%s'",name);
        Logger.clMsg(inf);
        Logger.Failure(inf);
        Logger.Destroy();
        return;
    }

    // Version
    F->r_chunk          (EB_Version,&version);
    Logger.clMsg("version: %d", version);
    R_ASSERT(XRCL_CURRENT_VERSION==version);

    // Header
    b_params                Params;
    F->r_chunk          (EB_Parameters,&Params);

    // Show options if needed
    if (bModifyOptions)     
    {
        Logger.Phase("Project options...");
        HMODULE     L = LoadLibrary     ("xrLC_Options");
        void*       P = GetProcAddress  (L,"_frmScenePropertiesRun");
        R_ASSERT    (P);
        xrOptions*  O = (xrOptions*)P;
        int         R = O(&Params,version,false);
        FreeLibrary (L);
        if (R==2)   {
            ExitProcess(0);
        }
    }
    
    // Conversion
    Logger.Phase("Converting data structures...");
    pBuild                  = xr_new<CBuild>();
    pBuild->Load            (Params,*F);
    FS.r_close              (F);
    
    // Call for builder
    string_path             lfn;
    CTimer  dwStartupTime;  dwStartupTime.Start();
    FS.update_path          (lfn,_game_levels_,name);
    pBuild->Run             (lfn);
    xr_delete               (pBuild);

    // Show statistic
    u32 dwEndTime           = dwStartupTime.GetElapsed_ms();
    xr_sprintf                  (inf,"Time elapsed: %s",make_time(dwEndTime/1000).c_str());
    Logger.clMsg("Build succesful!\n%s", inf);
    if (!strstr(cmd,"-silent"))
        Logger.Success(inf);
    Logger.Destroy();
}

//typedef void DUMMY_STUFF (const void*,const u32&,void*);
//XRCORE_API DUMMY_STUFF    *g_temporary_stuff;
//XRCORE_API DUMMY_STUFF    *g_dummy_stuff;



int APIENTRY WinMain(HINSTANCE hInst,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
//  g_temporary_stuff   = &trivial_encryptor::decode;
//  g_dummy_stuff       = &trivial_encryptor::encode;

    // Initialize debugging
    Debug._initialize   (false);
    Core._initialize    ("xrLC");
    
    if(strstr(Core.Params,"-nosmg"))
        g_using_smooth_groups = false;

    Startup             (lpCmdLine);
    Core._destroy       ();
    
    return 0;
}
