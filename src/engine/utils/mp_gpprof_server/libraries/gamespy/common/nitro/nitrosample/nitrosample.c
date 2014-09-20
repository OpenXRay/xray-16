#include "../../nonport.h"
#include "../../peer/peer.h"
#include "../../peer/peerMain.h"
#include "menu.h"
#include "screen.h"

/************************************************************************/
/* Sample Menu                                                          */
/************************************************************************/

static const char mscHostServer[] = "Host Server";
static const char mscListServers[] = "List Servers";
static const char mscExit[] = "Exit";
static const char mscStartHosting[] = "Start Hosting";
static const char mscCancel[] = "Cancel";
static const char mscStopHosting[] = "Stop Hosting";
static const char mscViewHostInfo[] = "View Host Info";
static const char mscMainMenu[] = "Main Menu";
static const char mscBack[] = "Back";

static void CheckingBackendAvailabilityInit(void);
static void CheckingBackendAvailabilityChose(const char * choice);
static void CheckingBackendAvailabilityThink(void);

static MenuScreenConfiguration msCheckingBackendAvailability =
{
	"Checking Backend Availability",
	{
		{ mscCancel }
	},
	CheckingBackendAvailabilityInit,
	CheckingBackendAvailabilityChose,
	CheckingBackendAvailabilityThink,
	SCREEN_OPTION_ANIMATED
};

static void BackendUnavailableChose(const char * choice);

static MenuScreenConfiguration msBackendUnavailable =
{
	"Backend Unavailable",
	{
		{ mscExit }
	},
	NULL,
	BackendUnavailableChose
};

static void FailedToIntializePeerSDKChose(const char * choice);

static MenuScreenConfiguration msFailedToIntializePeerSDK =
{
	"Failed to Initialize Peer SDK",
	{
		{ mscExit }
	},
	NULL,
	FailedToIntializePeerSDKChose
};

static void HostOrListChose(const char * choice);

static MenuScreenConfiguration msHostOrList =
{
	"Host or List",
	{
		{ mscHostServer },
		{ mscListServers },
		{ mscExit }
	},
	NULL,
	HostOrListChose
};

static void ChooseHostingNameInit(void);
static void ChooseHostingNameChose(const char * choice);

static MenuScreenConfiguration msChooseHostingName =
{
	"Choose Hosting Name",
	{
		{ mscStartHosting },
		{ mscCancel}
	},
	ChooseHostingNameInit,
	ChooseHostingNameChose,
	NULL,
	SCREEN_OPTION_KEYBOARD
};

static void HostingServerInit(void);
static void HostingServerChose(const char * choice);
static void HostingServerThink(void);

static MenuScreenConfiguration msHostingServer =
{
	"Hosting Server",
	{
		{ mscStopHosting }
	},
	HostingServerInit,
	HostingServerChose,
	HostingServerThink,
	SCREEN_OPTION_EXTRAS_CENTERED
};

static void ErrorStartingHostInit(void);
static void ErrorStartingHostChose(const char * choice);

static MenuScreenConfiguration msErrorStartingHost =
{
	"Error Starting Host",
	{
		{ mscMainMenu }
	},
	ErrorStartingHostInit,
	ErrorStartingHostChose
};

static void ListingHostsInit(void);
static void ListingHostsChose(const char * choice);
static void ListingHostsThink(void);

static MenuScreenConfiguration msListingHosts =
{
	"Listing Hosts",
	{
		{ mscViewHostInfo, CHOICE_OPTION_NEEDS_LIST_SELECTION },
		{ mscMainMenu }
	},
	ListingHostsInit,
	ListingHostsChose,
	ListingHostsThink,
	SCREEN_OPTION_LIST | SCREEN_OPTION_ANIMATED
};

static void ErrorListingHostsInit(void);
static void ErrorListingHostsChose(const char * choice);

static MenuScreenConfiguration msErrorListingHosts =
{
	"Error Listing Hosts",
	{
		{ mscMainMenu }
	},
	ErrorListingHostsInit,
	ErrorListingHostsChose
};

static void ViewHostInfoInit(void);
static void ViewHostInfoChose(const char * choice);

static MenuScreenConfiguration msViewHostInfo =
{
	"View Host Info",
	{
		{ mscBack }
	},
	ViewHostInfoInit,
	ViewHostInfoChose,
	NULL,
	SCREEN_OPTION_EXTRAS_CENTERED
};

