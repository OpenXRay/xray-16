// gptestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "gptest.h"
#include "gptestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CGptestDlg * dlg;

#define CHECK(result)  { if((result) != GP_NO_ERROR) { OutputDebugString(#result " failed\n");}}

#define PROCESS_TIME            20

#define GSI_DEFAULT_NAMESPACE   1
#define GSI_TEST_PRODUCTID      0
#define GSI_TEST_GAMENAME       "gmtest"


void vsDebugOut(va_list args, const char * format, ...)
{
#ifdef GSI_COMMON_DEBUG
	char buffer[256];

	// If args list is null then build it.
	if(args == NULL)
	{
		va_start(args, format);
		_vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
	}
	else
		_vsnprintf(buffer, sizeof(buffer), format, args);

	OutputDebugString(buffer);
#endif
	GSI_UNUSED(args);
	GSI_UNUSED(format);
}

#ifdef GSI_COMMON_DEBUG
	static void DebugCallback(GSIDebugCategory theCat, GSIDebugType theType,
	                          GSIDebugLevel theLevel, const char * theTokenStr,
	                          va_list theParamList)
	{
		GSI_UNUSED(theLevel);

		vsDebugOut(NULL, "[%s][%s] ",
				gGSIDebugCatStrings[theCat], 
				gGSIDebugTypeStrings[theType]);
		vsDebugOut(theParamList, theTokenStr);
	}
#endif


/////////////////////////////////////////////////////////////////////////////
// CGptestDlg dialog

CGptestDlg::CGptestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGptestDlg::IDD, pParent)
	, m_StatusState(0)
	, m_QueryPort(0)
	, m_HostPort(0)
	, m_RichStatus(_T(""))
	, m_GameType(_T(""))
	, m_GameVariant(_T(""))
	, m_GameMapname(_T(""))
	, m_KeyName(_T(""))
	, m_KeyValue(_T(""))
	, m_HostIp(_T(""))
	, m_HostPrivateIp(_T(""))
	, m_cdkey(_T(""))
{
	//{{AFX_DATA_INIT(CGptestDlg)
	m_partnerid = 0;
	m_email = _T("");
	m_nick = _T("");
	m_password = _T("");
	m_locationString = _T("");
	m_status = 1;
	m_statusString = _T("");
	m_iaddress = _T("");
	m_icountrycode = _T("");
	m_iemail = _T("");
	m_ihomepage = _T("");
	m_iicquin = 0;
	m_inick = _T("");
	m_isex = _T("");
	m_message = _T("Hi.  This is a message!");
	m_sfirstname = _T("");
	m_snick = _T("");
	m_sicquin = 0;
	m_slastname = _T("");
	m_semail = _T("");
	m_string = _T("");
	m_code = _T("");
	m_reason = _T("Hello.");
	m_rnick = 0;
	m_infoCache = TRUE;
	m_blocking = FALSE;
	m_ilastname = _T("");
	m_ifirstname = _T("");
	m_ibirthday = 0;
	m_ibirthmonth = 0;
	m_ibirthyear = 0;
	m_ipmbirthday = FALSE;
	m_ipmcountrycode = FALSE;
	m_ipmhomepage = FALSE;
	m_ipmsex = FALSE;
	m_ipmzipcode = FALSE;
	m_newnick = _T("");
	m_replace = FALSE;
	m_izipcode = _T("");
	m_invitePlayerID = 1;
	m_server = 0;
	m_otherServer = _T("");
	m_name = _T("");
	m_path = _T("");
	m_searchServer = _T("");
	m_ipmemail = FALSE;
	m_firewall = FALSE;
	m_ilatitude = 0.0f;
	m_iplace = _T("");
	m_ilongitude = 0.0f;
	m_suniquenick = _T("");
	m_iuniquenick = _T("");
	m_uniquenick = _T("");
	m_authtoken = _T("");
	m_partnerchallenge = _T("");
	m_namespace = _T("");
	m_productid = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	
}

void CGptestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGptestDlg)
	DDX_Control(pDX, IDC_CONNECTUNIQUE, m_connectunique);
	DDX_Control(pDX, IDC_CONNECTPREAUTH, m_connectpreauth);
	DDX_Control(pDX, IDC_NEWUSER, m_newuser);
	DDX_Control(pDX, IDC_SEARCH, m_search);
	DDX_Control(pDX, IDC_RESULTS, m_results);
	DDX_Control(pDX, IDC_UPDATE, m_update);
	DDX_Control(pDX, IDC_SEND, m_send);
	DDX_Control(pDX, IDC_SET, m_set);
	DDX_Control(pDX, IDC_BUDDIES, m_buddies);
	DDX_Control(pDX, IDC_DISCONNECT, m_disconnect);
	DDX_Control(pDX, IDC_CONNECT, m_connect);
	DDX_Text(pDX, IDC_PARTNERID, m_partnerid);
	DDX_Text(pDX, IDC_EMAIL, m_email);
	DDV_MaxChars(pDX, m_email, 50);
	DDX_Text(pDX, IDC_NICK, m_nick);
	DDV_MaxChars(pDX, m_nick, 30);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDV_MaxChars(pDX, m_password, 30);
	DDX_Text(pDX, IDC_LOCATIONSTRING, m_locationString);
	DDV_MaxChars(pDX, m_locationString, 255);
	DDX_Text(pDX, IDC_STATUS, m_status);
	DDV_MinMaxInt(pDX, m_status, 0, 100);
	DDX_Text(pDX, IDC_STATUSSTRING, m_statusString);
	DDV_MaxChars(pDX, m_statusString, 255);
	DDX_Text(pDX, IDC_IADDRESS, m_iaddress);
	DDX_Text(pDX, IDC_ICOUNTRYCODE, m_icountrycode);
	DDV_MaxChars(pDX, m_icountrycode, 2);
	DDX_Text(pDX, IDC_IEMAIL, m_iemail);
	DDX_Text(pDX, IDC_IHOMEPAGE, m_ihomepage);
	DDX_Text(pDX, IDC_IICQUIN, m_iicquin);
	DDX_Text(pDX, IDC_INICK, m_inick);
	DDX_Text(pDX, IDC_ISEX, m_isex);
	DDX_Text(pDX, IDC_MESSAGE, m_message);
	DDV_MaxChars(pDX, m_message, 1000);
	DDX_Text(pDX, IDC_SFIRSTNAME, m_sfirstname);
	DDV_MaxChars(pDX, m_sfirstname, 30);
	DDX_Text(pDX, IDC_SNICK, m_snick);
	DDV_MaxChars(pDX, m_snick, 30);
	DDX_Text(pDX, IDC_SICQUIN, m_sicquin);
	DDX_Text(pDX, IDC_SLASTNAME, m_slastname);
	DDV_MaxChars(pDX, m_slastname, 30);
	DDX_Text(pDX, IDC_SEMAIL, m_semail);
	DDV_MaxChars(pDX, m_semail, 50);
	DDX_Text(pDX, IDC_STRING, m_string);
	DDX_Text(pDX, IDC_CODE, m_code);
	DDX_Text(pDX, IDC_REASON, m_reason);
	DDV_MaxChars(pDX, m_reason, 1024);
	DDX_Radio(pDX, IDC_RNICK, m_rnick);
	DDX_Check(pDX, IDC_INFO_CACHE, m_infoCache);
	DDX_Check(pDX, IDC_BLOCKING, m_blocking);
	DDX_Text(pDX, IDC_ILASTNAME, m_ilastname);
	DDX_Text(pDX, IDC_IFIRSTNAME, m_ifirstname);
	DDX_Text(pDX, IDC_IBIRTHDAY, m_ibirthday);
	DDX_Text(pDX, IDC_IBIRTHMONTH, m_ibirthmonth);
	DDX_Text(pDX, IDC_IBIRTHYEAR, m_ibirthyear);
	DDX_Check(pDX, IDC_IPMBIRTHDAY, m_ipmbirthday);
	DDX_Check(pDX, IDC_IPMCOUNTRYCODE, m_ipmcountrycode);
	DDX_Check(pDX, IDC_IPMHOMEPAGE, m_ipmhomepage);
	DDX_Check(pDX, IDC_IPMSEX, m_ipmsex);
	DDX_Check(pDX, IDC_IPMZIPCODE, m_ipmzipcode);
	DDX_Text(pDX, IDC_NEWNICK, m_newnick);
	DDX_Check(pDX, IDC_REPLACE, m_replace);
	DDX_Text(pDX, IDC_IZIPCODE, m_izipcode);
	DDX_Text(pDX, IDC_INVITE_PLAYER_ID, m_invitePlayerID);
	DDX_Radio(pDX, IDC_SAUTO, m_server);
	DDX_Text(pDX, IDC_OTHER_SERVER, m_otherServer);
	DDX_Text(pDX, IDC_NAME, m_name);
	DDX_Text(pDX, IDC_PATH, m_path);
	DDX_Text(pDX, IDC_SEARCH_SERVER, m_searchServer);
	DDX_Check(pDX, IDC_IPMEMAIL, m_ipmemail);
	DDX_Check(pDX, IDC_FIREWALL, m_firewall);
	DDX_Text(pDX, IDC_ILATITUDE, m_ilatitude);
	DDX_Text(pDX, IDC_IPLACE, m_iplace);
	DDX_Text(pDX, IDC_ILONGITUDE, m_ilongitude);
	DDX_Text(pDX, IDC_SUNIQUENICK, m_suniquenick);
	DDV_MaxChars(pDX, m_suniquenick, 50);
	DDX_Text(pDX, IDC_IUNIQUENICK, m_iuniquenick);
	DDX_Text(pDX, IDC_UNIQUENICK, m_uniquenick);
	DDV_MaxChars(pDX, m_uniquenick, 50);
	DDX_Text(pDX, IDC_AUTHTOKEN, m_authtoken);
	DDV_MaxChars(pDX, m_authtoken, 255);
	DDX_Text(pDX, IDC_PARTNERCHALLENGE, m_partnerchallenge);
	DDV_MaxChars(pDX, m_partnerchallenge, 255);
	DDX_Text(pDX, IDC_NAMESPACE, m_namespace);
	DDX_Text(pDX, IDC_PRODUCTID, m_productid);
	//}}AFX_DATA_MAP
	DDX_CBIndex(pDX, IDC_STATUS_STATE, m_StatusState);
	DDX_Text(pDX, IDC_QPORT, m_QueryPort);
	DDX_Text(pDX, IDC_HPORT, m_HostPort);
	DDX_Control(pDX, IDC_SFLAGS, m_SessionFlags);
	DDX_Text(pDX, IDC_RICH_STATUS, m_RichStatus);
	DDX_Text(pDX, IDC_GAME_TYPE, m_GameType);
	DDX_Text(pDX, IDC_GAME_VARIANT, m_GameVariant);
	DDX_Text(pDX, IDC_GAME_MAPNAME, m_GameMapname);
	DDX_Text(pDX, IDC_EDIT1, m_KeyName);
	DDX_Text(pDX, IDC_EDIT2, m_KeyValue);
	DDX_Text(pDX, IDC_HOST_IP, m_HostIp);
	DDX_Text(pDX, IDC_HOST_PRIVATE_IP, m_HostPrivateIp);
	DDX_Text(pDX, IDC_CDKEY, m_cdkey);
    DDX_Control(pDX, IDC_BLOCKLIST, m_blocklist);
    DDX_Control(pDX, IDC_GET_BLOCKEDLIST, m_getblockedlist);
    DDX_Control(pDX, IDC_ADD_BLOCK, m_addblock);
    DDX_Control(pDX, IDC_REMOVE_BLOCK, m_removeblock);
}

