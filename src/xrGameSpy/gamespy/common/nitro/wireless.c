#include "../nonport.h"
#include "wireless.h"
#include "screen.h"
#include "menu.h"
#include "backup.h"
#include "touch.h"

#define USE_DHCP
#define SUPPORT_SAVED_CONFIGS

#define AUTO_CONNECT
//static u8 MyBSSID[IW_BSSID_SIZE] = { 0x00, 0x0f, 0x66, 0x82, 0x38, 0x70 };
static u8 MyBSSID[IW_BSSID_SIZE] = { 0x00, 0x09, 0x5b, 0x3f, 0x4b, 0xfb };
static u8 MyWEP[IW_WEP_SIZE] = { WM_WEPMODE_NO };

/************************************************************************/
/* Wireless Menu                                                        */
/************************************************************************/

static const char mscUseSelectedConfiguration[] = "Use Selected Configuration";
static const char mscCreateNewConfiguration[] = "Create New Configuration";
static const char mscRenameSelectedConfiguration[] = "Rename Selected Configuration";
static const char mscDeleteSelectedConfiguration[] = "Delete Selected Configuration";
static const char mscExit[] = "Exit";
static const char mscYes[] = "Yes";
static const char mscNo[] = "No";
static const char mscOK[] = "OK";
static const char mscCancel[] = "Cancel";
static const char mscRename[] = "Rename";
static const char mscConnectToSelectedNetwork[] = "Connect to Selected Network";
static const char mscViewNetworkInformation[] = "View Network Information";
static const char mscBack[] = "Back";
static const char mscEnter[] = "Enter";
static const char mscUseKeyType[] = "Use Key Type";
static const char mscRetry[] = "Retry";
static const char mscContinue[] = "Continue";
static const char mscSave[] = "Save";
static const char mscDontSave[] = "Don't Save";

static void InitializingWirelessInit(void);
static void InitializingWirelessThink(void);
static void InitializingWirelessChose(const char * choice);

static MenuScreenConfiguration msInitializingWireless =
{
	"Initializing Wireless",
	{
		{ mscCancel }
	},
	InitializingWirelessInit,
	InitializingWirelessChose,
	InitializingWirelessThink,
	SCREEN_OPTION_ANIMATED
};

static void FailedToInitializeWirelessInit(void);
static void FailedToInitializeWirelessChose(const char * choice);

static MenuScreenConfiguration msFailedToInitializeWireless =
{
	"Failed to Initialize Wireless",
	{
		{ mscOK }
	},
	FailedToInitializeWirelessInit,
	FailedToInitializeWirelessChose,
	NULL,
	SCREEN_OPTION_EXTRAS_CENTERED
};

static void SelectNetworkConfigurationInit(void);
static void SelectNetworkConfigurationChose(const char * choice);

static MenuScreenConfiguration msSelectNetworkConfiguration =
{
	"Select Network Configuration",
	{
		{ mscUseSelectedConfiguration, CHOICE_OPTION_NEEDS_LIST_SELECTION },
		{ mscCreateNewConfiguration },
		{ mscRenameSelectedConfiguration, CHOICE_OPTION_NEEDS_LIST_SELECTION },
		{ mscDeleteSelectedConfiguration, CHOICE_OPTION_NEEDS_LIST_SELECTION },
		{ mscExit }
	},
	SelectNetworkConfigurationInit,
	SelectNetworkConfigurationChose,
	NULL,
	SCREEN_OPTION_LIST
};

static void RenameConfigurationInit(void);
static void RenameConfigurationChose(const char * choice);

static MenuScreenConfiguration msRenameConfiguration =
{
	"Rename Configuration",
	{
		{ mscRename },
		{ mscCancel }
	},
	RenameConfigurationInit,
	RenameConfigurationChose,
	NULL,
	SCREEN_OPTION_KEYBOARD | SCREEN_OPTION_EXTRAS_CENTERED
};

static void ConfirmDeleteInit(void);
static void ConfirmDeleteChose(const char * choice);

static MenuScreenConfiguration msConfirmDelete =
{
	"Confirm Delete",
	{
		{ mscYes },
		{ mscNo }
	},
	ConfirmDeleteInit,
	ConfirmDeleteChose,
	NULL,
	SCREEN_OPTION_EXTRAS_CENTERED
};

static void ConfigurationDeletedChose(const char * choice);

static MenuScreenConfiguration msConfigurationDeleted =
{
	"Configuration Deleted",
	{
		{ mscOK }
	},
	NULL,
	ConfigurationDeletedChose
};

static void SearchingForNetworksInit(void);
static void SearchingForNetworksThink(void);
static void SearchingForNetworksChose(const char * choice);

static MenuScreenConfiguration msSearchingForNetworks =
{
	"Searching for Networks",
	{
		{ mscConnectToSelectedNetwork, CHOICE_OPTION_NEEDS_LIST_SELECTION },
		{ mscViewNetworkInformation, CHOICE_OPTION_NEEDS_LIST_SELECTION },
		{ mscCancel }
	},
	SearchingForNetworksInit,
	SearchingForNetworksChose,
	SearchingForNetworksThink,
	SCREEN_OPTION_ANIMATED | SCREEN_OPTION_LIST
};

static void SearchingForSavedNetworkInit(void);
static void SearchingForSavedNetworkThink(void);
static void SearchingForSavedNetworkChose(const char * choice);
static MenuScreenConfiguration msSearchingForSavedNetwork =
{
	"Searching for Saved Network",
	{
		{ mscCancel }
	},
	SearchingForSavedNetworkInit,
	SearchingForSavedNetworkChose,
	SearchingForSavedNetworkThink,
	SCREEN_OPTION_ANIMATED
};

