#include "stdafx.h"
#include "xr_cda.h"

CCDA::CCDA( ) {
	retLen			= sizeof(retStr);
	dwCurTrack		= 0;
	lKeepTime		= 0;
	lTotalTime		= 0;
	bWorking		= false;
	bPaused			= false;
}


CCDA::~CCDA( ) {
	Stop			( );
	Close			( );
}

void CCDA::Open( )	{
	Close			( );
	mciSendString	( "open cdaudio shareable wait",	retStr, retLen, 0 );
	mciSendString	( "set cdaudio door closed",		retStr, retLen, 0 );
	mciSendString	( "set cdaudio time format tmsf",	retStr, retLen, 0 );
	mciSendString	( "set cdaudio audio_all",			retStr, retLen, 0 );
}

CDA_STATE CCDA::GetState( ) 
{
	err	= mciSendString( "status cdaudio mode", retStr, retLen, NULL );
	if ( err == 0 ){
		if ( !stricmp( retStr, "not ready") )		return CDA_STATE_NOTREADY;
		if ( !stricmp( retStr, "paused"	) )			return CDA_STATE_PAUSE;
		if ( !stricmp( retStr, "playing"	) )		return CDA_STATE_PLAY;
		if ( !stricmp( retStr, "stopped"	) )		return CDA_STATE_STOP;
		if ( !stricmp( retStr, "open"	) )			return CDA_STATE_OPEN;
	}
	return CDA_STATE_NOTREADY;
}

void CCDA::SetTrack( int track )
{
	dwCurTrack		= 0;
	lTotalTime		= 0;
	char			ch[64];
	sprintf			( ch, "status cdaudio length track %d", track );
	ZeroMemory		( retStr, sizeof(retStr) );
	err	= mciSendString( ch, retStr, retLen, NULL );
	if ( err )		return;

	dwCurTrack		= track;
	int				mm, ss, msec;
	sscanf			( retStr, "%d:%d:%d", &mm, &ss, &msec );
	lTotalTime		= mm*60000 + ss*1000 + msec + 2000;
	Stop			( );
}

void CCDA::Close( )
{
	CDA_STATE state			= GetState( );
	if ( (state == CDA_STATE_PAUSE) || (state == CDA_STATE_PLAY) ){
		mciSendString		( "stop cdaudio", retStr, retLen, 0 );
	}
	mciSendString			( "close cdaudio", retStr, retLen, 0 );
	lKeepTime		= 0;
	bWorking		= false;
	bPaused			= false;
}

void CCDA::Play( )
{
	char			ch[64];
	bWorking		= false;
	CDA_STATE state	= GetState( );
	if ( (state != CDA_STATE_NOTREADY) ){
		if (bPaused){
			sprintf	( ch, "play cdaudio to %d", dwCurTrack + 1 );
			err = mciSendString	( ch, retStr, retLen, 0 );
			if ( err != 0 ){
				sprintf( ch, "play cdaudio" );
				err = mciSendString	( ch, retStr, retLen, 0 );
			}
			if ( err == 0 ){
				bWorking	= true;
				bPaused		= false;
			}
			return;
		}
		if ( dwCurTrack ){
			lKeepTime= lTotalTime;
			sprintf	( ch, "play cdaudio from %d to %d", dwCurTrack, dwCurTrack + 1 );
			err = mciSendString	( ch, retStr, retLen, 0 );
			if ( err != 0 ){
				sprintf( ch, "play cdaudio from %d", dwCurTrack );
				err = mciSendString	( ch, retStr, retLen, 0 );
			}
			if (err == 0){
				lKeepTime	= lTotalTime;
				bWorking	= true;
			}
		}
	}
}

void CCDA::Stop( )
{
	mciSendString	( "stop cdaudio", retStr, retLen, 0 );
	lKeepTime		= 0;
	bWorking		= false;
}

void CCDA::Pause( )
{
	mciSendString	( "pause cdaudio", retStr, retLen, 0 );
	bWorking		= false;
	bPaused			= true;
}


void CCDA::OnMove( )
{
	if ( bWorking ){
		if (lKeepTime <= 0){
			CDA_STATE state	= GetState( );
			if (state == CDA_STATE_STOP) Play ( );
			if (state != CDA_STATE_STOP) bWorking	= false;
		}else{
			if (!bPaused) lKeepTime -= u32(Device.fTimeDelta*1000);
		}
	}
}