BEGIN_MESSAGE_MAP(CGptestDlg, CDialog)
	//{{AFX_MSG_MAP(CGptestDlg)
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_DISCONNECT, OnDisconnect)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_SET, OnSet)
	ON_LBN_SELCHANGE(IDC_BUDDIES, OnSelchangeBuddies)
	ON_BN_CLICKED(IDC_SEND, OnSend)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_UPDATE, OnUpdate)
	ON_BN_CLICKED(IDC_SEARCH, OnSearch)
	ON_LBN_SELCHANGE(IDC_RESULTS, OnSelchangeResults)
	ON_BN_CLICKED(IDC_NEWUSER, OnNewuser)
	ON_BN_CLICKED(IDC_SENDREQUEST, OnSendrequest)
	ON_BN_CLICKED(IDC_INFO_CACHE, OnInfoCache)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_SETINFO, OnSetinfo)
	ON_BN_CLICKED(IDC_DELETEPRO, OnDeletepro)
	ON_BN_CLICKED(IDC_NEWPRO, OnNewpro)
	ON_BN_CLICKED(IDC_DELETEALL, OnDeleteall)
	ON_BN_CLICKED(IDC_VALIDATE, OnValidate)
	ON_BN_CLICKED(IDC_NICKS, OnNicks)
	ON_BN_CLICKED(IDC_INVITE_PLAYER, OnInvitePlayer)
	ON_BN_CLICKED(IDC_REPORT, OnReport)
	ON_BN_CLICKED(IDC_CHECK, OnCheck)
	ON_BN_CLICKED(IDC_SEND_FILES, OnSendFiles)
	ON_EN_CHANGE(IDC_SEARCH_SERVER, OnChangeSearchServer)
	ON_BN_CLICKED(IDC_PUBLICMASK_ALL, OnPublicmaskAll)
	ON_BN_CLICKED(IDC_PUBLICMASK_NONE, OnPublicmaskNone)
	ON_BN_CLICKED(IDC_REVOKE, OnRevoke)
	ON_BN_CLICKED(IDC_SUGGEST, OnSuggest)
	ON_BN_CLICKED(IDC_CONNECTUNIQUE, OnConnectunique)
	ON_BN_CLICKED(IDC_CONNECTPREAUTH, OnConnectpreauth)
	ON_BN_CLICKED(IDC_INITIALIZE, OnInitialize)
	ON_BN_CLICKED(IDC_DESTROY, OnDestroyGP)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_UTM, OnUTM)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_SETSTATUSINFO, OnSetstatusinfo)
	ON_BN_CLICKED(IDC_BUTTON2, OnAddSetKey)
	ON_BN_CLICKED(IDC_BUTTON4, OnGetKeyValue)
	ON_BN_CLICKED(IDC_BUTTON5, OnGetBuddyKeys)
	ON_BN_CLICKED(IDC_BUTTON3, OnDelKeyVal)
	ON_BN_CLICKED(IDC_REGISTER_CDKEY, OnRegisterCdKey)
    ON_BN_CLICKED(IDC_GET_BLOCKEDLIST, OnGetBlocked)
    ON_LBN_SELCHANGE(IDC_BLOCKLIST, OnSelchangeBlocklist)
    ON_BN_CLICKED(IDC_ADD_BLOCK, OnAddBlock)
    ON_BN_CLICKED(IDC_REMOVE_BLOCK, OnRemoveBlock)
END_MESSAGE_MAP()

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGptestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

//GP CALLBACKS
//////////////
void ConnectResponse(GPConnection * connection, void * arg_, void * param)
{
	GPConnectResponseArg * arg = (GPConnectResponseArg *)arg_;

	if(arg->result == GP_NO_ERROR)
	{
		dlg->m_StatusState = dlg->m_status = GP_ONLINE;
		dlg->m_statusString = "Online";
		dlg->m_locationString = "";

		dlg->m_uniquenick = arg->uniquenick;
		//dlg->m_RichStatus = "Online";
		dlg->UpdateData(FALSE);

		gpSetStatus(connection, (GPEnum)dlg->m_status, dlg->m_statusString, dlg->m_locationString);
		//gpSetStatusInfo(connection, (GPEnum)dlg->m_StatusState, 0, 0, 0, 0, 0, dlg->m_RichStatus, dlg->m_RichStatus.GetLength(), "", 0, "", 0, "", 0);
		CString text;
		text.Format("Connected as profileid %d\r\n", arg->profile);
		OutputDebugString((LPCSTR)text);
	}
	else
	{
		dlg->MessageBox("Connect failed");
	}
	GSI_UNUSED(param);
}

void CheckResponse(GPConnection * connection, void * arg_, void * param)
{
	GPCheckResponseArg * arg = (GPCheckResponseArg *)arg_;

	CString strMessage;
	char buf[16];

	strMessage.Format("%s@%s (%s)\n", dlg->m_nick, dlg->m_email, dlg->m_password);

	if(arg->result == GP_NO_ERROR)
	{
		strMessage += "pid = ";
		itoa(arg->profile, buf, 10);
		strMessage += buf;
	}
	else
	{
		GPErrorCode errorCode;
		gpGetErrorCode(connection, &errorCode);
		if(errorCode == GP_CHECK_BAD_EMAIL)
			strMessage += "Invalid e-mail";
		else if(errorCode == GP_CHECK_BAD_PASSWORD)
			strMessage += "Invalid password";
		else if(errorCode == GP_CHECK_BAD_NICK)
			strMessage += "Invalid nick";
	}

	dlg->MessageBox(strMessage);
	GSI_UNUSED(param);
}