static void SearchFailedInit(void);
static void SearchFailedChose(const char * choice);

static MenuScreenConfiguration msSearchFailed =
{
	"Search Failed",
	{
		{ mscOK }
	},
	SearchFailedInit,
	SearchFailedChose,
	NULL,
	SCREEN_OPTION_EXTRAS_CENTERED
};

static void NetworkInformationInit(void);
static void NetworkInformationChose(const char * choice);

static MenuScreenConfiguration msNetworkInformation =
{
	"Network Information",
	{
		{ mscBack }
	},
	NetworkInformationInit,
	NetworkInformationChose,
	NULL
};

static void SelectSecurityTypeInit(void);
static void SelectSecurityTypeChose(const char * choice);

static MenuScreenConfiguration msSelectSecurityType =
{
	"Select Security Type",
	{
		{ mscUseKeyType, CHOICE_OPTION_NEEDS_LIST_SELECTION },
		{ mscBack }
	},
	SelectSecurityTypeInit,
	SelectSecurityTypeChose,
	NULL,
	SCREEN_OPTION_LIST
};

static void EnterSecurityKeyInit(void);
static void EnterSecurityKeyChose(const char * choice);

static MenuScreenConfiguration msEnterSecurityKey =
{
	"Enter Security (WEP) Key",
	{
		{ mscEnter },
		{ mscBack }
	},
	EnterSecurityKeyInit,
	EnterSecurityKeyChose,
	NULL,
	SCREEN_OPTION_KEYBOARD
};

static void BadSecurityKeyEnteredInit(void);
static void BadSecurityKeyEnteredChose(const char * choice);

static MenuScreenConfiguration msBadSecurityKeyEntered =
{
	"Bad Security Key Entered",
	{
		{ mscBack }
	},
	BadSecurityKeyEnteredInit,
	BadSecurityKeyEnteredChose,
	NULL,
	SCREEN_OPTION_EXTRAS_CENTERED
};

static void ConnectingToNetworkInit(void);
static void ConnectingToNetworkChose(const char * choice);
static void ConnectingToNetworkThink(void);

static MenuScreenConfiguration msConnectingToNetwork =
{
	"Connecting to Network",
	{
		{ mscCancel }
	},
	ConnectingToNetworkInit,
	ConnectingToNetworkChose,
	ConnectingToNetworkThink,
	SCREEN_OPTION_ANIMATED
};

static void FailedToConnectToNetworkInit(void);
static void FailedToConnectToNetworkChose(const char * choice);

static MenuScreenConfiguration msFailedToConnectToNetwork =
{
	"Failed to Connect to Network",
	{
		{ mscBack }
	},
	FailedToConnectToNetworkInit,
	FailedToConnectToNetworkChose,
	NULL,
	SCREEN_OPTION_EXTRAS_CENTERED
};

static void ConnectingToInternetInit(void);
static void ConnectingToInternetChose(const char * choice);
static void ConnectingToInternetThink(void);

static MenuScreenConfiguration msConnectingToInternet =
{
	"Connecting to Internet",
	{
		{ mscCancel }
	},
	ConnectingToInternetInit,
	ConnectingToInternetChose,
	ConnectingToInternetThink,
	SCREEN_OPTION_ANIMATED
};

static void FailedToConnectToInternetInit(void);
static void FailedToConnectToInternetChose(const char * choice);

static MenuScreenConfiguration msFailedToConnectToInternet =
{
	"Failed to Connect to Internet",
	{
		{ mscBack }
	},
	FailedToConnectToInternetInit,
	FailedToConnectToInternetChose,
	NULL,
	SCREEN_OPTION_EXTRAS_CENTERED
};

static void ConnectedToInternetInit(void);
static void ConnectedToInternetChose(const char * choice);

static MenuScreenConfiguration msConnectedToInternet =
{
	"Connected to Internet",
	{
		{ mscContinue }
	},
	ConnectedToInternetInit,
	ConnectedToInternetChose
};

static void SaveNetworkConfigurationInit(void);
static void SaveNetworkConfigurationChose(const char * choice);

static MenuScreenConfiguration msSaveNetworkConfiguration =
{
	"Save Network Configuration",
	{
		{ mscSave},
		{ mscDontSave }
	},
	SaveNetworkConfigurationInit,
	SaveNetworkConfigurationChose,
	NULL,
	SCREEN_OPTION_KEYBOARD
};

static void ConfigurationSavedChose(const char * choice);

static MenuScreenConfiguration msConfigurationSaved =
{
	"Configuration Saved",
	{
		{ mscContinue }
	},
	NULL,
	ConfigurationSavedChose
};

static MenuScreenConfiguration msCancelled =
{
	"Wireless Setup Cancelled"
};

static MenuScreenConfiguration msFailed =
{
	"Wireless Setup Failed"
};

/************************************************************************/
/* Wireless Funcs                                                       */
/************************************************************************/

#define APLIST_COUNT 8

#define MAX_SAVED_CONFIGS 3
#define MAX_CONFIG_NAME_LEN 32

#define BSSDESC_MAX 16

static const char MagicString[] = "GOA-SAVED-CONFIGS";
static const size_t MagicStringLen = sizeof(MagicString);

typedef struct SavedConfig
{
	char name[MAX_CONFIG_NAME_LEN + 1];
	u8 bssid[IW_BSSID_SIZE];
	u8 wep[IW_WEP_SIZE];
} SavedConfig;