/************************************************************************/
/* Sample Funcs                                                         */
/************************************************************************/


#define GAMENAME "gmtest"
#define SECRETKEY "HA6zkS"

#define MAX_HOSTNAME_LEN   MAX_KEYBOARD_TEXT_LEN

static char Hostname[MAX_HOSTNAME_LEN + 1] = "Nitro Host";
static char FailureReason[MAX_EXTRA_TEXT_STRINGS][MAX_EXTRA_TEXT_STRING_LEN + 1];
static qr2_error_t QR2AddError;
static BOOL ListingError;
static PEER Peer;
static SBServer SelectedServer;
static int Mapname;
static int Gametype;

static const char Mapnames[][64] = { "Rome", "London", "New York" };
static const char Gametypes[][64] = { "Deathmatch", "Teamplay", "1 on 1" };
static const int NumMapnames = (sizeof(Mapnames) / sizeof(Mapnames[0]));
static const int NumGametypes = (sizeof(Gametypes) / sizeof(Gametypes[0]));

static gsi_u32 DebugLog2(gsi_u32 level)
{
	gsi_u32 total = 0;
	while (level > 1)
	{
		level = level >> 1;
		total++;
	}
	return total;
}

static void DebugCallback(GSIDebugCategory category, GSIDebugType type,
                          GSIDebugLevel level, const char * format, va_list params)
{
	// Output line prefix
	Printf("[%s][%s][%s] ", 
		gGSIDebugCatStrings[category], 
		gGSIDebugTypeStrings[type],
		gGSIDebugLevelStrings[DebugLog2(level)]);
	
	// Output to file
	VPrintf(format, 
		params);
}

int main(int argc, char * argv)
{
	gsSetDebugCallback(DebugCallback);
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Verbose);

	GSI_UNUSED(argc);
	GSI_UNUSED(argv);

	StartMenuScreen(&msCheckingBackendAvailability);
	
	return 0;
}

static const char * QR2ErrorToString(qr2_error_t result)
{
	if(result == e_qrnoerror)
		return "no error";
	if(result == e_qrwsockerror)
		return "socket error";
	if(result == e_qrbinderror)
		return "bind error";
	if(result == e_qrdnserror)
		return "dns lookup error";
	if(result == e_qrconnerror)
		return "nat error";
	if(result == e_qrnochallengeerror)
		return "no challenge received";
	return "unknown error";
}

static void SetFailureReason(const char * reason)
{
	int i;
	const char * line;
	const char * nextLine;
	size_t len;

	memset(FailureReason, 0, sizeof(FailureReason));

	nextLine = reason;

	for(i = 0 ; i < MAX_EXTRA_TEXT_STRINGS ; i++)
	{
		line = nextLine;
		if(line[0] == '\0')
			return;

		nextLine = strchr(line, '\n');
		if(nextLine && ((nextLine - line) < MAX_EXTRA_TEXT_STRING_LEN))
		{
			len = (size_t)(nextLine - line);
			nextLine++;
		}
		else if(strlen(line) >= MAX_EXTRA_TEXT_STRING_LEN)
		{
			len = MAX_EXTRA_TEXT_STRING_LEN;
			nextLine = (line + MAX_EXTRA_TEXT_STRING_LEN);
		}
		else
		{
			len = strlen(line);
			nextLine = "";
		}

		memcpy(FailureReason[i], line, len);
		FailureReason[i][len] = '\0';
	}
}

