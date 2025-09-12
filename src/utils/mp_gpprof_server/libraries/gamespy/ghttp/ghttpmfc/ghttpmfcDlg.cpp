// ghttpmfcDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ghttpmfc.h"
#include "ghttpmfcDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "InetReg.h"

/////////////////////////////////////////////////////////////////////////////
// CGhttpmfcDlg dialog

CGhttpmfcDlg::CGhttpmfcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGhttpmfcDlg::IDD, pParent)
{
	char buffer[512];
	int rcode;
	FILE * fp;
	fp = fopen("url.cache", "rt");
	if(fp)
	{
		rcode = fread(buffer, 1, 511, fp);
		buffer[rcode] = '\0';
		m_url = buffer;
		fclose(fp);
	}
	else
		m_url = _T("http://planetquake.com/excessive");

	fp = fopen("saveas.cache", "rt");
	if(fp)
	{
		rcode = fread(buffer, 1, 511, fp);
		buffer[rcode] = '\0';
		m_saveAs = buffer;
		fclose(fp);
	}
	else
		m_saveAs = _T("file.html");

	//{{AFX_DATA_INIT(CGhttpmfcDlg)
	m_blocking = FALSE;
	m_completedCallback = TRUE;
	m_headers = _T("");
	m_progressCallback = TRUE;
	m_bufferSize = 0;
	m_userBuffer = FALSE;
	m_type = 0;
	m_file = _T("");
	m_soFar = _T("");
	m_state = -1;
	m_throttle = FALSE;
	m_headersRecv = _T("");
	m_status = _T("");
	m_stepThink = FALSE;
	m_postFile = FALSE;
	m_postObjects = _T("");
	m_postBytes = _T("");
	m_proxy = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGhttpmfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGhttpmfcDlg)
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Check(pDX, IDC_BLOCKING, m_blocking);
	DDX_Check(pDX, IDC_COMPLETED_CALLBACK, m_completedCallback);
	DDX_Text(pDX, IDC_HEADERS, m_headers);
	DDX_Check(pDX, IDC_PROGRESS_CALLBACK, m_progressCallback);
	DDX_Text(pDX, IDC_URL, m_url);
	DDX_Text(pDX, IDC_BUFFER_SIZE, m_bufferSize);
	DDX_Text(pDX, IDC_SAVE_AS, m_saveAs);
	DDX_Check(pDX, IDC_USER_BUFFER, m_userBuffer);
	DDX_Radio(pDX, IDC_GET_FILE, m_type);
	DDX_Text(pDX, IDC_FILE, m_file);
	DDX_Text(pDX, IDC_SO_FAR, m_soFar);
	DDX_Radio(pDX, IDC_HOST_LOOKUP, m_state);
	DDX_Check(pDX, IDC_THROTTLE, m_throttle);
	DDX_Text(pDX, IDC_HEADERS_RECV, m_headersRecv);
	DDX_Text(pDX, IDC_STATUS, m_status);
	DDX_Check(pDX, IDC_STEP_THINK, m_stepThink);
	DDX_Check(pDX, IDC_POST_FILE, m_postFile);
	DDX_Text(pDX, IDC_POST_OBJECTS, m_postObjects);
	DDX_Text(pDX, IDC_POST_BYTES, m_postBytes);
	DDX_Text(pDX, IDC_PROXY, m_proxy);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGhttpmfcDlg, CDialog)
	//{{AFX_MSG_MAP(CGhttpmfcDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_BN_CLICKED(IDC_CANCEL, OnCancel_)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_THROTTLE, OnThrottle)
	ON_BN_CLICKED(IDC_THINK, OnThink)
	ON_BN_CLICKED(IDC_SET_PROXY, OnSetProxy)
	ON_BN_CLICKED(IDC_IE_SETTINGS, OnIeSettings)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGhttpmfcDlg message handlers

BOOL CGhttpmfcDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_request = -1;
	m_memFile = NULL;
	SetTimer(50, 50, NULL);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGhttpmfcDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGhttpmfcDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

static GHTTPBool CompletedCallback
(
	GHTTPRequest request,
	GHTTPResult result,
	char * buffer,
	GHTTPByteCount bufferLen,
	void * param
)
{
	CGhttpmfcDlg * dlg = (CGhttpmfcDlg *)param;

	static char * resultStrings[] =
	{
		"GHTTPSuccess",
		"GHTTPOutOfMemory",
		"GHTTPBufferOverflow",
		"GHTTPParseURLFailed",
		"GHTTPHostLookupFailed",
		"GHTTPSocketFailed",
		"GHTTPConnectFailed",
		"GHTTPBadResponse",
		"GHTTPRequestRejected",
		"GHTTPUnauthorized",
		"GHTTPForbidden",
		"GHTTPFileNotFound",
		"GHTTPServerError",
		"GHTTPFileWriteFailed",
		"GHTTPFileReadFailed",
		"GHTTPFileIncomplete",
		"GHTTPFileToBig",
		"GHTTPEncryptionError",
		"GHTTPRequestCancelled"
	};

	CString msg;

	if( GHTTPSuccess == result )
	{
		msg = "File received successfully";
	}
	else
	{
		msg.Format("Error: (%d) ", result);
		if( result < 0 || result >= sizeof(resultStrings)/sizeof(resultStrings[0]) )
			msg += "Unknown - local error table may be out of date";
		else
			msg += resultStrings[(int)result];
	}

	dlg->MessageBox(msg);

	dlg->m_request = -1;

	return GHTTPTrue;

	GSI_UNUSED(bufferLen);
	GSI_UNUSED(buffer);
	GSI_UNUSED(request);
}