typedef struct SavedFile
{
	char magic[MagicStringLen];
	u8 numConfigs;
	SavedConfig configs[MAX_SAVED_CONFIGS];
} SavedFile;

static SavedFile SavedSettings;

static u8 IWBuffer[IW_WORK_SIZE] ATTRIBUTE_ALIGN(32);

static u8 BSSID[IW_BSSID_SIZE];

static u8 WEPAny[IW_WEP_SIZE];
static char WEPEntered[256];
static u8 WEPKey[IW_WEP_SIZE];

static unsigned long BSSDescBuffer[IW_BSS_SIZE * BSSDESC_MAX / 4];

static IWConfig WirelessConfig =
{
	3,
	BSSDescBuffer,
	IW_BSS_SIZE * BSSDESC_MAX,
	0
};

static WMBssDesc APList[APLIST_COUNT];
static int APIndex;
static int WEPType;

static char FailureReason[MAX_EXTRA_TEXT_STRING_LEN + 1];

static int SelectedConfig;

static BOOL NewConfig;

static gsi_bool KeepPolling;

static void * soAlloc(u32 name, s32 size)
{
	OSIntrMode nOSIntrMode;
	void * alloc = NULL;

	GSI_UNUSED(name);

	nOSIntrMode = OS_DisableInterrupts();
	alloc = OS_Alloc((u32)size);
	OS_RestoreInterrupts(nOSIntrMode);

	return alloc;
}

static void soFree(u32 name, void * ptr, s32 size)
{
	OSIntrMode nOSIntrMode;

	GSI_UNUSED(name);
	GSI_UNUSED(size);

	nOSIntrMode = OS_DisableInterrupts();
	OS_Free(ptr);
	OS_RestoreInterrupts(nOSIntrMode);
}

SOConfig SocketsConfig =
{
	SOC_VENDOR_NINTENDO,     // vendor
	SOC_VERSION,             // version

	soAlloc,                // alloc
	soFree,                 // free

#if defined(USE_DHCP)
	SOC_FLAG_DHCP,           // flag
	SOC_HtoNl(SOC_INADDR_ANY), // addr
	SOC_HtoNl(SOC_INADDR_ANY), // netmask
	SOC_HtoNl(SOC_INADDR_ANY), // router
	SOC_HtoNl(SOC_INADDR_ANY), // dns1
	SOC_HtoNl(SOC_INADDR_ANY), // dns2
#else
	0,                      // flag
	SOC_HtoNl(0xC0A8000C),   // addr         192.168.  0. 12
	SOC_HtoNl(0xFFFFFF00),   // netmask      255.255.255.  0
	SOC_HtoNl(0xC0A80001),   // router       192.168.  0.  1
	SOC_HtoNl(0xC0A80001),   // dns1         192.168.  0.  1
	SOC_HtoNl(SOC_INADDR_ANY), // dns2
#endif
	4096,                   // timeWaitBuffer
	4096,                   // reassemblyBuffer
	0,                      // maximum transmission unit size

	// TCP
	0,                      // default TCP receive window size (default 2 x MSS)
	0,                      // default TCP total retransmit timeout value (default 100 sec)

	// PPP
	NULL,
	NULL,

	// PPPoE
	NULL,

	// DHCP
	"NintendoDS",           // DHCP host name
	50,                    // TCP total retransmit times (default 4)

	// UDP
	0,                      // default UDP send buffer size (default 1472)
	0                       // defualt UDP receive buffer size (default 4416)
};

static const char * IWResultToString(int result)
{
	if(result == IW_RESULT_SUCCESS)
		return "success";
    if(result == IW_RESULT_FAILURE)
		return "failure";
    if(result == IW_RESULT_PROGRESS)
		return "progress";
    if(result == IW_RESULT_ACCEPT)
		return "accept";
    if(result == IW_RESULT_REJECT)
		return "reject";
    if(result == IW_RESULT_WMDISABLE)
		return "wmdisable";
    if(result == IW_RESULT_MEMORYERROR)
		return "memoryerror";
    if(result == IW_RESULT_FATALERROR)
		return "fatalerror";
	return "unknown result";
}

static const char * IWNotifyToString(int notify)
{
    if(notify == IW_NOTIFY_COMMON)
		return "common";
    if(notify == IW_NOTIFY_STARTUP)
		return "startup";
    if(notify == IW_NOTIFY_CLEANUP)
		return "cleanup";
    if(notify == IW_NOTIFY_SEARCH_START)
		return "search_start";
    if(notify == IW_NOTIFY_SEARCH_END)
		return "search_end";
    if(notify == IW_NOTIFY_CONNECT)
		return "connect";
    if(notify == IW_NOTIFY_DISCONNECT)
		return "disconnect";
    if(notify == IW_NOTIFY_FOUND)
		return "found";
    if(notify == IW_NOTIFY_BEACON_LOST)
		return "beacon_lost";
    if(notify == IW_NOTIFY_FATALERROR)
		return "fatalerror";
	return "unknown notify";
}

static const char * IWPhaseToString(int phase)
{
    if(phase == IW_PHASE_NULL)
		return "null";
    if(phase == IW_PHASE_WAIT)
		return "wait";
    if(phase == IW_PHASE_WAIT_IDLE)
		return "wait_idle";
    if(phase == IW_PHASE_IDLE)
		return "idle";
    if(phase == IW_PHASE_IDLE_WAIT)
		return "idle_wait";
    if(phase == IW_PHASE_IDLE_SCAN)
		return "idle_scan";
    if(phase == IW_PHASE_SCAN)
		return "scan";
    if(phase == IW_PHASE_SCAN_IDLE)
		return "scan_idle";
    if(phase == IW_PHASE_IDLE_LINK)
		return "idle_link";
    if(phase == IW_PHASE_LINK)
		return "link";
    if(phase == IW_PHASE_LINK_IDLE)
		return "link_idle";
    if(phase == IW_PHASE_FATALERROR)
		return "fatalerror";
	return "unknown phase";
}

