///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __SCIREPORT_H__
#define __SCIREPORT_H__


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "sci.h"
#include "../hashtable.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Constants
#define SC_REPORT_PROTOCOL 2

// NOTE: broke up entity index in two - for later versions we will want to send the 2 byte
// team index followed by the 4 byte team ID later in the stream

	// Protocol length settings
#define SC_REPORT_ROSTERDATA_LENGTH    20 // V1.0 - 16 byte connection id + 4 byte team index
#define SC_REPORT_PLAYERINDEX_LENGTH    2 // V1.0 - 2 bytes for a player index
#define SC_REPORT_TEAMINDEX_LENGTH		4 // V1.0 - 4 bytes for a team index
#define SC_REPORT_AUTHDATA_LENGTH      16 // V1.0 - length of auth data per player
#define SC_REPORT_ENTITYRESULT_LENGTH   4 // V1.0 - 4 byte enum value


#define SC_REPORT_ENTITY_NONE   0
#define SC_REPORT_ENTITY_PLAYER 1
#define SC_REPORT_ENTITY_TEAM   2

// (must match server)
#define SC_REPORT_FLAG_AUTHORITATIVE		(1<<0)
#define SC_REPORT_FLAG_NON_RELATIVE_RESULT  (1<<1)
#define SC_REPORT_FLAG_MATCHLESS_SESSION	(1<<2)
//#define SC_REPORT_FLAG_DEDICATED_HOST		(1<<3)

typedef enum
{
	SCIReportState_NONE,
	SCIReportState_ROSTER,
	//SCIReportState_AUTHDATA,
	//SCIReportState_RESTULTS,
	SCIReportState_GLOBALDATA,
	SCIReportState_PLAYERDATA,
	SCIReportState_TEAMDATA
} SCIReportState;

typedef enum
{
	SCIKeyType_INT32,
	SCIKeyType_INT16,
	SCIKeyType_BYTE,
	SCIKeyType_STRING,
	SCIKeyType_FLOAT,
	SCIKeyType_INT64
} SCIKeyType;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Structs
typedef struct SCIReportHeader
{
	gsi_u32  mProtocolVersion;      // To support future changes
	gsi_u32  mDeveloperVersion;
	gsi_u8   mChecksum[GS_CRYPT_MD5_HASHSIZE];        // Hash(session, player, team data)
	gsi_u32  mGameStatus;
	gsi_u32  mFlags;                // Flags for authoritative, final, etc.
	gsi_u16  mPlayerCount;          // Players in session
	gsi_u16  mTeamCount;            // Teams in session
	gsi_u16  mGameKeyCount;
	gsi_u16  mPlayerKeyCount;
	gsi_u16  mTeamKeyCount;
	gsi_u16  mReserved;             // pad, for 32-bit alignment
	gsi_u32  mRosterSectionLength;  // 
	gsi_u32  mAuthSectionLength;    // 
	gsi_u32  mResultsSectionLength; // 
	gsi_u32  mGameSectionLength;    // 
	gsi_u32  mPlayerSectionLength;  // 
	gsi_u32  mTeamSectionLength;    // 
} SCIReportHeader;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef struct SCIReportBuffer
{
	gsi_bool mIsStatic;

	gsi_u32 mLen;
	gsi_u32 mCapacity;
	gsi_u32 mPos;

	char * mData;
	
} SCIReportBuffer;

typedef struct SCIReport
{
	SCIReportState mReportState;

	gsi_i32 mCurEntityStartPos;  // where this entity's data begins
	gsi_u16 mCurEntityKeyCount;  // how many keys we've added for this entity

	//gsi_u32 mNumPlayersReported; // to check against expected count - not used
	gsi_u32 mNumResultsReported; // to check against expected count

	gsi_u32 mNumTeamsReported;	 // keeps track of the number of team IDs that have been reported
	gsi_u32 mTeamIds[SC_MAX_NUM_TEAMS];		// internal list of team IDs

	SCIReportBuffer mBuffer;
} SCIReport;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciCreateReport(gsi_u8 theSessionGuid[16], 
						 gsi_u32 theHeaderVersion, 
						 gsi_u32 thePlayerCount,
						 gsi_u32 theTeamCount,
						 SCIReport ** theReportOut);

SCResult sciDestroyReport(SCIReport *theReport);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ReportData functions
SCResult SC_CALL sciReportSetPlayerConnectionId(SCIReport * theReport, gsi_u32 thePlayerIndex, const gsi_u8 theConnectionId[SC_CONNECTION_GUID_SIZE]);
SCResult SC_CALL sciReportSetPlayerTeamIndex   (SCIReport * theReport, gsi_u32 thePlayerIndex, gsi_u32 theTeamIndex);
SCResult SC_CALL sciReportSetPlayerGameResult  (SCIReport * theReport, gsi_u32 thePlayerIndex, SCGameResult theGameResult);
SCResult SC_CALL sciReportSetPlayerAuthInfo    (SCIReport * theReport, gsi_u32 thePlayerIndex, const GSLoginCertificate * theCertificate, const gsi_u8 theAuthHash[16]);
SCResult SC_CALL sciReportSetTeamGameResult    (SCIReport * theReport, gsi_u32 theTeamIndex  , SCGameResult theGameResult);
SCResult SC_CALL sciReportSetAsMatchless       (SCIReport * theReport);

// Key/Value data
SCResult SC_CALL sciReportBeginGlobalData(SCIReport * theReport);
SCResult SC_CALL sciReportBeginPlayerData(SCIReport * theReport);
SCResult SC_CALL sciReportBeginTeamData  (SCIReport * theReport);

SCResult SC_CALL sciReportAddIntValue(SCIReport * theReport,
									  gsi_u16     theKeyId,
									  gsi_i32     theValue);
SCResult SC_CALL sciReportAddInt64Value(SCIReport * theReport,
										gsi_u16     theKeyId,
										gsi_i64     theValue);
SCResult SC_CALL sciReportAddShortValue(SCIReport * theReport,
									    gsi_u16     theKeyId,
									    gsi_i16     theValue);
SCResult SC_CALL sciReportAddByteValue(SCIReport * theReport,
									   gsi_u16     theKeyId,
									   gsi_i8      theValue);
SCResult SC_CALL sciReportAddFloatValue(SCIReport * theReport,
									    gsi_u16     theKeyId,
									    float       theValue);
SCResult SC_CALL sciReportAddStringValue(SCIReport *      theReport,
										 gsi_u16          theKeyId,
										 const gsi_char * theValue);

SCResult SC_CALL sciReportBeginNewTeam(SCIReport * theReport);
SCResult SC_CALL sciReportBeginNewPlayer(SCIReport * theReport);
SCResult SC_CALL sciReportEndEntity(SCIReport * theReport);
// Call when finished writing
SCResult SC_CALL sciReportEnd(SCIReport * theReport, gsi_bool isAuth, SCGameStatus theStatus);






///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // __SCREPORT_H__