static void KeyListCallback(PEER peer, qr2_key_type type, qr2_keybuffer_t buffer, void * param)
{
	switch(type)
	{
	case key_server:
		qr2_keybuffer_add(buffer, HOSTNAME_KEY);
		qr2_keybuffer_add(buffer, MAPNAME_KEY);
		qr2_keybuffer_add(buffer, GAMETYPE_KEY);
		break;
	case key_player:
		break;
	case key_team:
		break;
	}

	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void ServerKeyCallback(PEER peer, int key, qr2_buffer_t buffer, void * param)
{
	switch(key)
	{
	case HOSTNAME_KEY:
		qr2_buffer_add(buffer, Hostname);
		break;
	case MAPNAME_KEY:
		qr2_buffer_add(buffer, Mapnames[Mapname]);
		break;
	case GAMETYPE_KEY:
		qr2_buffer_add(buffer, Gametypes[Gametype]);
		break;
	default:
		qr2_buffer_add(buffer, _T(""));
	}
	
	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void AddErrorCallback(PEER peer, qr2_error_t error, gsi_char * errorString, void * param)
{
	QR2AddError = error;

	SetFailureReason(errorString);

	GSI_UNUSED(peer);
	GSI_UNUSED(param);
}

static void ListingGamesCallback(PEER peer, PEERBool success, const gsi_char * name, SBServer server,
								 PEERBool staging, int msg, int progress, void * param)
{
	int num;
	int i;
	MenuScreen * screen = GetMenuScreen();
	piConnection * connection = (piConnection *)peer;

	if(success == PEERFalse)
	{
		ListingError = PEERTrue;
		return;
	}

	num = SBServerListCount(&connection->gameList);

	for(i = 0 ; i < MAX_LIST_STRINGS ; i++)
	{
		if(i < num)
		{
			server = SBServerListNth(&connection->gameList, i);
			strncpy(screen->list[i], SBServerGetStringValue(server, "hostname", "[no name]"), MAX_LIST_STRING_LEN + 1);
			screen->list[i][MAX_LIST_STRING_LEN] = '\0';
		}
		else
		{
			screen->list[i][0] = '\0';
		}
	}

	GSI_UNUSED(name);
	GSI_UNUSED(staging);
	GSI_UNUSED(msg);
	GSI_UNUSED(progress);
	GSI_UNUSED(param);
}

/************************************************************************/
/* Sample Menu Funcs                                                    */
/************************************************************************/

static void CheckingBackendAvailabilityInit(void)
{
	GSIStartAvailableCheck(GAMENAME);
}
static void CheckingBackendAvailabilityChose(const char * choice)
{
	if(choice == mscCancel)
		ExitMenu();
}
static void CheckingBackendAvailabilityThink(void)
{
	GSIACResult result = GSIAvailableCheckThink();
	PEERCallbacks callbacks;

	if(result == GSIACWaiting)
		return;

	if(result != GSIACAvailable)
	{
		SetNextMenuScreen(&msBackendUnavailable);
		return;
	}

	memset(&callbacks, 0, sizeof(PEERCallbacks));
	callbacks.qrKeyList = KeyListCallback;
	callbacks.qrServerKey = ServerKeyCallback;
	callbacks.qrAddError = AddErrorCallback;

	Peer = peerInitialize(&callbacks);
	if(!Peer)
	{
		SetNextMenuScreen(&msFailedToIntializePeerSDK);
		return;
	}

	if(!peerSetTitle(Peer, GAMENAME, SECRETKEY, GAMENAME, SECRETKEY, 0, 10, PEERTrue, NULL, NULL))
	{
		peerShutdown(Peer);
		SetNextMenuScreen(&msFailedToIntializePeerSDK);
		return;
	}

	SetNextMenuScreen(&msHostOrList);
}

static void BackendUnavailableChose(const char * choice)
{
	if(choice == mscExit)
		ExitMenu();
}

static void FailedToIntializePeerSDKChose(const char * choice)
{
	if(choice == mscExit)
		ExitMenu();
}

static void HostOrListChose(const char * choice)
{
	if(choice == mscHostServer)
		SetNextMenuScreen(&msChooseHostingName);
	else if(choice == mscListServers)
		SetNextMenuScreen(&msListingHosts);
	else if(choice == mscExit)
	{
		peerShutdown(Peer);
		ExitMenu();
	}
}

static void ChooseHostingNameInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(screen->keyboardText, Hostname, MAX_KEYBOARD_TEXT_LEN + 1);
	screen->keyboardText[MAX_KEYBOARD_TEXT_LEN] = '\0';
}
static void ChooseHostingNameChose(const char * choice)
{
	MenuScreen * screen = GetMenuScreen();

	strncpy(Hostname, screen->keyboardText, MAX_HOSTNAME_LEN + 1);
	Hostname[MAX_HOSTNAME_LEN] = '\0';

	if(choice == mscStartHosting)
		SetNextMenuScreen(&msHostingServer);
	else if(choice == mscCancel)
		SetNextMenuScreen(&msHostOrList);
}

static void HostingServerInit(void)
{
	PEERBool result;
	MenuScreen * screen = GetMenuScreen();
	int line = 0;

	QR2AddError = e_qrnoerror;

	srand((unsigned int)time(NULL));
	Mapname = (rand() % NumMapnames);
	Gametype = (rand() % NumGametypes);

	result = peerStartReporting(Peer);

	if(result == PEERFalse)
	{
		SetFailureReason("Failed to initialize hosting");
		SetNextMenuScreen(&msErrorStartingHost);
		return;
	}

	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN + 1, "Mapname: %s", Mapnames[Mapname]);
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN + 1, "Gametype: %s", Gametypes[Gametype]);
}
static void HostingServerChose(const char * choice)
{
	if(choice == mscStopHosting)
	{
		peerStopGame(Peer);
		SetNextMenuScreen(&msHostOrList);
	}
}
static void HostingServerThink(void)
{
	peerThink(Peer);

	if(QR2AddError != e_qrnoerror)
	{
		char text[256];

		peerStopGame(Peer);
		snprintf(text, sizeof(text), "Failed to initialize hosting:\n%s", QR2ErrorToString(QR2AddError));
		SetFailureReason(text);
		SetNextMenuScreen(&msErrorStartingHost);
	}
}