static void IW_Callback(IWNotice * arg)
{
	GSI_UNUSED(arg);
	OS_TPrintf("Notify: %s | Result: %s\n", IWNotifyToString(arg->notify), IWResultToString(arg->result));
}

static gsi_bool IW_CheckResult(int result, const char * funcName)
{
	assert(funcName);

	OS_Printf("%s: %s\n", funcName, IWResultToString(result));
	switch(result)
	{
	case IW_RESULT_SUCCESS:
	case IW_RESULT_PROGRESS:
	case IW_RESULT_ACCEPT:
		return gsi_true;
	case IW_RESULT_FAILURE:
	case IW_RESULT_REJECT:
		OS_Printf("  IW_Phase = %s\n", IWPhaseToString(IW_GetPhase()));
	case IW_RESULT_WMDISABLE:
	case IW_RESULT_MEMORYERROR:
	case IW_RESULT_FATALERROR:
	default:
		snprintf(FailureReason, sizeof(FailureReason), "%s result: %s", funcName, IWResultToString(result));
		return gsi_false;
	}
}

static gsi_bool IW_Poll(int polling, int success, const char * funcName, gsi_bool * keepPolling)
{
	int phase;

	assert(funcName);

	phase = IW_GetPhase();
	if(phase == polling)
	{
		if(keepPolling)
			*keepPolling = gsi_true;
		return gsi_true;
	}

	if(keepPolling)
		*keepPolling = gsi_false;

	if(phase == success)
		return gsi_true;

	snprintf(FailureReason, sizeof(FailureReason), "%s phase: %s", funcName, IWPhaseToString(phase));
	return gsi_false;
}

static void LoadSettings(void)
{
	if(BackupExists())
	{
		// load the saved settings from the backup
		ReadFromBackup(&SavedSettings, sizeof(SavedSettings));
	}
	else
	{
		// use a default when there isn't a backup device
		// this makes debugging with the dev kit easier
		SavedConfig config;
		strcpy(config.name, "Default WEP");
		memcpy(config.bssid, MyBSSID, sizeof(MyBSSID));
		memcpy(config.wep, MyWEP, sizeof(MyWEP));

		memcpy(SavedSettings.magic, MagicString, MagicStringLen);
		SavedSettings.numConfigs = 1;
		memcpy(&SavedSettings.configs[0], &config, sizeof(SavedConfig));
	}

	if(memcmp(SavedSettings.magic, MagicString, MagicStringLen) != 0)
		SavedSettings.numConfigs = 0;

	SavedSettings.numConfigs = (u8)min(SavedSettings.numConfigs, MAX_SAVED_CONFIGS);
}

static void SaveSettings(void)
{
	if(BackupExists())
	{
		memcpy(SavedSettings.magic, MagicString, MagicStringLen);
		WriteToBackup(&SavedSettings, sizeof(SavedSettings));
	}
}

void WirelessInit(void)
{
	// load wireless settings
	LoadSettings();

	// start the menu
	StartMenuScreen(&msInitializingWireless);
}

void WirelessCleanup(void)
{
	int result;

	SOC_Cleanup();

	result = IW_Disconnect();
	IW_CheckResult(result, "IW_Disconnect");

	do
	{
		if(!IW_Poll(IW_PHASE_LINK_IDLE, IW_PHASE_IDLE, "IW_Disconnect", &KeepPolling))
			OS_TPrintf("Failuring waiting for wait phase\n");
	} while(KeepPolling);

	result = IW_Cleanup();
	IW_CheckResult(result, "IW_Cleanup");

	do
	{
		if(!IW_Poll(IW_PHASE_IDLE_WAIT, IW_PHASE_WAIT, "IW_Cleanup", &KeepPolling))
			OS_TPrintf("Failuring waiting for wait phase\n");
	} while(KeepPolling);

	result = IW_Exit();
	IW_CheckResult(result, "IW_Exit");
}

/************************************************************************/
/* Wireless Menu Funcs                                                  */
/************************************************************************/

static void InitializingWirelessInit(void)
{
	int result;

	// init the wireless library
	result = IW_Init(IWBuffer, sizeof(IWBuffer));
	if(!IW_CheckResult(result, "IW_Init"))
	{
		SetNextMenuScreen(&msFailedToInitializeWireless);
		return;
	}

	// startup wireless
	result = IW_Startup(&WirelessConfig, IW_Callback);
	if(!IW_CheckResult(result, "IW_Startup"))
	{
		SetNextMenuScreen(&msFailedToInitializeWireless);
		return;
	}

	KeepPolling = gsi_true;
}

static void InitializingWirelessChose(const char * choice)
{
	if(choice == mscCancel)
		SetNextMenuScreen(&msCancelled);
}