void NewUserResponse(GPConnection * connection, void * arg_, void * param)
{
	GPNewUserResponseArg * arg = (GPNewUserResponseArg *)arg_;

	CString strMessage;
	char buf[16];

	if(arg->result == GP_NO_ERROR)
	{
		strMessage = "Success\n";

		strMessage += "pid = ";
		itoa(arg->profile, buf, 10);
		strMessage += buf;
	}
	else
	{
		strMessage += "Error\n";

		GPErrorCode errorCode;
		gpGetErrorCode(connection, &errorCode);
		if(errorCode == GP_NEWUSER_BAD_PASSWORD)
			strMessage += "Invalid password";
		else if(errorCode == GP_NEWUSER_BAD_NICK)
		{
			strMessage += "Profile already exists: ";
			itoa(arg->profile, buf, 10);
			strMessage += buf;
		}
		else if(errorCode == GP_NEWUSER_UNIQUENICK_INUSE)
			strMessage += "Uniquenick in use";
		else if(errorCode == GP_NEWUSER_UNIQUENICK_INVALID)
			strMessage += "Uniquenick invalid";
	}

	dlg->MessageBox(strMessage, "New User");
	GSI_UNUSED(param);
}

void RecvBuddyStatus(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyStatusArg * arg = (GPRecvBuddyStatusArg *)arg_;
	GPBuddyStatus status;

	char intValue[16];
	int index;
	CString string;
	int i;
	int count;
	bool online;
	static int onlinecount;

	//CHECK(gpGetBuddyStatusInfo(connection, arg->index, &status));
	CHECK(gpGetBuddyStatus(connection, arg->index, &status));

	if(status.status == GP_ONLINE)
	{
		string = "ON:";
		online = true;
	}
	sprintf(intValue, "%04d", arg->profile);
	string += intValue;

	count = dlg->m_buddies.GetCount();
	for(i = 0 ; i < count ; i++)
		if(dlg->m_buddies.GetItemData(i) == (DWORD)arg->profile)
			dlg->m_buddies.DeleteString(i);

	CString text;
	count = dlg->m_buddies.GetCount();
	for(i = 0 ; i < count ; i++)
	{
		dlg->m_buddies.GetText(i, text);
		if(text.Left(3).Compare("ON:") != 0)
			break;
	}
	index = dlg->m_buddies.InsertString(i, string);
	dlg->m_buddies.SetItemData(index, (DWORD)arg->profile);
	GSI_UNUSED(param);
}