void ProgressCallback
(
	GHTTPRequest request,
	GHTTPState state,
	const char * buffer,
	GHTTPByteCount bufferLen,
	GHTTPByteCount bytesReceived,
	GHTTPByteCount totalSize,
	void * param
)
{
	CGhttpmfcDlg * dlg = (CGhttpmfcDlg *)param;

	dlg->UpdateData();

	dlg->m_state = state;

	//added new states for asynch DNS lookup - set all to HOST_LOOKUP for radio buttons
	if (dlg->m_state == GHTTPSocketInit || dlg->m_state == GHTTPHostLookup || dlg->m_state == GHTTPLookupPending)
		dlg->m_state = 0;

	if(state == GHTTPReceivingFile)
	{
		if(totalSize == -1)
			dlg->m_soFar.Format("%d bytes", bytesReceived);
		else
		{
			int percent = (int)((bytesReceived * 100) / totalSize);
			dlg->m_soFar.Format("%d%% (%d / %d bytes)", percent, bytesReceived, totalSize);
			dlg->m_progress.SetPos(percent);
		}
		if((dlg->m_type == 0) || (dlg->m_type == 4))
			dlg->m_file = buffer;
	}

	dlg->UpdateData(FALSE);

	GSI_UNUSED(bufferLen);
	GSI_UNUSED(request);
}

void PostCallback
(
	GHTTPRequest request,
	int bytesPosted,
	int totalBytes,
	int objectsPosted,
	int totalObjects,
	void * param
)
{
	CGhttpmfcDlg * dlg = (CGhttpmfcDlg *)param;

	dlg->UpdateData();

	int percent = ((bytesPosted * 100) / totalBytes);
	dlg->m_postObjects.Format("%d / %d", objectsPosted, totalObjects);
	dlg->m_postBytes.Format("%d%% (%d / %d)", percent, bytesPosted, totalBytes);

	dlg->UpdateData(FALSE);

	GSI_UNUSED(request);
}

void CGhttpmfcDlg::OnStart() 
{
	UpdateData();

	if(m_type == 0)
	{
		m_request = ghttpGetEx(
			m_url,
			m_headers,
			NULL,
			0,
			NULL,
			(GHTTPBool)m_throttle,
			(GHTTPBool)m_blocking,
			ProgressCallback,
			CompletedCallback,
			this);
	}
	else if(m_type == 1)
	{
		m_request = ghttpSaveEx(
			m_url,
			m_saveAs,
			m_headers,
			NULL,
			(GHTTPBool)m_throttle,
			(GHTTPBool)m_blocking,
			ProgressCallback,
			CompletedCallback,
			this);
	}
	else if(m_type == 2)
	{
		m_request = ghttpStreamEx(
			m_url,
			m_headers,
			NULL,
			(GHTTPBool)m_throttle,
			(GHTTPBool)m_blocking,
			ProgressCallback,
			CompletedCallback,
			this);
	}
	else if(m_type == 3)
	{
		m_request = ghttpHeadEx(
			m_url,
			m_headers,
			(GHTTPBool)m_throttle,
			(GHTTPBool)m_blocking,
			ProgressCallback,
			CompletedCallback,
			this);
	}
	else if(m_type == 4)
	{
		GHTTPPost post;
		post = ghttpNewPost();
		ghttpPostSetCallback(post, PostCallback, this);
		ghttpPostAddString(post, "test1", "bag");
		ghttpPostAddString(post, "test2", "test%test!@#) $(%^(*&test");
		if(m_postFile)
		{
			static int memFileSize = 100000;
			if(!m_memFile)
				m_memFile = (char *)malloc(memFileSize);
			memset(m_memFile, 0, memFileSize);
			sprintf(m_memFile, "steve");
			ghttpPostAddFileFromMemory(post, "memfile", m_memFile, memFileSize, "steve.txt", NULL);
			ghttpPostAddFileFromDisk(post, "diskfile", "../ghttpMain.c", "main.c", "text/html");
		}

#if 1
		m_request = ghttpGetEx(
			m_url,
			m_headers,
			NULL,
			0,
			post,
			(GHTTPBool)m_throttle,
			(GHTTPBool)m_blocking,
			ProgressCallback,
			CompletedCallback,
			this);
#else
		m_request = ghttpPostEx(
			m_url,
			m_headers,
			post,
			(GHTTPBool)m_throttle,
			(GHTTPBool)m_blocking,
			ProgressCallback,
			CompletedCallback,
			this);
#endif
	}

	if(m_request == -1)
		MessageBox("Unable to start request");
	else if(m_url.Left(8).Compare("https://") == 0)
		ghttpSetRequestEncryptionEngine(m_request, GHTTPEncryptionEngine_GameSpy);
	
	m_state = 0;
	m_soFar = "";
	m_file = "";
	m_postObjects = "";
	m_postBytes = "";

	UpdateData(FALSE);
}