static void InitializingWirelessThink(void)
{
	// poll the phase
	if(KeepPolling)
	{
		if(!IW_Poll(IW_PHASE_WAIT_IDLE, IW_PHASE_IDLE, "IW_Startup", &KeepPolling))
		{
			SetNextMenuScreen(&msFailedToInitializeWireless);
			return;
		}
		if(KeepPolling)
			return;
	}

#if defined(AUTO_CONNECT)
	{
		memcpy(WEPKey, MyWEP, sizeof(WEPKey));
		memcpy(BSSID, MyBSSID, WM_SIZE_BSSID);
		NewConfig = FALSE;
		SetNextMenuScreen(&msSearchingForSavedNetwork);
		return;
	}
#endif

	// we're done with the startup
#if defined(SUPPORT_SAVED_CONFIGS)
	if(SavedSettings.numConfigs > 0)
		SetNextMenuScreen(&msSelectNetworkConfiguration);
	else
#endif
		SetNextMenuScreen(&msSearchingForNetworks);
}

static void FailedToInitializeWirelessInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->extraText[0], FailureReason, MAX_EXTRA_TEXT_STRING_LEN + 1);
	screen->extraText[0][MAX_EXTRA_TEXT_STRING_LEN] = '\0';	
}

static void FailedToInitializeWirelessChose(const char * choice)
{
	if(choice == mscOK)
		SetNextMenuScreen(&msFailed);
}

static void SelectNetworkConfigurationInit(void)
{
	MenuScreen * screen = GetMenuScreen();
	int i;

	for(i = 0 ; i < SavedSettings.numConfigs ; i++)
	{
		strncpy(screen->list[i], SavedSettings.configs[i].name, MAX_LIST_STRING_LEN);
		screen->list[i][MAX_LIST_STRING_LEN - 1] = '\0';
	}
}

static void SelectNetworkConfigurationChose(const char * choice)
{
	MenuScreen * screen = GetMenuScreen();

	SelectedConfig = screen->listSelection;

	if(choice == mscUseSelectedConfiguration)
	{
		memcpy(WEPKey, SavedSettings.configs[SelectedConfig].wep, sizeof(WEPKey));
		memcpy(BSSID, SavedSettings.configs[SelectedConfig].bssid, WM_SIZE_BSSID);
		NewConfig = FALSE;
		//SetNextMenuScreen(&msConnectingToNetwork);
		SetNextMenuScreen(&msSearchingForSavedNetwork);
	}
	else if(choice == mscCreateNewConfiguration)
	{
		SetNextMenuScreen(&msSearchingForNetworks);
	}
	else if(choice == mscRenameSelectedConfiguration)
	{
		SetNextMenuScreen(&msRenameConfiguration);
	}
	else if(choice == mscDeleteSelectedConfiguration)
	{
		SetNextMenuScreen(&msConfirmDelete);
	}
	else if(choice == mscExit)
	{
		SetNextMenuScreen(&msCancelled);
	}
}

static void RenameConfigurationInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->extraText[0], SavedSettings.configs[SelectedConfig].name, MAX_EXTRA_TEXT_STRING_LEN);
	screen->extraText[0][MAX_EXTRA_TEXT_STRING_LEN - 1] = '\0';

	strncpy(screen->keyboardText, SavedSettings.configs[SelectedConfig].name, MAX_KEYBOARD_TEXT_LEN);
	screen->keyboardText[MAX_KEYBOARD_TEXT_LEN - 1] = '\0';
}

static void RenameConfigurationChose(const char * choice)
{
	MenuScreen * screen = GetMenuScreen();

	if(choice == mscRename)
	{
		strncpy(SavedSettings.configs[SelectedConfig].name, screen->keyboardText, MAX_CONFIG_NAME_LEN);
		SavedSettings.configs[SelectedConfig].name[MAX_CONFIG_NAME_LEN - 1] = '\0';

		SaveSettings();

		SetNextMenuScreen(&msSelectNetworkConfiguration);
	}
	else if(choice == mscCancel)
	{
		SetNextMenuScreen(&msSelectNetworkConfiguration);
	}
}

static void ConfirmDeleteInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->extraText[0], SavedSettings.configs[SelectedConfig].name, MAX_EXTRA_TEXT_STRING_LEN);
	screen->extraText[0][MAX_EXTRA_TEXT_STRING_LEN - 1] = '\0';
}

static void ConfirmDeleteChose(const char * choice)
{
	int i;

	if(choice == mscYes)
	{
		SavedSettings.numConfigs--;
		for(i = SelectedConfig ; i < SavedSettings.numConfigs ; i++)
			memcpy(&SavedSettings.configs[i], &SavedSettings.configs[i + 1], sizeof(SavedConfig));

		SaveSettings();

		SetNextMenuScreen(&msConfigurationDeleted);
	}
	else if(choice == mscNo)
	{
		SetNextMenuScreen(&msSelectNetworkConfiguration);
	}
}

static void ConfigurationDeletedChose(const char * choice)
{
	if(choice == mscOK)
	{
		if(SavedSettings.numConfigs > 0)
			SetNextMenuScreen(&msSelectNetworkConfiguration);
		else
			SetNextMenuScreen(&msSearchingForNetworks);
	}
}

static void SearchingForNetworksInit(void)
{
	int result;

	// search for access points
	result = IW_Search(IW_BSSID_ANY, IW_ESSID_ANY, IW_OPT_CHANNEL_ALL|IW_OPT_SCANTYPE_PASSIVE);
	if(!IW_CheckResult(result, "IW_Search"))
	{
		SetNextMenuScreen(&msSearchFailed);
		return;
	}

	KeepPolling = gsi_true;
}