static void ErrorStartingHostInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	memcpy(screen->extraText, FailureReason, sizeof(FailureReason));
}
static void ErrorStartingHostChose(const char * choice)
{
	if(choice == mscMainMenu)
		SetNextMenuScreen(&msHostOrList);
}

static void ListingHostsInit(void)
{
	const unsigned char keys[] = { HOSTNAME_KEY, MAPNAME_KEY, GAMETYPE_KEY };
	ListingError = FALSE;

	peerStartListingGames(Peer, keys, sizeof(keys), NULL, ListingGamesCallback, NULL);
}
static void ListingHostsChose(const char * choice)
{
	if(choice == mscViewHostInfo)
	{
		MenuScreen * screen = GetMenuScreen();
		piConnection * connection = (piConnection *)Peer;

		SelectedServer = SBServerListNth(&connection->gameList, screen->listSelection);
		SetNextMenuScreen(&msViewHostInfo);
	}
	else if(choice == mscMainMenu)
	{
		peerStopListingGames(Peer);
		SetNextMenuScreen(&msHostOrList);
	}
}
static void ListingHostsThink(void)
{
	peerThink(Peer);

	if(ListingError == TRUE)
	{
		peerStopListingGames(Peer);
		SetFailureReason("Failed to list hosts");
		SetNextMenuScreen(&msErrorListingHosts);
	}
}

static void ErrorListingHostsInit(void)
{
	MenuScreen * screen = GetMenuScreen();

	memcpy(screen->extraText, FailureReason, sizeof(FailureReason));
}
static void ErrorListingHostsChose(const char * choice)
{
	if(choice == mscMainMenu)
		SetNextMenuScreen(&msHostOrList);
}

static void ViewHostInfoInit(void)
{
	MenuScreen * screen = GetMenuScreen();
	int line = 0;

	// hostname
	strcpy(screen->extraText[line++], "[hostname]");
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  %s", SBServerGetStringValue(SelectedServer, "hostname", "N/A"));

	// public address
	strcpy(screen->extraText[line++], "[public address]");
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  %s:%d",
		SBServerGetPublicAddress(SelectedServer), SBServerGetPublicQueryPort(SelectedServer));

	// private address
	strcpy(screen->extraText[line++], "[private address]");
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  %s:%d",
		SBServerGetPrivateAddress(SelectedServer), SBServerGetPrivateQueryPort(SelectedServer));

	// mapname
	strcpy(screen->extraText[line++], "[mapname]");
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  %s", SBServerGetStringValue(SelectedServer, "mapname", "N/A"));

	// gametype
	strcpy(screen->extraText[line++], "[gametype]");
	snprintf(screen->extraText[line++], MAX_EXTRA_TEXT_STRING_LEN,
		"  %s", SBServerGetStringValue(SelectedServer, "gametype", "N/A"));
}
static void ViewHostInfoChose(const char * choice)
{
	if(choice == mscBack)
		SetNextMenuScreen(&msListingHosts);
}