void GetInfoResponse(GPConnection * connection, void * arg_, void * param)
{
	GPGetInfoResponseArg * arg = (GPGetInfoResponseArg *)arg_;

	int index;
	//GPBuddyStatusInfo statusInfo;
	GPBuddyStatus status;
	IN_ADDR addr;
	char intValue[16];
	addr.s_addr = 0;
	if(arg->result == GP_NO_ERROR)
	{
		// Get this guy's buddy status.
		///////////////////////////////
		if(arg->profile)
		{
			gpGetBuddyIndex(&dlg->m_connection, arg->profile, &index);
			if(index != -1)
			{
				//gpGetBuddyStatusInfo(&dlg->m_connection, index, &statusInfo);
				gpGetBuddyStatus(&dlg->m_connection, index, &status);
				//addr.S_un.S_addr = statusInfo.buddyIp;
				addr.S_un.S_addr = status.ip;
			}
		}
		else
		{
			index = -1;
		}

		// Set the stuff.
		/////////////////
		dlg->UpdateData();
		dlg->m_ifirstname = arg->firstname;
		dlg->m_ilastname = arg->lastname;
		dlg->m_inick = arg->nick;
		dlg->m_iuniquenick = arg->uniquenick;
		dlg->m_iemail = arg->email;
		dlg->m_iicquin = arg->icquin;
		dlg->m_ihomepage = arg->homepage;
		dlg->m_ibirthday = arg->birthday;
		dlg->m_ibirthmonth = arg->birthmonth;
		dlg->m_ibirthyear = arg->birthyear;
		dlg->m_isex = (arg->sex == GP_MALE)?"Male":((arg->sex == GP_FEMALE)?"Female":"Pat");
		dlg->m_icountrycode = arg->countrycode;
		dlg->m_izipcode = arg->zipcode;
		dlg->m_ilongitude = arg->longitude;
		dlg->m_ilatitude = arg->latitude;
		dlg->m_iplace = arg->place;
		dlg->m_ipmhomepage = (arg->publicmask & GP_MASK_HOMEPAGE)?TRUE:FALSE;
		dlg->m_ipmzipcode = (arg->publicmask & GP_MASK_ZIPCODE)?TRUE:FALSE;
		dlg->m_ipmcountrycode = (arg->publicmask & GP_MASK_COUNTRYCODE)?TRUE:FALSE;
		dlg->m_ipmbirthday = (arg->publicmask & GP_MASK_BIRTHDAY)?TRUE:FALSE;
		dlg->m_ipmsex = (arg->publicmask & GP_MASK_SEX)?TRUE:FALSE;
		dlg->m_ipmemail = (arg->publicmask & GP_MASK_EMAIL)?TRUE:FALSE;
		if(index != -1)
		{
			dlg->m_iaddress = inet_ntoa(addr);
			dlg->m_iaddress += ":";
			//itoa(statusInfo.buddyPort, intValue, 10);
			itoa(status.port, intValue, 10);
			dlg->m_iaddress += intValue;
			dlg->m_status = status.status;
			dlg->m_statusString = status.statusString;
			dlg->m_locationString = status.locationString;
			
			/*
			dlg->m_StatusState = statusInfo.statusState;
			dlg->m_RichStatus = statusInfo.richStatus;
			dlg->m_GameType = statusInfo.gameType;
			dlg->m_GameVariant = statusInfo.gameVariant;
			dlg->m_GameMapname = statusInfo.gameMapName;
			addr.s_addr = statusInfo.hostIp;
			dlg->m_HostIp = inet_ntoa(addr);
			addr.s_addr = statusInfo.hostPrivateIp;
			dlg->m_HostPrivateIp = inet_ntoa(addr);
			dlg->m_HostPort = statusInfo.hostPort;
			dlg->m_QueryPort = statusInfo.queryPort;
			
			for (int i = 0; i < dlg->m_SessionFlags.GetCount(); i++)
			{
				int flag = dlg->m_SessionFlags.GetItemData(i);
				if (statusInfo.sessionFlags & flag)
					dlg->m_SessionFlags.SetSel(i, TRUE);
				else
					dlg->m_SessionFlags.SetSel(i, FALSE);
			}
			*/
		}
		else
		{
			dlg->m_iaddress = "";
			dlg->m_StatusState = 0;
			//dlg->m_statusString = "";
			//dlg->m_locationString = "";
			
			dlg->m_RichStatus = "";
			dlg->m_GameType = "";
			dlg->m_GameVariant = "";
			dlg->m_GameMapname = "";
			dlg->m_HostIp = "0.0.0.0";
			dlg->m_HostPrivateIp = "0.0.0.0";
			dlg->m_HostPort = 0;
			dlg->m_QueryPort = 0;

			for (int i=0; i < dlg->m_SessionFlags.GetCount(); i++)
				dlg->m_SessionFlags.SetSel(0, FALSE);

		}
		dlg->UpdateData(FALSE);
	}
	else
	{
		dlg->MessageBox("GetInfo failed");
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void Error(GPConnection * connection, void * arg_, void * param)
{
	GPErrorArg * arg = (GPErrorArg *)arg_;

	if(arg->fatal == GP_FATAL)
	{
		MessageBox(NULL, arg->errorString, "Fatal Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
	}
	else
	{
		MessageBox(NULL, arg->errorString, "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

char whois[128];

void Whois(GPConnection * connection, void * arg_, void * param)
{
	GPGetInfoResponseArg * arg = (GPGetInfoResponseArg *)arg_;

	if(arg->result == GP_NO_ERROR)
	{
		dlg->UpdateData();
		switch(dlg->m_rnick)
		{
		// Nick.
		////////
		case 0:
			strcpy(whois, arg->nick);
			break;
		// Name.
		////////
		case 1:
			strcpy(whois, arg->firstname);
			strcat(whois, " ");
			strcat(whois, arg->lastname);
			break;
		// Email.
		/////////
		case 2:
			strcpy(whois, arg->email);
			break;
		}
	}
	else
	{
		dlg->MessageBox("Whois failed");
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void RecvBuddyMessage(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyMessageArg * arg = (GPRecvBuddyMessageArg *)arg_;

	dlg->UpdateData();
	gpGetInfo(connection, arg->profile, GP_CHECK_CACHE, GP_BLOCKING, Whois, NULL);

	char msg[4096];
	sprintf(msg, "%s: %s", whois, arg->message);
	char * str = gsiSecondsToString((time_t *)&arg->date);
	char date[256];
	strcpy(date, str);
	date[strlen(str) - 1] = '\0';

#if 0
	OutputDebugString(msg);
	OutputDebugString("\n");
#else
	if(MessageBox(NULL, msg, date, MB_YESNO | MB_APPLMODAL) == IDYES)
	{
		gpSendBuddyMessage(connection, arg->profile, "Thanks for the message!");
	}
#endif
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void RecvBuddyUTM(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyUTMArg * arg = (GPRecvBuddyUTMArg *)arg_;

	dlg->UpdateData();
	gpGetInfo(connection, arg->profile, GP_CHECK_CACHE, GP_BLOCKING, Whois, NULL);

	char msg[4096];
	sprintf(msg, "[UTM] %s: %s", whois, arg->message);
	char * str = gsiSecondsToString((time_t *)&arg->date);
	char date[256];
	strcpy(date, str);
	date[strlen(str) - 1] = '\0';

#if 0
	OutputDebugString(msg);
	OutputDebugString("\n");
#else
	if(MessageBox(NULL, msg, date, MB_YESNO | MB_APPLMODAL) == IDYES)
	{
		char* message = "Thanks for the message!";
		gpSendBuddyUTM(connection, arg->profile, message, 0);
	}
#endif
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void RecvBuddyRevoke(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyRevokeArg * arg = (GPRecvBuddyRevokeArg *)arg_;

	dlg->UpdateData();

	if (gpIsBuddy(connection, arg->profile))
	{
		char * str = gsiSecondsToString((time_t *)&arg->date);
		char date[256];
		strcpy(date, str);
		date[strlen(str) - 1] = '\0';

		CString message;
		message.Format("User %d has revoked their buddy authorization.", arg->profile);
		MessageBox(NULL, (LPCSTR)message, date, MB_OK);

		// Remove them from the UI list
		int index = 0;
		while (index < dlg->m_buddies.GetCount())
		{
			GPProfile profile = (GPProfile)dlg->m_buddies.GetItemData(index);
			if (profile == arg->profile)
			{
				dlg->m_buddies.DeleteString(index);
				break;
			}
			index++;
		}
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

GPProfileSearchMatch searchMatches[100];

void ProfileSearchResponse(GPConnection * connection, void * arg_, void * param)
{
	GPProfileSearchResponseArg * arg = (GPProfileSearchResponseArg *)arg_;

	// Check the results.
	/////////////////////
	if(arg->result == GP_NO_ERROR)
	{
		// Loop through the matches.
		////////////////////////////
		for(int i = 0 ; (i < arg->numMatches) && (i < (sizeof(searchMatches) / sizeof(searchMatches[0]))) ; i++)
		{
			// Add it to the list.
			//////////////////////
			dlg->m_results.InsertString(i, arg->matches[i].nick);
			dlg->m_results.SetItemData(i, (DWORD)arg->matches[i].profile);
			// Save the match.
			//////////////////
			memcpy(&searchMatches[i], &arg->matches[i], sizeof(GPProfileSearchMatch));
		}

		if(arg->more == GP_MORE)
		{
			if(dlg->MessageBox("More search results?", "Profile Search", MB_YESNO | MB_TASKMODAL) == IDNO)
			{
				arg->more = GP_DONE;
			}
		}
	}
	else
	{
		dlg->MessageBox("ProfileSearch failed");
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void RecvBuddyRequest(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyRequestArg * arg = (GPRecvBuddyRequestArg *)arg_;

	// Show who the buddy request is from.
	// PANTS|05.18.00
	//////////////////////////////////////
	gpGetInfo(connection, arg->profile, GP_CHECK_CACHE, GP_BLOCKING, Whois, NULL);
	CString message = "Buddy Request from ";
	message += whois;
	if(MessageBox(dlg->m_hWnd, arg->reason, message, MB_YESNO) == IDYES)
		gpAuthBuddyRequest(connection, arg->profile);
	else
		gpDenyBuddyRequest(connection, arg->profile);
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void RecvBuddyAuth(GPConnection * connection, void * arg_, void * param)
{
	GPRecvBuddyAuthArg * arg = (GPRecvBuddyAuthArg *)arg_;
	dlg->UpdateData();
	
	gpGetInfo(connection, arg->profile, GP_CHECK_CACHE, GP_BLOCKING, Whois, NULL);

	CString msg_for_requester = whois;
	msg_for_requester += ": Yo man, joo on ma buddy list";
	MessageBox(dlg->m_hWnd, msg_for_requester, "Received Buddy Auth", MB_ICONINFORMATION);
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void NewProfileResponse(GPConnection * connection, void * arg_, void * param)
{
	GPNewProfileResponseArg * arg = (GPNewProfileResponseArg *)arg_;

	if(arg->result == GP_NO_ERROR)
	{
		dlg->MessageBox("New profile created");
	}
	else
	{
		dlg->MessageBox("NewProfile failed");
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void IsValidEmailResponse(GPConnection * connection, void * arg_, void * param)
{
	char buffer[128];
	GPIsValidEmailResponseArg * arg = (GPIsValidEmailResponseArg *)arg_;

	if(arg->result == GP_NO_ERROR)
	{
		if(arg->isValid)
			sprintf(buffer, "%s is a valid e-mail address.", arg->email);
		else
			sprintf(buffer, "%s is NOT a valid e-mail address.", arg->email);
	}
	else
	{
		sprintf(buffer, "IsValidEmail failed");
	}

	dlg->MessageBox(buffer);
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void GetUserNicksResponse(GPConnection * connection, void * arg_, void * param)
{
	GPGetUserNicksResponseArg * arg = (GPGetUserNicksResponseArg *)arg_;

	if(arg->result == GP_NO_ERROR)
	{
		int i;
		for(i = 0 ; i < arg->numNicks ; i++)
		{
			dlg->m_results.InsertString(i, arg->nicks[i]);
			dlg->m_results.SetItemData(i, 0);
			memset(&searchMatches[i], 0, sizeof(GPProfileSearchMatch));
			strcpy(searchMatches[i].nick, arg->nicks[i]);
			strcpy(searchMatches[i].uniquenick, arg->uniquenicks[i]);
		}
	}
	else
	{
		dlg->MessageBox("GetUserNicksResponse failed");
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void SuggestUniqueNickResponse(GPConnection * connection, void * arg_, void * param)
{
	GPSuggestUniqueNickResponseArg * arg = (GPSuggestUniqueNickResponseArg *)arg_;

	if(arg->result == GP_NO_ERROR)
	{
		int i;
		for(i = 0 ; i < arg->numSuggestedNicks ; i++)
		{
			dlg->m_results.InsertString(i, arg->suggestedNicks[i]);
			dlg->m_results.SetItemData(i, 0);
			// don't get info for this one
			searchMatches[i].profile = -1;
		}
	}
	else
	{
		dlg->MessageBox("GetUserNicksResponse failed");
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void GetReverseBuddiesResponse(GPConnection * connection, void * arg_, void * param)
{
	GPGetReverseBuddiesResponseArg * arg = (GPGetReverseBuddiesResponseArg *)arg_;

	if(arg->result == GP_NO_ERROR)
	{
		int i;
		CString str;
		int rcode;
		GPProfileSearchMatch * match;

		for(i = 0 ; i < arg->numProfiles ; i++)
		{
			match = &arg->profiles[i];

			str.Format("Revoke %s [%s] (%s %s, %s, %d)?", match->nick, match->uniquenick, match->firstname, match->lastname, match->email, match->profile);
			rcode = dlg->MessageBox(str, NULL, MB_YESNOCANCEL);
			if(rcode == IDYES)
				gpRevokeBuddyAuthorization(connection, match->profile);
			else if(rcode == IDCANCEL)
				break;
		}
	}
	else
	{
		dlg->MessageBox("GetReverseBuddiesResponse failed");
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void RecvGameInvite(GPConnection * connection, void * arg_, void * param)
{
	GPRecvGameInviteArg * arg = (GPRecvGameInviteArg *)arg_;

	gpGetInfo(connection, arg->profile, GP_CHECK_CACHE, GP_BLOCKING, Whois, NULL);

	char msg[4096];
	sprintf(msg, "%s has invited you to play the game with product ID %d", whois, arg->productID);

	dlg->MessageBox(msg);
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void GetInfoForBuddyKeys(GPConnection * theConnection, void * theArg, void * theParam)
{
	GPGetInfoResponseArg *anArg = (GPGetInfoResponseArg *)theArg;
	GPGetInfoResponseArg **anInfoArg = (GPGetInfoResponseArg **)theParam;
	
	memcpy(*anInfoArg,anArg, sizeof(GPGetInfoResponseArg));
	GSI_UNUSED(theConnection);
}

void GetBuddyKeysCallback(GPConnection * theConnection, void * theArg, void * theParam)
{
	GPGetBuddyStatusInfoKeysArg *anArg = (GPGetBuddyStatusInfoKeysArg *)theArg;
	GPGetInfoResponseArg *anInfoArg = new GPGetInfoResponseArg;
	gpGetInfo(theConnection, anArg->profile, GP_CHECK_CACHE, GP_BLOCKING, GetInfoForBuddyKeys, (void *)&anInfoArg);

	CString aUniqueNick = anInfoArg->uniquenick;
	CString aBuddyeKeys = "";

	for (int i = 0; i < anArg->numKeys; i++)
	{
		if (i == 0)
		{
			aBuddyeKeys += anArg->keys[i];
			aBuddyeKeys += " = ";
			aBuddyeKeys += anArg->values[i];
		}
		else
		{
			aBuddyeKeys += "\n";
			aBuddyeKeys += anArg->keys[i];
			aBuddyeKeys += " = ";
			aBuddyeKeys += anArg->values[i];
		}
	}

	CString aMessage = "Buddy: " + aUniqueNick;
	aMessage += "\n";
	aMessage += aBuddyeKeys;
	
	dlg->MessageBox(aMessage, "Buddy Keys");
	delete anInfoArg;
	GSI_UNUSED(theParam);
}

void RegisterCdKeyCallback(GPConnection * connection, void * _arg, void * param)
{
	GPRegisterCdKeyResponseArg *arg = (GPRegisterCdKeyResponseArg *)_arg;
		
	if (arg->result == GP_NO_ERROR)
	{
		CString msg = "CDKey is now associated with your account";
		dlg->MessageBox(msg);
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

LPCSTR TypeToString(GPEnum type)
{
	switch(type)
	{
	case GP_TRANSFER_SEND_REQUEST:
		return "GP_TRANSFER_SEND_REQUEST";
	case GP_TRANSFER_ACCEPTED:
		return "GP_TRANSFER_ACCEPTED";
	case GP_TRANSFER_REJECTED:
		return "GP_TRANSFER_REJECTED";
	case GP_TRANSFER_NOT_ACCEPTING:
		return "GP_TRANSFER_NOT_ACCEPTING";
	case GP_TRANSFER_NO_CONNECTION:
		return "GP_TRANSFER_NO_CONNECTION";
	case GP_TRANSFER_DONE:
		return "GP_TRANSFER_DONE";
	case GP_TRANSFER_CANCELLED:
		return "GP_TRANSFER_CANCELLED";
	case GP_TRANSFER_LOST_CONNECTION:
		return "GP_TRANSFER_LOST_CONNECTION";
	case GP_TRANSFER_ERROR:
		return "GP_TRANSFER_ERROR";
	case GP_TRANSFER_THROTTLE:
		return "GP_TRANSFER_THROTTLE";
	case GP_FILE_BEGIN:
		return "GP_FILE_BEGIN";
	case GP_FILE_PROGRESS:
		return "GP_FILE_PROGRESS";
	case GP_FILE_END:
		return "GP_FILE_END";
	case GP_FILE_DIRECTORY:
		return "GP_FILE_DIRECTORY";
	case GP_FILE_SKIP:
		return "GP_FILE_SKIP";
	case GP_FILE_FAILED:
		return "GP_FILE_FAILED";
	}

	return "!!!!!!!BAD TYPE!!!!!!!";
}

void TransferCallback(GPConnection * connection, void * arg_, void * param)
{
	GPTransferCallbackArg * arg = (GPTransferCallbackArg *)arg_;
	static DWORD transferStart;
	char * name;
	char * path;
	GPEnum side;

	CString str;
	str.Format("%s - %d - %d - %s\n", TypeToString(arg->type), arg->index, arg->num, arg->message);
	OutputDebugString(str);

	switch(arg->type)
	{
	case GP_TRANSFER_SEND_REQUEST:
		transferStart = SDL_GetTicks();
		CHECK(gpAcceptTransfer(connection, arg->transfer, "I'd like your file"));
		break;

	case GP_FILE_END:
		gpGetTransferSide(connection, arg->transfer, &side);
		if(side == GP_TRANSFER_RECEIVER)
		{
			gpGetFileName(connection, arg->transfer, arg->index, &name);
			gpGetFilePath(connection, arg->transfer, arg->index, &path);
			rename(path, "file.ext");
		}
		break;

	case GP_TRANSFER_REJECTED:
	case GP_TRANSFER_NOT_ACCEPTING:
	case GP_TRANSFER_NO_CONNECTION:
	case GP_TRANSFER_DONE:
	case GP_TRANSFER_CANCELLED:
	case GP_TRANSFER_LOST_CONNECTION:
	case GP_TRANSFER_ERROR:
		gpFreeTransfer(connection, arg->transfer);
		break;
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

// Utility func for converting an error code to a string.
/////////////////////////////////////////////////////////
#define CtoS(code)  case code: string += " (" #code ")"; return;
void CGptestDlg::CodeToString(CString & string)
{
	GPErrorCode code;
	gpGetErrorCode(&m_connection, &code);

	switch(code)
	{
	CtoS(GP_GENERAL)
	CtoS(GP_PARSE)
	CtoS(GP_NOT_LOGGED_IN)
	CtoS(GP_BAD_SESSKEY)
	CtoS(GP_DATABASE)
	CtoS(GP_NETWORK)
	CtoS(GP_FORCED_DISCONNECT)
	CtoS(GP_CONNECTION_CLOSED)
	CtoS(GP_LOGIN)
	CtoS(GP_LOGIN_TIMEOUT)
	CtoS(GP_LOGIN_BAD_NICK)
	CtoS(GP_LOGIN_BAD_EMAIL)
	CtoS(GP_LOGIN_BAD_PASSWORD)
	CtoS(GP_LOGIN_BAD_PROFILE)
	CtoS(GP_LOGIN_PROFILE_DELETED)
	CtoS(GP_LOGIN_CONNECTION_FAILED)
	CtoS(GP_LOGIN_SERVER_AUTH_FAILED)
	CtoS(GP_LOGIN_BAD_UNIQUENICK)
	CtoS(GP_LOGIN_BAD_PREAUTH)
	CtoS(GP_NEWUSER)
	CtoS(GP_NEWUSER_BAD_NICK)
	CtoS(GP_NEWUSER_BAD_PASSWORD)
	CtoS(GP_NEWUSER_UNIQUENICK_INVALID)
	CtoS(GP_NEWUSER_UNIQUENICK_INUSE)
	CtoS(GP_UPDATEUI)
	CtoS(GP_UPDATEUI_BAD_EMAIL)
	CtoS(GP_NEWPROFILE)
	CtoS(GP_NEWPROFILE_BAD_NICK)
	CtoS(GP_NEWPROFILE_BAD_OLD_NICK)
	CtoS(GP_UPDATEPRO)
	CtoS(GP_UPDATEPRO_BAD_NICK)
	CtoS(GP_ADDBUDDY)
	CtoS(GP_ADDBUDDY_BAD_FROM)
	CtoS(GP_ADDBUDDY_BAD_NEW)
	CtoS(GP_ADDBUDDY_ALREADY_BUDDY)
	CtoS(GP_AUTHADD)
	CtoS(GP_AUTHADD_BAD_FROM)
	CtoS(GP_AUTHADD_BAD_SIG)
	CtoS(GP_STATUS)
	CtoS(GP_BM)
	CtoS(GP_BM_NOT_BUDDY)
	CtoS(GP_GETPROFILE)
	CtoS(GP_GETPROFILE_BAD_PROFILE)
	CtoS(GP_DELBUDDY)
	CtoS(GP_DELBUDDY_NOT_BUDDY)
	CtoS(GP_DELPROFILE)
	CtoS(GP_DELPROFILE_LAST_PROFILE)
	CtoS(GP_SEARCH)
	CtoS(GP_SEARCH_CONNECTION_FAILED)
	CtoS(GP_CHECK)
	CtoS(GP_CHECK_BAD_EMAIL)
	CtoS(GP_CHECK_BAD_NICK)
	CtoS(GP_CHECK_BAD_PASSWORD)
	CtoS(GP_REGISTERUNIQUENICK)
	CtoS(GP_REGISTERUNIQUENICK_TAKEN)
	CtoS(GP_REGISTERUNIQUENICK_RESERVED)
	CtoS(GP_REGISTERUNIQUENICK_BAD_NAMESPACE)
	CtoS(GP_REGISTERCDKEY_BAD_KEY)
	CtoS(GP_REGISTERCDKEY_ALREADY_SET)
	CtoS(GP_REGISTERCDKEY_ALREADY_TAKEN)
	}

	string += " (XXXUNKOWNXXX)";
	return;
}

/////////////////////////////////////////////////////////////////////////////
// CGptestDlg message handlers

CString GPServerDefault;
CString GPSearchServerDefault;

BOOL CGptestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	dlg = this;
	m_connection = NULL;

	GPServerDefault = GPConnectionManagerHostname;
	GPSearchServerDefault = GPSearchManagerHostname;
	
	UpdateData();

	CString str = "Closed";
	m_SessionFlags.InsertString(0, str);
	m_SessionFlags.SetItemData(0, GP_SESS_IS_CLOSED);

	str = "Open";
	m_SessionFlags.InsertString(1, str);
	m_SessionFlags.SetItemData(1, GP_SESS_IS_OPEN);

	str = "Has Password";
	m_SessionFlags.InsertString(2, str);
	m_SessionFlags.SetItemData(2, GP_SESS_HAS_PASSWORD);

	str = "Behind NAT";
	m_SessionFlags.InsertString(3, str);
	m_SessionFlags.SetItemData(3, GP_SESS_IS_BEHIND_NAT);

	str = "Ranked";
	m_SessionFlags.InsertString(4, str);
	m_SessionFlags.SetItemData(4, GP_SESS_IS_RANKED);

	SetTimer(1, PROCESS_TIME, NULL);
	SetTimer(2, 500, NULL);

	m_namespace.Format("%d", GSI_DEFAULT_NAMESPACE);
	m_productid = GSI_TEST_PRODUCTID;
	
	UpdateData(FALSE);
	FILE * file;
	file = fopen("login.txt", "rt");
	if(file)
	{
		char buffer[256];
		if(fgets(buffer, sizeof(buffer), file))
		{
			m_email = buffer;
			m_email.Remove('\n');
		}
		if(fgets(buffer, sizeof(buffer), file))
		{
			m_nick = buffer;
			m_nick.Remove('\n');
		}
		if(fgets(buffer, sizeof(buffer), file))
		{
			m_password = buffer;
			m_password.Remove('\n');
		}
		if(fgets(buffer, sizeof(buffer), file))
		{
			m_uniquenick = buffer;
			m_uniquenick.Remove('\n');
		}
		fclose(file);
	}
	
	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGptestDlg::OnDestroy() 
{
	if(m_connection)
		OnDestroyGP();

	UpdateData();
	FILE * file;
	file = fopen("login.txt", "wt");
	if(file)
	{
		fprintf(file, "%s\n%s\n%s\n%s", m_email, m_nick, m_password, m_uniquenick);
		fclose(file);
	}

	CDialog::OnDestroy();
}

void CGptestDlg::OnInitialize() 
{
	if(m_connection)
	{
		MessageBox("GP is already initialized");
		return;
	}


#ifdef GSI_COMMON_DEBUG
	// Define GSI_COMMON_DEBUG if you want to view the SDK debug output
	// Set the SDK debug log file, or set your own handler using gsSetDebugCallback
	gsSetDebugCallback(DebugCallback);

	// Set some debug levels
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Debug);
#endif

	// check that the game's backend is available
	GSIACResult result;
	GSIStartAvailableCheck(GSI_TEST_GAMENAME);
	while((result = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(result != GSIACAvailable)
	{
		MessageBox("The backend is not available\n");
		return;
	}

	UpdateData();

	if(m_namespace.IsEmpty() || !isdigit(m_namespace.GetAt(0)))
	{
		MessageBox("Enter a namespace of 0 or greater");
		return;
	}

 	CHECK(gpInitialize(&m_connection, m_productid, atoi(m_namespace), m_partnerid));
	CHECK(gpSetCallback(&m_connection, GP_RECV_BUDDY_STATUS, RecvBuddyStatus, NULL));
	CHECK(gpSetCallback(&m_connection, GP_RECV_BUDDY_AUTH, RecvBuddyAuth, NULL));
	CHECK(gpSetCallback(&m_connection, GP_ERROR, Error, NULL));
	CHECK(gpSetCallback(&m_connection, GP_RECV_BUDDY_MESSAGE, RecvBuddyMessage, NULL));
	CHECK(gpSetCallback(&m_connection, GP_RECV_BUDDY_REQUEST, RecvBuddyRequest, NULL));
	CHECK(gpSetCallback(&m_connection, GP_RECV_GAME_INVITE, RecvGameInvite, NULL));
	CHECK(gpSetCallback(&m_connection, GP_TRANSFER_CALLBACK, TransferCallback, NULL));
	CHECK(gpSetCallback(&m_connection, GP_RECV_BUDDY_UTM, RecvBuddyUTM, NULL));
	CHECK(gpSetCallback(&m_connection, GP_RECV_BUDDY_REVOKE, RecvBuddyRevoke, NULL));
}

void CGptestDlg::OnDestroyGP() 
{
	if(!m_connection)
		return;

	gpDestroy(&m_connection);
	m_connection = NULL;
}

void CGptestDlg::SetHost()
{
	if(m_server == 0)
		strcpy(GPConnectionManagerHostname, GPServerDefault);
	else if(m_server == 1)
		strcpy(GPConnectionManagerHostname, "aphexgp1");
	else if(m_server == 2)
		strcpy(GPConnectionManagerHostname, "aphexgp2");
#if 0
	else if(m_server == 3)
		strcpy(GPConnectionManagerHostname, "aphexapp3");
	else if(m_server == 4)
		strcpy(GPConnectionManagerHostname, "aphexapp4");
	else if(m_server == 5)
		strcpy(GPConnectionManagerHostname, "chat3");
	else if(m_server == 6)
		strcpy(GPConnectionManagerHostname, "chat4");
#endif
	else if(m_server == 7)
		strcpy(GPConnectionManagerHostname, "localhost");
	else if(m_server == 8)
		strcpy(GPConnectionManagerHostname, m_otherServer);
}

void CGptestDlg::OnConnect() 
{
	if(!m_connection)
		return;

	UpdateData();

	// Set which server to use.
	///////////////////////////
	SetHost();
	CHECK(gpConnect(&m_connection, m_nick, m_email, m_password, (GPEnum)m_firewall, (GPEnum)m_blocking, ConnectResponse, NULL));
}

void CGptestDlg::OnConnectunique() 
{
	if(!m_connection)
		return;

	UpdateData();

	// Set which server to use.
	///////////////////////////
	SetHost();

	CHECK(gpConnectUniqueNick(&m_connection, m_uniquenick, m_password, (GPEnum)m_firewall, (GPEnum)m_blocking, ConnectResponse, NULL));
}

void CGptestDlg::OnConnectpreauth() 
{
	if(!m_connection)
		return;

	UpdateData();

	// Set which server to use.
	///////////////////////////
	SetHost();

	CHECK(gpConnectPreAuthenticated(&m_connection, m_authtoken, m_partnerchallenge, (GPEnum)m_firewall, (GPEnum)m_blocking, ConnectResponse, NULL));
}

void CGptestDlg::OnDisconnect() 
{
	if(!m_connection)
		return;

	gpDisconnect(&m_connection);

	m_buddies.ResetContent();
}

void CGptestDlg::OnCheck() 
{
	if(!m_connection)
		return;

	UpdateData();
	CHECK(gpCheckUser(&m_connection, m_nick, m_email, m_password, (GPEnum)m_blocking, CheckResponse, NULL));
}

void CGptestDlg::OnNewuser() 
{
	if(!m_connection)
		return;

	UpdateData();
	SetHost();
CHECK(gpNewUser(&m_connection, m_nick, m_uniquenick, m_email, m_password, NULL, (GPEnum)m_blocking, NewUserResponse, NULL));
//	CHECK(gpConnectNewUser(&m_connection, m_nick, m_uniquenick, m_email, m_password, NULL, (GPEnum)m_firewall, (GPEnum)m_blocking, ConnectResponse, NULL));
}

void CGptestDlg::OnUpdate() 
{
	if(!m_connection)
		return;

	// Update some info.
	////////////////////
	UpdateData();
	CHECK(gpSetInfos(&m_connection, GP_EMAIL, m_email));
	CHECK(gpSetInfos(&m_connection, GP_NICK, m_nick));
	CHECK(gpSetInfos(&m_connection, GP_PASSWORD, m_password));
	CHECK(gpSetInfos(&m_connection, GP_UNIQUENICK, m_uniquenick));
}


void CGptestDlg::OnTimer(UINT nIDEvent) 
{
	if(!m_connection)
		return;

	if (m_InTimer == 1)
		return;
	m_InTimer = 1;

	if(nIDEvent == 1)
	{
		CHECK(gpProcess(&m_connection));		
	}

	if(nIDEvent == 2)
	{
		char buffer[256];
		GPErrorCode code;

		UpdateData();
		gpGetErrorCode(&m_connection, &code);
		sprintf(buffer, "0x%04X", code);
		m_code = buffer;
		CodeToString(m_code);
		gpGetErrorString(&m_connection, buffer);
		m_string = buffer;
		UpdateData(FALSE);
	}
	
	CDialog::OnTimer(nIDEvent);

	m_InTimer = 0;
}

void CGptestDlg::OnSelchangeBuddies() 
{
	if(!m_connection)
		return;

	// Get the profile for this item.
	/////////////////////////////////
	int index = m_buddies.GetCurSel();
	UpdateData();
	if(index != LB_ERR)
	{
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);

		// Get the info on this buddy.
		//////////////////////////////
		if(profile != -1)
			CHECK(gpGetInfo(&m_connection, profile, GP_CHECK_CACHE, (GPEnum)m_blocking, GetInfoResponse, NULL));
	}
}

void CGptestDlg::OnSet() 
{
	if(!m_connection)
		return;

	UpdateData();

	CHECK(gpSetStatus(&m_connection, (GPEnum)m_status, m_statusString, m_locationString));
}

void CGptestDlg::OnSend() 
{
	if(!m_connection)
		return;

	int index = m_buddies.GetCurSel();
	if(index != LB_ERR)
	{
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);
				
		// Send a message to this guy.
		//////////////////////////////
		UpdateData();
		CHECK(gpSendBuddyMessage(&m_connection, profile, m_message));
	}
}

void CGptestDlg::OnRefresh() 
{
	if(!m_connection)
		return;

	// Get the profile for this item.
	/////////////////////////////////
	int index = m_buddies.GetCurSel();
	UpdateData();
	if(index != LB_ERR)
	{
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);

		// Get the info on this buddy.
		//////////////////////////////
		CHECK(gpGetInfo(&m_connection, profile, GP_DONT_CHECK_CACHE, (GPEnum)m_blocking, GetInfoResponse, NULL));
	}
}

void CGptestDlg::OnSearch() 
{
	if(!m_connection)
		return;

	// Search.
	//////////
	UpdateData();
	dlg->m_results.ResetContent();
	CHECK(gpProfileSearch(&m_connection, m_snick, m_suniquenick, m_semail, m_sfirstname, m_slastname, m_sicquin, (GPEnum)m_blocking, ProfileSearchResponse, NULL));
}

void CGptestDlg::OnValidate() 
{
	if(!m_connection)
		return;

	UpdateData();
	CHECK(gpIsValidEmail(&m_connection, m_semail, (GPEnum)m_blocking, IsValidEmailResponse, NULL));
}

void CGptestDlg::OnNicks() 
{
	if(!m_connection)
		return;

	UpdateData();
	if(m_semail.IsEmpty())
	{
		MessageBox("Type in an email for which to find nicks");
		return;
	}
	dlg->m_results.ResetContent();
	CHECK(gpGetUserNicks(&m_connection, m_semail, m_password, (GPEnum)m_blocking, GetUserNicksResponse, NULL));
}

void CGptestDlg::OnSuggest() 
{
	if(!m_connection)
		return;

	UpdateData();
	if(m_suniquenick.IsEmpty())
	{
		MessageBox("Type in a desired unique nick");
		return;
	}
	dlg->m_results.ResetContent();
	CHECK(gpSuggestUniqueNick(&m_connection, m_suniquenick, (GPEnum)m_blocking, SuggestUniqueNickResponse, NULL));
}

void CGptestDlg::OnSelchangeResults() 
{
	if(!m_connection)
		return;

	int index = m_results.GetCurSel();
	if(index != LB_ERR)
	{
		// Get the profile.
		int profile = m_results.GetItemData(index);
		if(profile == -1)
			return;

		// Put in what we now.
		//////////////////////
		GPGetInfoResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		arg.profile = searchMatches[index].profile;
		strcpy(arg.nick, searchMatches[index].nick);
		strcpy(arg.uniquenick, searchMatches[index].uniquenick);
		strcpy(arg.firstname, searchMatches[index].firstname);
		strcpy(arg.lastname, searchMatches[index].lastname);
		strcpy(arg.email, searchMatches[index].email);
		GetInfoResponse(&m_connection, &arg, NULL);

		// Do a full get info if connected.
		///////////////////////////////////
		GPEnum connected;
		GPResult result = gpIsConnected(&m_connection, &connected);
		if((result == GP_NO_ERROR) && connected && profile)
		{
			CHECK(gpGetInfo(&m_connection, profile, GP_CHECK_CACHE, (GPEnum)m_blocking, GetInfoResponse, NULL));
		}
	}
}

void CGptestDlg::OnSendrequest() 
{
	if(!m_connection)
		return;

	int index = m_results.GetCurSel();
	if(index != LB_ERR)
	{
		UpdateData();
		GPProfile profile = (GPProfile)m_results.GetItemData(index);
		CHECK(gpSendBuddyRequest(&m_connection, profile, m_reason));
	}
}

void CGptestDlg::OnInfoCache() 
{
	if(!m_connection)
		return;

	UpdateData();
	if(m_infoCache)
	{
		CHECK(gpEnable(&m_connection, GP_INFO_CACHING));
	}
	else
	{
		CHECK(gpDisable(&m_connection, GP_INFO_CACHING));
	}
}

void CGptestDlg::OnDelete() 
{
	if(!m_connection)
		return;

	// Get the profile for this item.
	/////////////////////////////////
	int index = m_buddies.GetCurSel();
	if(index != LB_ERR)
	{
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);

		// Delete this buddy.
		/////////////////////
		CHECK(gpDeleteBuddy(&m_connection, profile));

		m_buddies.DeleteString(index);
	}
}

void CGptestDlg::OnSetinfo() 
{
	if(!m_connection)
		return;

	int mask;
	UpdateData();
	gpSetInfos(&m_connection, GP_FIRSTNAME, m_ifirstname);
	gpSetInfos(&m_connection, GP_LASTNAME, m_ilastname);
	gpSetInfoi(&m_connection, GP_ICQUIN, m_iicquin);
	gpSetInfos(&m_connection, GP_HOMEPAGE, m_ihomepage);
	gpSetInfos(&m_connection, GP_ZIPCODE, m_izipcode);
	if(!m_icountrycode.IsEmpty())
		gpSetInfos(&m_connection, GP_COUNTRYCODE, m_icountrycode);
	gpSetInfod(&m_connection, GP_BIRTHDAY, m_ibirthday, m_ibirthmonth, m_ibirthyear);
	gpSetInfos(&m_connection, GP_SEX, m_isex);
	mask = 0;
	if(m_ipmhomepage)
		mask |= GP_MASK_HOMEPAGE;
	if(m_ipmzipcode)
		mask |= GP_MASK_ZIPCODE;
	if(m_ipmcountrycode)
		mask |= GP_MASK_COUNTRYCODE;
	if(m_ipmbirthday)
		mask |= GP_MASK_BIRTHDAY;
	if(m_ipmsex)
		mask |= GP_MASK_SEX;
	if(m_ipmemail)
		mask |= GP_MASK_EMAIL;
	gpSetInfoMask(&m_connection, (GPEnum)mask);
}

static void DeleteResponseCallback(GPConnection *connection, void * _arg, void *param)
{
	GPDeleteProfileResponseArg *anArg = (GPDeleteProfileResponseArg *)_arg;
	
	if (anArg->result == GP_NO_ERROR)
	{
		AfxMessageBox("Profile delete success!", MB_ICONINFORMATION|MB_OK);
	}
	else
	{
		AfxMessageBox("Profile delete failed.", MB_ICONERROR|MB_OK);
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void CGptestDlg::OnDeletepro() 
{
	if(!m_connection)
		return;

	CHECK(gpDeleteProfile(&m_connection, DeleteResponseCallback, NULL));

}

void CGptestDlg::OnNewpro() 
{
	if(!m_connection)
		return;

	UpdateData();
	CHECK(gpNewProfile(&m_connection, m_newnick, m_replace?GP_REPLACE:GP_DONT_REPLACE, (GPEnum)m_blocking, NewProfileResponse, NULL));
}

void CGptestDlg::OnDeleteall() 
{
	if(!m_connection)
		return;

	int num;
	GPBuddyStatus status;

	while(1)
	{
		gpGetNumBuddies(&m_connection, &num);
		if(num == 0)
			return;
		gpGetBuddyStatus(&m_connection, 0, &status);
		gpDeleteBuddy(&m_connection, status.profile);
	}
}

void CGptestDlg::OnInvitePlayer() 
{
	if(!m_connection)
		return;

	UpdateData();

	// Get the profile for this item.
	/////////////////////////////////
	int index = m_buddies.GetCurSel();
	if(index != LB_ERR)
	{
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);

		// Invite this player.
		//////////////////////
		CHECK(gpInvitePlayer(&m_connection, profile, m_invitePlayerID, NULL));
	}
}

void report(const char * text)
{
	OutputDebugString(text);
	OutputDebugString("\n");
}

void CGptestDlg::OnReport() 
{
#ifdef _DEBUG
	gpProfilesReport(&m_connection, report);
#endif
}

void SendFilesCallback(GPConnection * connection, int index, const char ** path, const char ** name, void * param)
{
	if(index == 0)
	{
		*path = dlg->m_path;
		*name = dlg->m_name;
	}
	GSI_UNUSED(connection);
	GSI_UNUSED(param);
}

void CGptestDlg::OnSendFiles() 
{
	if(!m_connection)
		return;

	int index = m_buddies.GetCurSel();
	if(index != LB_ERR)
	{
		GPTransfer transfer;
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);

		// Send a file.
		///////////////
		UpdateData();
		CHECK(gpSendFiles(&m_connection, &transfer, profile, "Want this file?", SendFilesCallback, NULL));
	}
}

void CGptestDlg::OnChangeSearchServer() 
{
	if(!m_connection)
		return;

	UpdateData();

	if(!m_searchServer)
		strcpy(GPSearchManagerHostname, GPSearchServerDefault);
	else
		strcpy(GPSearchManagerHostname, m_searchServer);
}

void CGptestDlg::OnPublicmaskAll() 
{
	if(!m_connection)
		return;

	UpdateData();

	m_ipmhomepage = TRUE;
	m_ipmzipcode = TRUE;
	m_ipmcountrycode = TRUE;
	m_ipmbirthday = TRUE;
	m_ipmsex = TRUE;
	m_ipmemail = TRUE;

	UpdateData(FALSE);
}

void CGptestDlg::OnPublicmaskNone() 
{
	if(!m_connection)
		return;

	UpdateData();

	m_ipmhomepage = FALSE;
	m_ipmzipcode = FALSE;
	m_ipmcountrycode = FALSE;
	m_ipmbirthday = FALSE;
	m_ipmsex = FALSE;
	m_ipmemail = FALSE;

	UpdateData(FALSE);
}

void CGptestDlg::OnRevoke() 
{
	if(!m_connection)
		return;

	CHECK(gpGetReverseBuddies(&m_connection, (GPEnum)m_blocking, GetReverseBuddiesResponse, NULL));
}

void CGptestDlg::OnUTM() 
{
	if(!m_connection)
		return;

	int index = m_buddies.GetCurSel();
	if(index != LB_ERR)
	{
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);


		// Send a message to this guy.
		//////////////////////////////
		UpdateData();
		CHECK(gpSendBuddyUTM(&m_connection, profile, (LPCSTR)m_message, 0));
    }
}

void CGptestDlg::OnSetstatusinfo()
{
	if (!m_connection)
		return;

	UpdateData();

	/*
	GPEnum aStatusState = (GPEnum)m_StatusState;
	unsigned int aHostIp = inet_addr(m_HostIp);
	unsigned int aHostPrivateIp = inet_addr(m_HostPrivateIp);
	unsigned short aHostPort = m_HostPort;
	unsigned short aQueryPort = m_QueryPort;
	int *aSessionFlagsSelected = new int[m_SessionFlags.GetSelCount()];
	unsigned int sessFlags = 0;
	m_SessionFlags.GetSelItems(m_SessionFlags.GetSelCount(), aSessionFlagsSelected);

	for (int i = 0; i < m_SessionFlags.GetSelCount(); i++)
	{
		int itemData = m_SessionFlags.GetItemData(aSessionFlagsSelected[i]);
		sessFlags+= itemData;
	}

	gpSetStatusInfo(&m_connection, aStatusState, aHostIp, aHostPrivateIp, aQueryPort, aHostPort, sessFlags, (LPCSTR)m_RichStatus, 
		m_RichStatus.GetLength(), (LPCSTR)m_GameType, m_GameType.GetLength(), (LPCSTR)m_GameVariant, m_GameVariant.GetLength(), 
		(LPCSTR)m_GameMapname, m_GameMapname.GetLength());
	delete[] aSessionFlagsSelected;
	*/
}

void CGptestDlg::OnAddSetKey()
{
	if (!m_connection)
		return;

	UpdateData();
	/*
	char *keyValue = NULL;
	gpGetStatusInfoKeyVal(&m_connection, (LPCSTR)m_KeyName, &keyValue);
	if (keyValue)
		gpSetStatusInfoKey(&m_connection, (LPCSTR)m_KeyName, (LPCSTR)m_KeyValue);
	else
		gpAddStatusInfoKey(&m_connection, (LPCSTR)m_KeyName, (LPCSTR)m_KeyValue);
	*/
}

void CGptestDlg::OnGetKeyValue()
{
	if (!m_connection)
		return;

	UpdateData();
	/*
	char *keyValue = NULL;
	gpGetStatusInfoKeyVal(&m_connection, (LPCSTR)m_KeyName, &keyValue);
	if (keyValue)
		m_KeyValue = keyValue;
	*/
}

void CGptestDlg::OnGetBuddyKeys()
{
	if(!m_connection)
		return;

	/*
	int index = m_buddies.GetCurSel();
	if(index != LB_ERR)
	{
		GPProfile profile = (GPProfile)m_buddies.GetItemData(index);

		// Send a message to this guy.
		//////////////////////////////
		UpdateData();
		int bIndex;
		gpGetBuddyIndex(&m_connection, profile, &bIndex);

		CHECK(gpGetBuddyStatusInfoKeys(&m_connection, bIndex, GetBuddyKeysCallback, NULL));
	}
	*/
}

void CGptestDlg::OnDelKeyVal()
{
	if (!m_connection)
		return;

	UpdateData();
	/*
	char *keyValue = NULL;
	gpGetStatusInfoKeyVal(&m_connection, (LPCSTR)m_KeyName, &keyValue);
	if (keyValue)
		gpDelStatusInfoKeyA(&m_connection, (LPCSTR)m_KeyName);
	*/
}

void CGptestDlg::OnRegisterCdKey()
{
	if (!m_connection)
		return;

	UpdateData();
	
	// make sure the length isn't too much
	if (m_cdkey.GetLength() > GP_CDKEY_LEN)
		return;

	// Register CDkey
	CHECK(gpRegisterCdKey(&m_connection, (LPCSTR)m_cdkey, GP_BLOCKING, RegisterCdKeyCallback, NULL));
}

void CGptestDlg::OnGetBlocked()
{
    if(!m_connection)
    	return;

	m_blocklist.ResetContent();
    UpdateData();

    // Grab block list
    GPProfile profile;
    int numBlocked;
    CHECK(gpGetNumBlocked(&m_connection, &numBlocked));

    int i=0;
    for (i=0; i<numBlocked; i++)
    {
        CHECK(gpGetBlockedProfile(&m_connection, i, &profile));

        char intValue[16];
        sprintf(intValue, "%04d", profile);
        CString string = intValue;

        int index = m_blocklist.InsertString(i, string);
        m_blocklist.SetItemData(index, (DWORD)profile);
    }
}

void CGptestDlg::OnSelchangeBlocklist() 
{
    if(!m_connection)
        return;

    // Get the profile for this item.
    /////////////////////////////////
    int index = m_blocklist.GetCurSel();
    UpdateData();
    if(index != LB_ERR)
    {
        GPProfile profile = (GPProfile)m_blocklist.GetItemData(index);

        // Get the info on this block.
        //////////////////////////////
        if(profile != -1)
            CHECK(gpGetInfo(&m_connection, profile, GP_CHECK_CACHE, (GPEnum)m_blocking, GetInfoResponse, NULL));
    }
}

void CGptestDlg::OnAddBlock()
{
    if(!m_connection)
        return;

    int index = m_results.GetCurSel();
    if(index != LB_ERR)
    {
        UpdateData();
        GPProfile profile = (GPProfile)m_results.GetItemData(index);

        // If a buddy, remove from display list
        ///////////////////////////////////////
        int count = m_buddies.GetCount();
        int i;
        for(i = 0 ; i < count ; i++)
            if(m_buddies.GetItemData(i) == (DWORD)profile)
                m_buddies.DeleteString(i);

        CHECK(gpAddToBlockedList(&m_connection, profile));
    }
}

void CGptestDlg::OnRemoveBlock()
{
    if(!m_connection)
        return;

    // Get the profile for this item.
    /////////////////////////////////
    int index = m_blocklist.GetCurSel();
    if(index != LB_ERR)
    {
        GPProfile profile = (GPProfile)m_blocklist.GetItemData(index);

        // Remove this block.
        /////////////////////
        CHECK(gpRemoveFromBlockedList(&m_connection, profile));

        m_blocklist.DeleteString(index);
    }
}