static void SearchingForNetworksChose(const char * choice)
{
	MenuScreen * screen = GetMenuScreen();

	APIndex = screen->listSelection;

	if(choice == mscViewNetworkInformation)
	{
		SetNextMenuScreen(&msNetworkInformation);
		return;
	}

	// we're connecting or cancelling, so stop searching
	IW_Search(NULL, NULL, 0);

	if(choice == mscConnectToSelectedNetwork)
	{
		SetNextMenuScreen(&msSelectSecurityType);
	}
	else if(choice == mscCancel)
	{
#if defined(SUPPORT_SAVED_CONFIGS)
		if(SavedSettings.numConfigs > 0)
			SetNextMenuScreen(&msSelectNetworkConfiguration);
		else
#endif
			SetNextMenuScreen(&msCancelled);
	}
}

static void SearchingForNetworksThink(void)
{
	MenuScreen * screen;
	int i;
	int count;
	WMBssDesc * desc;

	if(KeepPolling)
	{
		// poll the search
		if(!IW_Poll(IW_PHASE_IDLE_SCAN, IW_PHASE_SCAN, "IW_Search", &KeepPolling))
		{
			IW_Search(NULL, NULL, 0);
			SetNextMenuScreen(&msSearchFailed);
			return;
		}
		if(KeepPolling)
			return;
	}

	// get the screen
	screen = GetMenuScreen();

	// lock the AP descriptions
	IW_LockBssDesc(1);

	// get a count
	count = IW_CountBssDesc();
	count = min(count, APLIST_COUNT);

	// copy the access points
	for(i = 0 ; i < count ; i++)
	{
		desc = IW_PointBssDesc(i);
		if(!desc)
		{
			count = i;
			break;
		}

		memcpy(&APList[i], desc, sizeof(WMBssDesc));
	}

	// unlock the APs
	IW_LockBssDesc(0);

	// fill the screen list
	for(i = 0 ; i < MAX_LIST_STRINGS ; i++)
	{
		if(i < count)
		{
			if(APList[i].ssidLength > 0)
				snprintf(screen->list[i], MAX_LIST_STRING_LEN, "%s", APList[i].ssid);
			else
				strcpy(screen->list[i], "[no name]");
		}
		else
		{
			screen->list[i][0] = '\0';
		}
	}
}

static void SearchingForSavedNetworkInit(void)
{
	int result;

	// search for access points
	result = IW_Search(BSSID, IW_ESSID_ANY, IW_OPT_CHANNEL_ALL|IW_OPT_SCANTYPE_PASSIVE);
	if(!IW_CheckResult(result, "IW_Search"))
	{
		SetNextMenuScreen(&msSearchFailed);
		return;
	}

	KeepPolling = gsi_true;
}

static void SearchingForSavedNetworkChose(const char * choice)
{
	if(choice == mscCancel)
	{
		// stop searching
		IW_Search(NULL, NULL, 0);

		SetNextMenuScreen(&msSelectNetworkConfiguration);
	}
}

static void SearchingForSavedNetworkThink(void)
{
	int count;
	WMBssDesc * desc = NULL;

	if(KeepPolling)
	{
		// poll the search
		if(!IW_Poll(IW_PHASE_IDLE_SCAN, IW_PHASE_SCAN, "IW_Search", &KeepPolling))
		{
			IW_Search(NULL, NULL, 0);
			SetNextMenuScreen(&msSearchFailed);
			return;
		}
		if(KeepPolling)
			return;
	}
	// lock the AP descriptions
	IW_LockBssDesc(1);

	// get a count
	count = IW_CountBssDesc();

	// if we found it, copy it
	if(count > 0)
	{
		desc = IW_PointBssDesc(0);
		if(desc)
		{
			memcpy(&APList[0], desc, sizeof(WMBssDesc));
			APIndex = 0;
		}
	}

	// unlock the APs
	IW_LockBssDesc(0);

	// check if we found it
	if(desc)
	{
		// stop searching
		IW_Search(NULL, NULL, 0);

		// connect
		SetNextMenuScreen(&msConnectingToNetwork);
	}
}

static void SearchFailedInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->extraText[0], FailureReason, MAX_EXTRA_TEXT_STRING_LEN + 1);
	screen->extraText[0][MAX_EXTRA_TEXT_STRING_LEN] = '\0';	
}

static void SearchFailedChose(const char * choice)
{
	if(choice == mscOK)
	{
#if defined(SUPPORT_SAVED_CONFIGS)
		if(SavedSettings.numConfigs > 0)
			SetNextMenuScreen(&msSelectNetworkConfiguration);
		else
#endif
			SetNextMenuScreen(&msFailed);
	}
}

static void NetworkInformationInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	// BSSID / MAC
	snprintf(screen->extraText[0], MAX_EXTRA_TEXT_STRING_LEN,
		"  MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
		APList[APIndex].bssid[0],
		APList[APIndex].bssid[1],
		APList[APIndex].bssid[2],
		APList[APIndex].bssid[3],
		APList[APIndex].bssid[4],
		APList[APIndex].bssid[5]);

	// channel
	snprintf(screen->extraText[1], MAX_EXTRA_TEXT_STRING_LEN,
		"  Channel: %d", 
		APList[APIndex].channel);
}

static void NetworkInformationChose(const char * choice)
{
	if(choice == mscBack)
		SetNextMenuScreen(&msSearchingForNetworks);
}

static void SelectSecurityTypeInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strcpy(screen->list[0], "No WEP key");
	strcpy(screen->list[1], "40/64 bit key");
	strcpy(screen->list[2], "104 bit key");
	strcpy(screen->list[3], "128 bit key");
}

