// ladderTrack.h : main header file for the LADDERTRACK application
//

#if !defined(AFX_LADDERTRACK_H__E9856F44_580A_48C0_ABFF_6FFA9BA944A3__INCLUDED_)
#define AFX_LADDERTRACK_H__E9856F44_580A_48C0_ABFF_6FFA9BA944A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// global defines used in the sample

#define SCRACE_GAMENAME		"sc_race"
#define SCRACE_SECRETKEY	"Zc0eM6"
#define SCRACE_GAMEID		1649
#define SCRACE_PRODUCTID	11030

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// This represents the player data structure in your game.
typedef struct SamplePlayerData
{
	// "Normal" game data
	gsi_u32 mProfileId;
	GSLoginCertificate mCertificate;
	GSLoginPrivateData mPrivateData;
	SCPeerCipher mPeerSendCipher; // for fast encryption
	SCPeerCipher mPeerRecvCipher; // for fast decryption
	
	SCInterfacePtr mStatsInterface;

	// Stats related data
	gsi_u8  mSessionId[SC_SESSION_GUID_SIZE];
	gsi_u8  mConnectionId[SC_CONNECTION_GUID_SIZE];
	gsi_u8  mStatsAuthdata[16];

	gsi_i32 mFrags;
	gsi_i32 mScore;
	gsi_i16 mDeaths;
	gsi_i16 mShots;
	gsi_i32 mTeam;
	
	// Obfuscated versions
	SCHiddenData mHiddenFrags;
	SCHiddenData mHiddenDeaths;
	SCHiddenData mHiddenShots;
	SCHiddenData mHiddenScore;

	// A simple way to block the sample's progress
	gsi_u32 mWaitCount; 

} SamplePlayerData;

extern SamplePlayerData gPlayerData;

/////////////////////////////////////////////////////////////////////////////
// CLadderTrackApp:
// See ladderTrack.cpp for the implementation of this class
//

class CLadderTrackApp : public CWinApp
{
public:
	CLadderTrackApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLadderTrackApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CLadderTrackApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LADDERTRACK_H__E9856F44_580A_48C0_ABFF_6FFA9BA944A3__INCLUDED_)
