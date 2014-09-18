#if !defined(AFX_SETUPDLG_H__36B5B6D7_690D_4628_92E4_96B2CAE34B82__INCLUDED_)
#define AFX_SETUPDLG_H__36B5B6D7_690D_4628_92E4_96B2CAE34B82__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SetupDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSetupDlg dialog
#include "../../voice2/gv.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const unsigned int MAX_DEVICES = 10;

struct VoiceSetupInfo
{
	GVDevice      m_PlaybackDevice;
	GVDevice      m_CaptureDevice;
	GVDeviceInfo  m_DeviceInfoArray[MAX_DEVICES];
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class CSetupDlg : public CDialog
{
public:
	VoiceSetupInfo* m_SetupInfo;

	void StartSelCaptureDevice();
	void StartSelPlaybackDevice();


	// MFC STUFF BELOW

// Construction 
public:
	CSetupDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetupDlg)
	enum { IDD = IDD_SETUPDIALOG };
	CSliderCtrl	m_ActivateLevel;
	CProgressCtrl	m_VoiceLevelCtrl;
	CStatic	m_IsSpeakingCtrl;
	CComboBox	m_PlaybackCombo;
	CComboBox	m_CaptureCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetupDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetupDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangeCaptureCombo();
	afx_msg void OnSelChangePlaybackCombo();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETUPDLG_H__36B5B6D7_690D_4628_92E4_96B2CAE34B82__INCLUDED_)