static void SelectSecurityTypeChose(const char * choice)
{
	MenuScreen * screen = GetMenuScreen();

	WEPType = screen->listSelection;

	if(choice == mscUseKeyType)
	{
		if(WEPType == WM_WEPMODE_NO)
		{
			memset(WEPKey, 0, sizeof(WEPKey));
			memcpy(BSSID, APList[APIndex].bssid, WM_SIZE_BSSID);
			NewConfig = TRUE;
			SetNextMenuScreen(&msConnectingToNetwork);
		}
		else
		{
			SetNextMenuScreen(&msEnterSecurityKey);
		}
	}
	else if(choice == mscBack)
	{
		SetNextMenuScreen(&msSearchingForNetworks);
	}
}

static int HexToInt(char hex)
{
	switch(hex)
	{
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'A':
		return 10;
	case 'B':
		return 11;
	case 'C':
		return 12;
	case 'D':
		return 13;
	case 'E':
		return 14;
	case 'F':
		return 15;
	}

	return -1;
}

static void EnterSecurityKeyInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->keyboardText, WEPEntered, sizeof(screen->keyboardText));
	screen->keyboardText[sizeof(screen->keyboardText) - 1] = '\0';
}

static void EnterSecurityKeyChose(const char * choice)
{
	MenuScreen * screen = GetMenuScreen();

	// save off the key as entered
	strncpy(WEPEntered, screen->keyboardText, sizeof(WEPEntered));
	WEPEntered[sizeof(WEPEntered) - 1] = '\0';

	if(choice == mscEnter)
	{
		int i;
		int len;
		int correctLen;
		int index;
		int val[2];
		char badChar;
		// get the key len
		len = (int)strlen(screen->keyboardText);

		// get the correct len for the type
		if(WEPType == WM_WEPMODE_40BIT)
			correctLen = (40 / 4);
		else if(WEPType == WM_WEPMODE_104BIT)
			correctLen = (104 / 4);
		else if(WEPType == WM_WEPMODE_128BIT)
			correctLen = (128 / 4);
		else
			correctLen = -1;

		// check the len
		if(len != correctLen)
		{
			snprintf(FailureReason, sizeof(FailureReason), "Bad length (%d instead of %d)", len, correctLen);
			SetNextMenuScreen(&msBadSecurityKeyEntered);
			return;
		}

		// convert the keyboard text to hex
		memset(WEPKey, 0, sizeof(WEPKey));
		WEPKey[0] = (u8)WEPType;
		index = 2;
		for(i = 0 ; i < len ; i += 2)
		{
			// convert the next two chars to ints
			val[0] = HexToInt(screen->keyboardText[i]);
			val[1] = HexToInt(screen->keyboardText[i + 1]);

			// check the values
			if((val[0] == -1) || (val[1] == -1))
			{
				if(val[0] == -1)
					badChar = screen->keyboardText[i];
				else
					badChar = screen->keyboardText[i + 1];
				snprintf(FailureReason, sizeof(FailureReason), "Bad value ('%c' instead of hex)", badChar);
				SetNextMenuScreen(&msBadSecurityKeyEntered);
				return;
			}

			// enter them in the WEP array
			WEPKey[index++] = (u8)((val[0] << 4) | val[1]);
		}

		memcpy(BSSID, APList[APIndex].bssid, WM_SIZE_BSSID);
		NewConfig = TRUE;
		SetNextMenuScreen(&msConnectingToNetwork);
	}
	else if (choice == mscBack)
	{
		SetNextMenuScreen(&msSelectSecurityType);
	}
}

static void BadSecurityKeyEnteredInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->extraText[0], FailureReason, MAX_EXTRA_TEXT_STRING_LEN + 1);
	screen->extraText[0][MAX_EXTRA_TEXT_STRING_LEN] = '\0';
}

static void BadSecurityKeyEnteredChose(const char * choice)
{
	if(choice == mscBack)
		SetNextMenuScreen(&msEnterSecurityKey);
}

static void ConnectingToNetworkInit(void)
{
	int result;

	// make sure we're idle
	OS_TPrintf("Waiting for idle...\n");
	while(IW_GetPhase() != IW_PHASE_IDLE)
		;

	// connect
	result = IW_Connect(&APList[APIndex], WEPKey, IW_OPT_POWER_FULL);
	if(!IW_CheckResult(result, "IW_Connect"))
	{
		SetNextMenuScreen(&msFailedToConnectToNetwork);
		return;
	}

	KeepPolling = gsi_true;
}

static void ConnectingToNetworkChose(const char * choice)
{
	if(choice == mscCancel)
	{
		// disconnect
		IW_Disconnect();

#if defined(SUPPORT_SAVED_CONFIGS)
		if(SavedSettings.numConfigs > 0)
			SetNextMenuScreen(&msSelectNetworkConfiguration);
		else
#endif
			SetNextMenuScreen(&msSearchingForNetworks);
	}
}

static void ConnectingToNetworkThink(void)
{
	if(KeepPolling)
	{
		// poll the connect
		if(!IW_Poll(IW_PHASE_IDLE_LINK, IW_PHASE_LINK, "IW_Connect", &KeepPolling))
		{
			IW_Disconnect();
			SetNextMenuScreen(&msFailedToConnectToNetwork);
			return;
		}
		if(KeepPolling)
			return;
	}

	// we're connected to the network
	SetNextMenuScreen(&msConnectingToInternet);
}

static void FailedToConnectToNetworkInit(void)
{
	MenuScreen * screen = GetMenuScreen();
	
	strncpy(screen->extraText[0], FailureReason, MAX_EXTRA_TEXT_STRING_LEN + 1);
	screen->extraText[0][MAX_EXTRA_TEXT_STRING_LEN] = '\0';	
}