void CGhttpmfcDlg::OnCancel_() 
{
	if(m_request >= 0)
		ghttpCancelRequest(m_request);
}

void CGhttpmfcDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == 50)
	{
		UpdateData();

		if(!m_stepThink)
			ghttpThink();

		if(m_request >= 0)
		{
			const char * statusString;
			int statusCode;
			statusString = ghttpGetResponseStatus(m_request, &statusCode);
			if(statusString)
				m_status.Format("%d: %s", statusCode, statusString);
			else
				m_status.Empty();

			const char * headers;
			headers = ghttpGetHeaders(m_request);
			if(headers)
				m_headersRecv = headers;
			else
				m_headersRecv.Empty();

			m_state = (int)ghttpGetState(m_request);
			//added new states for asynch DNS lookup - set all to HOST_LOOKUP for radio buttons
			if (m_state == GHTTPSocketInit || m_state == GHTTPHostLookup || m_state == GHTTPLookupPending)
				m_state = 0;
			UpdateData(FALSE);
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}

void CGhttpmfcDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	FILE * fp;
	fp = fopen("url.cache", "wt");
	if(fp)
	{
		fprintf(fp, "%s", m_url);
		fclose(fp);
	}

	fp = fopen("saveas.cache", "wt");
	if(fp)
	{
		fprintf(fp, "%s", m_saveAs);
		fclose(fp);
	}

	ghttpCleanup();

	if(m_memFile)
		free(m_memFile);
	m_memFile = NULL;
}

void CGhttpmfcDlg::OnThrottle() 
{
	UpdateData();

	if(m_request >= 0)
		ghttpSetThrottle(m_request, (GHTTPBool)m_throttle);
}

void CGhttpmfcDlg::OnThink() 
{
	if(m_request >= 0)
		ghttpThink();
}

void CGhttpmfcDlg::OnSetProxy() 
{
	UpdateData();

	ghttpSetProxy(m_proxy);
}

// Copied from JED's ProxyInfo.  Edited down for this app's purposes.
void CGhttpmfcDlg::OnIeSettings() 
{
	HKEY key;
	LONG result;
	DWORD type;
	DWORD data;
	DWORD len;
	CString str;
	int nStart;
	int nEnd;

	UpdateData();
	
	// Open the IE settings in the registry.
	////////////////////////////////////////
	result = RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_INTERNETSETTINGS, 0, KEY_READ, &key);
	if(SUCCEEDED(result))
	{
		// Is the proxy enabled?
		////////////////////////
		len = sizeof(DWORD);
		data = 0;
		result = RegQueryValueEx(key, REGSTR_VAL_PROXYENABLE, 0, &type, (LPBYTE)&data, &len);
		if(SUCCEEDED(result) && data)
		{		
			//----------------------------------------------------------------------
			//
			// The list of proxy servers to use
			//
			len = 0;
			result = RegQueryValueEx(key, REGSTR_VAL_PROXYSERVER, 0, &type, NULL, &len);
			result = RegQueryValueEx(key, REGSTR_VAL_PROXYSERVER, 0, &type, (LPBYTE)str.GetBuffer(len), &len);
			str.ReleaseBuffer();
			if(SUCCEEDED(result) && (len > 0))
			{
				// Find the http proxy.
				//
				// Use the same proxy for all protocols: "single_proxy_address:13"
				// read as: server:port
				// individualized protocols: "ftp=ftp_address;gopher=gopher_address:4;http=http_address:1;https=secure_address:2;socks=socks_address:5"
				// read as: protocol=server:port;protocol=server:port
				// If only protocol is to use proxy: "socks=socks_address:5"
				// Apparently, ports are optional - I would presume that you should revert to the default port when it is missing
				//
				// First search for "http=<server>[:port]".
				///////////////////////////////////////////
				nStart = str.Find("http=");
				if(nStart != -1)
				{
					nStart += 5;
					nEnd = str.Find(';', nStart);
					if(nEnd == -1)
						nEnd = str.GetLength();
				}
				else if(str.Find('=') == -1)
				{
					nStart = 0;
					nEnd = str.GetLength();
				}
				else
				{
					nStart = -1;
					nEnd = 0; //won't be used; need to initialize to prevent compiler warning
				}

				if((nStart != -1) && (nStart != nEnd))
					str = str.Mid(nStart, nEnd - nStart);
				else
					str = "";
			}
		}
		
		// Cleanup.
		///////////
		RegCloseKey(key);
	}

	m_proxy = str;

	UpdateData(FALSE);

	OnSetProxy();
}