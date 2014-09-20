#if defined(_PSP)
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../gsCommon.h"

#include <kernel.h>
#include <stdio.h>
#include <string.h>

/********  THIS FILE IS FOR USE BY THE GAMESPY SAMPLE APPLICATIONS ********/

// Portions taken from Sony PSP http_get sample have been modified to suite the 
// sample/demonstration of the Gamespy SDKs
// This code is not intended to be used in a shipping title  
//    (e.g. You should have your own network setup code which supports other configurations)


// Code modified from http_get used to export information about this module
SCE_MODULE_INFO(PspCommon, 0, 1, 1);

// Paths setup for specific modules and libraries used by the Tool
#define DEVKIT_PATH "host0:/usr/local/devkit/"
#define MODULE_PATH DEVKIT_PATH "module/"
#define PSPNET_AP_DIALOG_DUMMY_PRX MODULE_PATH "pspnet_ap_dialog_dummy.prx"

// Memory pool size for network stack and other stacks
#define PSPNET_POOLSIZE (256 * 1024)
#define CALLOUT_TPL 32
#define NETINTR_TPL 32
#define SCE_APCTL_HANDLER_STACKSIZE (1024 * 1)
#define SCE_APCTL_STACKSIZE (SCE_NET_APCTL_LEAST_STACK_SIZE + SCE_APCTL_HANDLER_STACKSIZE)
#define SCE_APCTL_PRIO 40

#define AP_DIALOG_DUMMY_WAIT_TIME (1000 * 1000)

#define PSPNET_APCTLHDLR        0x80
#define PSPNET_APDUMDLG_STARTED 0x40
#define PSPNET_CONNECTED        0x20
// If necessary, use this to select a specific router by SSID.
#ifndef AUTO_SELECT_ROUTER 
#define YOUR_WIRELESS_ROUTER_NAME "PubServ DLink"
#endif

// function prototypes
int gsiPspLoadRequiredModules(void);
int gsiPspUnloadRequiredModules(void);

// static vars used for disconnect warnings and setting up network connection
static int gDisconnected = 0;
static struct SceNetApDialogDummyParam gApDialogDummyParam;

