// trackDlg.cpp : implementation file
//

#include "stdafx.h"
#include "track.h"
#include "trackDlg.h"

#include "../gstats.h"
#include "../../common/gsAvailable.h"
#include "../../ghttp/ghttp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TIMER_ONE_SECOND     100
#define TIMER_THINK          101

/////////////////////////////////////////////////////////////////////////////
// CTrackDlg dialog

CTrackDlg::CTrackDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrackDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrackDlg)
	m_info = _T("Ready");
	m_best100 = _T("");
	m_best200 = _T("");
	m_best50 = _T("");
	m_top100 = _T("");
	m_top200 = _T("");
	m_top50 = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTrackDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrackDlg)
	DDX_Control(pDX, IDC_PROGRESS, m_progress);
	DDX_Text(pDX, IDC_INFO, m_info);
	DDX_Text(pDX, IDC_BEST_100, m_best100);
	DDX_Text(pDX, IDC_BEST_200, m_best200);
	DDX_Text(pDX, IDC_BEST_50, m_best50);
	DDX_Text(pDX, IDC_TOP_100, m_top100);
	DDX_Text(pDX, IDC_TOP_200, m_top200);
	DDX_Text(pDX, IDC_TOP_50, m_top50);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTrackDlg, CDialog)
	//{{AFX_MSG_MAP(CTrackDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOGOUT, OnLogout)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_START_50, OnStart50)
	ON_BN_CLICKED(IDC_START_100, OnStart100)
	ON_BN_CLICKED(IDC_START_200, OnStart200)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrackDlg message handlers

CTrackDlg * Dlg;

GHTTPBool PlayerBestTimesPageCompleted
(
	GHTTPRequest request,
	GHTTPResult result,
	char * buffer,
	GHTTPByteCount bufferLen,
	void * param
)
{
	if(result == GHTTPSuccess)
	{
		int bestTimes[3];
		int rcode;

		// Scan out the best times.
		///////////////////////////
		rcode = sscanf(buffer, "%d %d %d", &bestTimes[0], &bestTimes[1], &bestTimes[2]);
		if(rcode == 3)
		{
			Dlg->UpdateData();
			Dlg->m_best50.Format("%0.3f", bestTimes[0] / 1000.0);
			Dlg->m_best100.Format("%0.3f", bestTimes[1] / 1000.0);
			Dlg->m_best200.Format("%0.3f", bestTimes[2] / 1000.0);
			Dlg->UpdateData(FALSE);
		}
	}

	return GHTTPTrue;
}

GHTTPBool TopTimePageCompleted
(
	GHTTPRequest request,
	GHTTPResult result,
	char * buffer,
	GHTTPByteCount bufferLen,
	void * param
)
{
	if(result == GHTTPSuccess)
	{
		int time = atoi(buffer);
		if(time)
		{
			CString str;
			str.Format("%0.3f", time / 1000.0);

			int event = (int)param;
			Dlg->UpdateData();
			if(event == 1)
				Dlg->m_top50 = str;
			else if(event == 2)
				Dlg->m_top100 = str;
			else if(event == 3)
				Dlg->m_top200 = str;
			Dlg->UpdateData(FALSE);
		}
	}

	return GHTTPTrue;
}

void CTrackDlg::SetupUser()
{
	CString title;
	title.Format("track - best time (%s - %s - %d)", m_loginDlg.m_email, m_loginDlg.m_nick, m_loginDlg.m_profile);
	SetWindowText(title);

	CString url;
	int event;
	url.Format("http://sdkdev.gamespy.com/games/st_highscore/web/playertimes.asp?pid=%d", m_loginDlg.m_profile);
	ghttpGet(url, GHTTPFalse, PlayerBestTimesPageCompleted, NULL);
	for(event = 1 ; event <= 3 ; event++)
	{
		url.Format("http://sdkdev.gamespy.com/games/st_highscore/web/top_%d.txt", event);
		ghttpGet(url, GHTTPFalse, TopTimePageCompleted, (void *)event);
	}

	m_start = 0;
	m_count = -1;
	m_racing = FALSE;
	m_event = EVENT_NONE;
	m_numSteps = 0;
	m_step = NONE;
}

BOOL CTrackDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	int rcode;
	CString str;

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	Dlg = this;

	// Use the development system.
	//////////////////////////////
	strcpy(StatsServerHostname, "sdkdev.gamespy.com");

	// Set the gamename and secret key.
	///////////////////////////////////
	strcpy(gcd_gamename, "st_highscore");
	gcd_secret_key[0] = 'K';
	gcd_secret_key[1] = 'S';
	gcd_secret_key[2] = '3';
	gcd_secret_key[3] = 'p';
	gcd_secret_key[4] = '2';
	gcd_secret_key[5] = 'Q';

	// Perform the availability check
	//////////////////////////////////////
	GSIACResult aResult = GSIACWaiting;
	GSIStartAvailableCheck(gcd_gamename);
	while(aResult == GSIACWaiting)
	{
		aResult = GSIAvailableCheckThink();
		Sleep(50);
	}

	// Init the connection to the backend.
	//////////////////////////////////////
	rcode = InitStatsConnection(0);
	if(rcode != GE_NOERROR)
	{
		str.Format("Failed to connect to the stats server (%d).", rcode);
		MessageBox(str);
		PostQuitMessage(1);
		return TRUE;
	}

	// Login the user (actually just verifying the login).
	//////////////////////////////////////////////////////
	if(m_loginDlg.DoModal() != IDOK)
	{
		PostQuitMessage(1);
		return TRUE;
	}

	SetTimer(TIMER_ONE_SECOND, 1000, NULL);
	SetTimer(TIMER_THINK, 50, NULL);

	SetupUser();

	return TRUE;
}

void CTrackDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this);

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

HCURSOR CTrackDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTrackDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	if(IsStatsConnected())
		CloseStatsConnection();

	ghttpCleanup();
}

void CTrackDlg::OnLogout() 
{
	ShowWindow(SW_HIDE);
	if(m_loginDlg.DoModal() != IDOK)
		PostQuitMessage(1);
	SetupUser();
	ShowWindow(SW_SHOW);
}

void CTrackDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == TIMER_ONE_SECOND)
	{
		if(m_count)
		{
			UpdateData(TRUE);

			if(m_count == 3)
				m_info = "READY";
			else if(m_count == 2)
				m_info = "SET";
			else if(m_count == 1)
				m_info = "GO";

			UpdateData(FALSE);

			if(m_count == 1)
			{
				m_numSteps = 0;
				m_step = NONE;
				m_racing = TRUE;
				m_start = GetTickCount();
			}

			m_count--;
		}
	}
	else if(nIDEvent == TIMER_THINK)
	{
		ghttpThink();
	}

	CDialog::OnTimer(nIDEvent);
}

void CTrackDlg::ReportStats(DWORD time)
{
	char response[33];

	NewGame(1);
	BucketStringOp(NULL, "hostname", bo_set, (char *)(LPCSTR)m_loginDlg.m_nick, bl_server, 0);
	BucketIntOp(NULL, "event", bo_set, m_event, bl_server, 0);

	NewPlayer(NULL, 0, (char *)(LPCSTR)m_loginDlg.m_nick);
	BucketIntOp(NULL, "time", bo_set, time, bl_player, 0);
	BucketIntOp(NULL, "pid", bo_set, m_loginDlg.m_profile, bl_player, 0);
	GenerateAuth(GetChallenge(NULL), (char *)(LPCSTR)m_loginDlg.m_password, response);
	BucketStringOp(NULL, "auth", bo_set, response, bl_player, 0);

	SendGameSnapShot(NULL, NULL, SNAP_FINAL);
	FreeGame(NULL);
}

BOOL CTrackDlg::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN)
	{
		int nChar = pMsg->wParam;
		if((nChar == 'Z') || (nChar == 'X'))
		{
			if((pMsg->lParam & 0xFFFF) == 1)
			{
				CString str;
				BOOL stepped = FALSE;

				if((nChar == 'Z') && (m_step != LEFT))
				{
					m_step = LEFT;
					m_numSteps++;
					stepped = TRUE;
				}
				else if ((nChar == 'X') && (m_step != RIGHT))
				{
					m_step = RIGHT;
					m_numSteps++;
					stepped = TRUE;
				}

				if(stepped && m_racing)
				{
					m_progress.SetPos(m_numSteps);
					if(m_numSteps == m_totalSteps)
					{
						DWORD diff;

						m_racing = FALSE;
						diff = (GetTickCount() - m_start);
						str.Format("%0.3fs\n", diff / 1000.0);
						OutputDebugString(str);
						MessageBox(str);

						UpdateData();

						m_info = "DONE";

						// Report the stats.
						////////////////////
						ReportStats(diff);

						// Update best time(s) if needed.
						/////////////////////////////////
						CString * topStr;
						CString * bestStr;
						DWORD topTime;
						DWORD bestTime;
						if(m_event == EVENT_50)
						{
							topStr = &m_top50;
							bestStr = &m_best50;
						}
						else if(m_event == EVENT_100)
						{
							topStr = &m_top100;
							bestStr = &m_best100;
						}
						else
						{
							topStr = &m_top200;
							bestStr = &m_best200;
						}

						topTime = (DWORD)(atof(*topStr) * 1000);
						bestTime = (DWORD)(atof(*bestStr) * 1000);
						if(!bestTime || (diff < bestTime))
						{
							bestStr->Format("%0.3f", diff / 1000.0);
							if(diff < topTime)
								*topStr = *bestStr;
						}

						UpdateData(FALSE);

						m_event = EVENT_NONE;
					}
				}
			}

			return TRUE;
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CTrackDlg::OnStart50() 
{
	m_count = 3;
	m_totalSteps = RACE_STEPS_50;
	m_progress.SetRange(0, m_totalSteps);
	m_progress.SetPos(0);
	m_event = EVENT_50;
}

void CTrackDlg::OnStart100()
{
	m_count = 3;
	m_totalSteps = RACE_STEPS_100;
	m_progress.SetRange(0, m_totalSteps);
	m_progress.SetPos(0);
	m_event = EVENT_100;
}

void CTrackDlg::OnStart200() 
{
	m_count = 3;
	m_totalSteps = RACE_STEPS_200;
	m_progress.SetRange(0, m_totalSteps);
	m_progress.SetPos(0);
	m_event = EVENT_200;
}