static void FailedToConnectToNetworkChose(const char * choice)
{
	if(choice == mscBack)
	{
#if defined(SUPPORT_SAVED_CONFIGS)
		if(SavedSettings.numConfigs > 0)
			SetNextMenuScreen(&msSelectNetworkConfiguration);
		else
#endif
			SetNextMenuScreen(&msSearchingForNetworks);
	}
}

static void ConnectingToInternetInit(void)
{
	int result;
	BOOL linkStatus = FALSE;

	// init the sockets library
	SOC_Init();

	// get the IP link
	do
	{
		IP_GetLinkState(NULL, &linkStatus);
	}
	while(!linkStatus);

	// startup the sockets library
	result = SOC_Startup(&SocketsConfig);
	if(result < 0)
	{
		snprintf(FailureReason, sizeof(FailureReason), "SOC_Startup failed: %d", result);
		SetNextMenuScreen(&msFailedToConnectToInternet);
		return;
	}
}

static void ConnectingToInternetChose(const char * choice)
{
	if(choice == mscCancel)
	{
		SOC_Cleanup();
		IW_Disconnect();

#if defined(SUPPORT_SAVED_CONFIGS)
		if(SavedSettings.numConfigs > 0)
			SetNextMenuScreen(&msSelectNetworkConfiguration);
		else
#endif
			SetNextMenuScreen(&msSearchingForNetworks);
	}
}

static void ConnectingToInternetThink(void)
{
	u32 localIP;

	// check if we have been allocated an IP
	localIP = (u32)SOC_GetHostID();
	if(localIP != SOC_HtoNl(SOC_INADDR_ANY))
	{
#if !defined(AUTO_CONNECT)
		SetNextMenuScreen(&msConnectedToInternet);
#else
		u8 ip[IP_ALEN];
		SOInAddr dns1, dns2;
		u8 mac[MAC_ALEN];

		IP_GetAddr(NULL, ip);
		OS_Printf("IP:      %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

		IP_GetGateway (NULL, ip);
		OS_Printf("Gateway: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);

		SOC_GetResolver(&dns1, &dns2);
		OS_Printf("DNS1:    %s\n", inet_ntoa(dns1));
		OS_Printf("DNS2:    %s\n", inet_ntoa(dns2));

		IP_GetMacAddr(NULL, mac);
		OS_Printf("MAC:     %02X-%02X-%02X-%02X-%02X-%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		ExitMenu();
#endif
	}
}

static void FailedToConnectToInternetInit(void)
{
	MenuScreen * screen = GetMenuScreen();
	
	strncpy(screen->extraText[0], FailureReason, MAX_EXTRA_TEXT_STRING_LEN + 1);
	screen->extraText[0][MAX_EXTRA_TEXT_STRING_LEN] = '\0';	
}

static void FailedToConnectToInternetChose(const char * choice)
{
	if(choice == mscBack)
	{
#if defined(SUPPORT_SAVED_CONFIGS)
		if(SavedSettings.numConfigs > 0)
			SetNextMenuScreen(&msSelectNetworkConfiguration);
		else
#endif
			SetNextMenuScreen(&msSearchingForNetworks);
	}
}

static void ConnectedToInternetInit(void)
{
	MenuScreen * screen = GetMenuScreen();
	u8 ip[IP_ALEN];
	SOInAddr dns1, dns2;
	u8 mac[MAC_ALEN];
	int line = 0;

	IP_GetAddr(NULL, ip);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  IP:        %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	IP_GetAlias(NULL, ip);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  Alias:     %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	IP_GetBroadcastAddr(NULL, ip);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  Broadcast: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	IP_GetNetmask (NULL, ip);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  Netmask:   %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	IP_GetGateway (NULL, ip);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  Gateway:   %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	SOC_GetResolver(&dns1, &dns2);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  DNS1:      %s", inet_ntoa(dns1));
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  DNS2:      %s", inet_ntoa(dns2));

	IP_GetMacAddr(NULL, mac);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  MAC:       %02X-%02X-%02X-%02X-%02X-%02X",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void ConnectedToInternetChose(const char * choice)
{
	if(choice == mscContinue)
	{
#if defined(SUPPORT_SAVED_CONFIGS)
		if(BackupExists() && NewConfig && (SavedSettings.numConfigs < MAX_SAVED_CONFIGS))
			SetNextMenuScreen(&msSaveNetworkConfiguration);
		else
#endif
			ExitMenu();
	}
}

static void SaveNetworkConfigurationInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->keyboardText, (const char *)APList[APIndex].ssid, MAX_KEYBOARD_TEXT_LEN);
	screen->keyboardText[MAX_KEYBOARD_TEXT_LEN - 1] = '\0';
}

static void SaveNetworkConfigurationChose(const char * choice)
{
	MenuScreen * screen = GetMenuScreen();

	if(choice == mscSave)
	{
		memcpy(SavedSettings.configs[SavedSettings.numConfigs].bssid, BSSID, IW_BSSID_SIZE);
		memcpy(SavedSettings.configs[SavedSettings.numConfigs].wep, WEPKey, IW_WEP_SIZE);
		strncpy(SavedSettings.configs[SavedSettings.numConfigs].name, screen->keyboardText, MAX_CONFIG_NAME_LEN);
		SavedSettings.configs[SavedSettings.numConfigs].name[MAX_CONFIG_NAME_LEN - 1] = '\0';
		SavedSettings.numConfigs++;
		SaveSettings();

		SetNextMenuScreen(&msConfigurationSaved);
	}
	else if(choice == mscDontSave)
	{
		ExitMenu();
	}
}

static void ConfigurationSavedChose(const char * choice)
{
	if(choice == mscContinue)
		ExitMenu();
}