// Globals
// the sce kernel detects this global variable and sets the heapsize from it
// see devkit\src\crt0\kernel_bridge.c(232-253)
int sce_newlib_heap_kb_size = 1000; 
SceUID gPspnetApDialogDummyModid = 0;
int gPspnetApctlHandlerId = -1;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Used to start needed kernel model
static SceUID gsiPspLoadModule(const char *path)
{
	SceUID modid = 0;
	int ret = 0, mresult;

	ret = sceKernelLoadModule(path, 0, NULL);
	if(ret < 0)
	{
		printf("sceKernelLoadModule() failed. ret = 0x%x\n", ret);
		return ret;
	}
	modid = ret;

	ret = sceKernelStartModule(modid, 0, 0, &mresult, NULL);
	if(ret < 0)
	{
		printf("sceKernelStartModule() failed. ret = 0x%x\n", ret);
		ret = sceKernelUnloadModule(modid);
		if (ret < 0)
			printf("sceKernelUnloadModule() failed. ret = 0x%x\n", ret);
		return ret;
	}
	ret = modid;

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Used to stop kernel model
static int gsiPspUnloadModule(SceUID modid)
{
	int ret = 0;

	ret = sceKernelStopModule(modid, 0, NULL, NULL, NULL);
	if (ret < 0) 
	{
		printf("sceKernelStopModule() failed. ret = 0x%x\n", ret);
		return ret;
	}

	ret = sceKernelUnloadModule(modid);
	if (ret < 0) 
	{
		printf("sceKernelUnloadModule() failed. ret = 0x%x\n", ret);
		return ret;
	}

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// used to start up network library modules
int gsiPspLoadRequiredModules()
{
	int ret = 0;

	ret = sceUtilityLoadModule(SCE_UTILITY_MODULE_NET_COMMON);
	if(ret < 0)
	{
		printf("sceUtilityLoadModule(SCE_UTILITY_MODULE_NET_COMMON) failed. ret = 0x%x\n", ret);
		gsiPspUnloadRequiredModules();
		return ret;
	}

	ret = sceUtilityLoadModule(SCE_UTILITY_MODULE_NET_INET);
	if(ret < 0)
	{
		printf("sceUtilityLoadModule(SCE_UTILITY_MODULE_NET_INET) failed. ret = 0x%x\n", ret);
		gsiPspUnloadRequiredModules();		
		return ret;
	}

	ret = gsiPspLoadModule(PSPNET_AP_DIALOG_DUMMY_PRX);
	if(ret < 0)
	{
		printf("load_module %s failed. ret = 0x%x\n",PSPNET_AP_DIALOG_DUMMY_PRX, ret);
		gsiPspUnloadRequiredModules();
		return ret;
	}
	gPspnetApDialogDummyModid = ret;

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Used to shutdown network library modules
int gsiPspUnloadRequiredModules()
{
	int ret = 0;


	ret = gsiPspUnloadModule(gPspnetApDialogDummyModid);
	if (ret < 0) 
	{
		printf("unload_module() failed. ret = 0x%x\n", ret);
		return ret;
	}

	ret = sceUtilityUnloadModule(SCE_UTILITY_MODULE_NET_INET);
	if (ret < 0) 
	{
		printf("sceUtilityUnloadModule(SCE_UTILITY_MODULE_NET_INET) failed. ret = 0x%x\n", ret);
	}

	ret = sceUtilityUnloadModule(SCE_UTILITY_MODULE_NET_COMMON);
	if (ret < 0) 
	{
		printf("sceUtilityUnloadModule(SCE_UTILITY_MODULE_NET_COMMON) failed. ret = 0x%x\n", ret);
	}
	return ret;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// AP network connection handler used to detect if psp was disconnected
void gsiPspApctlHandler(int prev_state, int new_state, int event, int error_code, void *arg)
{
	(void)arg;

	if(new_state == SCE_NET_APCTL_STATE_Disconnected)
	{
	    if(event == SCE_NET_APCTL_EVENT_DISCONNECT_REQ)
			gDisconnected = 1;
		if(prev_state == SCE_NET_APCTL_STATE_IPObtained &&
		    event == SCE_NET_APCTL_EVENT_ERROR)
			printf("Apctl error happened, error = 0x%x\n", error_code);
	}
}

int gsiPspnetDisconnect()
{	
	int ret;
	
	gDisconnected = 0;
	
	ret = sceNetApctlDisconnect();
	if(ret < 0)
	{
		printf("sceNetApctlDisconnect() failed. ret = 0x%x\n", ret);
		return ret;
	}

	while(gDisconnected == 0)
		sceKernelDelayThread(AP_DIALOG_DUMMY_WAIT_TIME);

	sceNetApDialogDummyTerm();
	if (gPspnetApctlHandlerId >= 0)
	{
		sceNetApctlDelHandler(gPspnetApctlHandlerId);
	}

	return ret;
}

void gsiPspnetStop()
{
	// turn off all services if they are available
	sceNetApctlTerm();

	sceNetResolverTerm();
			
	sceNetInetTerm();
			
	sceNetTerm();
}

// Error Handler used to stop network libraries in case of failure
void gsiPspnetErrorHandler(int shutDownServices)
{

	if (shutDownServices & PSPNET_CONNECTED)
		gsiPspnetDisconnect();

	if (shutDownServices & PSPNET_APDUMDLG_STARTED)
		sceNetApDialogDummyTerm();
	
	if (shutDownServices & PSPNET_APCTLHDLR)
		if(gPspnetApctlHandlerId >= 0)
			sceNetApctlDelHandler(gPspnetApctlHandlerId);
	
	gsiPspnetStop();	
}

//
int gsiPspStartNetworkModules()
{
	struct SceNetApDialogDummyStateInfo ap_dialog_dummy_state;
	union SceNetApctlInfo apctl_info;
	int ret;
	// error bit starts at 1 and is shifted left with one added
	int serviceToShutDown = 0;
	
	ret = sceNetInit(PSPNET_POOLSIZE, CALLOUT_TPL, 0,
	    NETINTR_TPL, 0);
	if(ret < 0)
	{
		printf("sceNetInit() failed. ret = 0x%x\n", ret);
		return ret;
	}

	
	ret = sceNetInetInit();
	if(ret < 0)
	{
		printf("sceNetInetInit() failed. ret = 0x%x\n", ret);
		gsiPspnetStop();
		return ret;
	}

	
	ret = sceNetResolverInit();
	if(ret < 0)
	{
		printf("sceNetResolverInit() failed. ret = 0x%x\n", ret);
		gsiPspnetStop();
		return ret;
	}

	
	ret = sceNetApctlInit(SCE_APCTL_STACKSIZE, SCE_APCTL_PRIO);
	if(ret < 0)
	{
		printf("sceNetApctlInit() failed. ret = 0x%x\n", ret);
		gsiPspnetStop();
		return ret;
	}

	
	ret = sceNetApctlAddHandler(gsiPspApctlHandler, NULL);
	if(ret < 0)
	{
		printf("sceNetApctlAddHandler() failed. ret = 0x%x\n", ret);
		gsiPspnetStop();
		return ret;
	}
	gPspnetApctlHandlerId = ret;

	serviceToShutDown = serviceToShutDown | PSPNET_APCTLHDLR;
	ret = sceNetApDialogDummyInit();
	if(ret < 0)
	{
		printf("sceNetApDialogDummyInit() failed. ret = 0x%x\n", ret);
		gsiPspnetErrorHandler(serviceToShutDown);   // called with 31
		return ret;
	}
	
	serviceToShutDown = serviceToShutDown | PSPNET_APDUMDLG_STARTED;
	/* check Wireless LAN switch */
	ret = sceWlanGetSwitchState();
	if(ret == SCE_WLAN_SWITCH_STATE_OFF){
		printf("Wireless LAN switch has been turned off.\n");
		gsiPspnetErrorHandler(serviceToShutDown);   // called with 63
		return ret;
	}

	
	memset(&gApDialogDummyParam, 0, sizeof(gApDialogDummyParam));
#ifndef AUTO_SELECT_ROUTER
	strcpy(gApDialogDummyParam.ssid, YOUR_WIRELESS_ROUTER_NAME);
#endif
	ret = sceNetApDialogDummyConnect(&gApDialogDummyParam);
	if(ret < 0)
	{
		printf("sceNetApDialogDummyConnect() failed. ret = 0x%x\n", ret);
		gsiPspnetErrorHandler(serviceToShutDown);   // called with 63
		return ret;
	}

	while(1){
		ret = sceNetApDialogDummyGetState(&ap_dialog_dummy_state);
		if(ret == 0){
			if(ap_dialog_dummy_state.state == SceNetApDialogDummyState_Connected ||
		    	ap_dialog_dummy_state.state == SceNetApDialogDummyState_Disconnected)
				break;
		}
		sceKernelDelayThread(AP_DIALOG_DUMMY_WAIT_TIME);
	}

	if(ap_dialog_dummy_state.state == SceNetApDialogDummyState_Disconnected)
	{
		printf("failed to Join or Get IP addr. error = 0x%x\n", ap_dialog_dummy_state.error_code);
		gsiPspnetErrorHandler(serviceToShutDown);   // called with 63
		return ap_dialog_dummy_state.error_code;
	}

	serviceToShutDown = serviceToShutDown | PSPNET_CONNECTED;
	ret = sceNetApctlGetInfo(SCE_NET_APCTL_INFO_IP_ADDRESS, &apctl_info);
	if(ret < 0)
	{
		printf("sceNetApctlGetInfo() failed. ret = 0x%x\n", ret);
		gsiPspnetErrorHandler(serviceToShutDown);   // called with 127
		return ret;
	}
	printf("obtained IP addr: %s\n", apctl_info.ip_address);
	return ret;
}

// sample entry point
extern int test_main(int argc, char ** argp); 

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Common PSP entry point
int main(int argc, char** argp)
{
	int ret = 0;
	
	
	// Load the modules required to do basic TCP/IP networking 
	ret = gsiPspLoadRequiredModules();
	if(ret < 0)
	{
		printf("gsiPspLoadRequiredModules failed. See previous return value. return = 0x%x\n",
				ret);
		return SCE_KERNEL_EXIT_SUCCESS;
	}


	// Start up PSPNET libraries
	ret = gsiPspStartNetworkModules();
	if (ret < 0)
	{
		printf("gsiPspStartNetworkModules failed See previous return value. return = 0x%x\n", ret);
		return ret;
	}


	///////////////////////////////////
	// Call the application entry point
	ret = test_main(argc, argp);

	ret = gsiPspnetDisconnect();
	if (ret < 0)
	{
		printf("gsiPspnetDisconnect failed See previous return value. return = 0x%x\n", ret);
		return ret;
	}
	
	// Shut down the PSPNET network library
	gsiPspnetStop();

	ret = gsiPspUnloadRequiredModules();
	if(ret < 0)
		return SCE_KERNEL_EXIT_SUCCESS;

	return SCE_KERNEL_EXIT_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // _PSP only
