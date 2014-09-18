#include "stdafx.h"

#include "xr_Client_BattlEye.h"
#include "xrMessages.h"
#include "Level.h"
#include "game_cl_base.h"

#ifdef BATTLEYE

extern int g_be_message_out;

BattlEyeClient::BattlEyeClient()
{
	m_module     = NULL;
	pfnRun       = NULL;
	pfnCommand   = NULL;
	pfnNewPacket = NULL;
	m_succefull  = false;
	InitDLL();
}

void BattlEyeClient::InitDLL()
{
	if ( !Level().battleye_system.InitDir() )
	{
		return;
	}

	m_module     = LoadLibrary( Level().battleye_system.GetClientPath() );
	if ( !m_module )
	{
		Msg( "! Error LoadLibrary  %s", BATTLEYE_CLIENT_DLL );
		return;
	}
//	string_path		path_dll;
//	GetModuleFileName( m_module, path_dll, sizeof(path_dll) );
//	Level().battleye_system.SetClientPath( path_dll );

	Init = (InitCl_t)GetProcAddress( m_module, "Init" );
	if ( !Init )
	{
		Msg( "! Error GetProcAddress <Init> from %s", BATTLEYE_CLIENT_DLL );
		if ( !FreeLibrary( m_module ) )
		{
			Msg( "! Error FreeLibrary for %s " BATTLEYE_CLIENT_DLL );
		}
		m_module = NULL;
		return;
	}

	m_succefull = Init(
		//		Level().battleye_system.auto_update,
		PrintMessage,
		SendPacket,
		&pfnExit,
		&pfnRun,
		&pfnCommand,
		&pfnNewPacket
		);

	if ( !m_succefull )
	{
		Msg( "! Error initialization of %s (function Init return false)", BATTLEYE_CLIENT_DLL );
		if ( !FreeLibrary( m_module ) )
		{
			Msg( "! Error FreeLibrary for %s", BATTLEYE_CLIENT_DLL );
		}
		m_module     = NULL;
		pfnRun       = NULL;
		pfnCommand   = NULL;
		pfnNewPacket = NULL;
		return;
	}
}

void BattlEyeClient::PrintMessage( char* message )
{
	//if ( g_be_message_out )
	{
		string512 text;
		sprintf_s( text, sizeof(text), "BattlEye Client: %s", message );
		Msg( "%s", text );
		
		if( g_be_message_out ) //==2
		{
			if ( Level().game )
			{
				Level().game->CommonMessageOut( text );
			}
		}
	}
}

void BattlEyeClient::SendPacket( void* packet, int len )
{
	NET_Packet P;
	P.w_begin( M_BATTLEYE );
	P.w_u32( len );
	P.w( packet, len );
	Level().Send( P, net_flags() );
}

bool BattlEyeClient::Run()
{
	R_ASSERT( m_module );
	R_ASSERT( pfnRun );
	return pfnRun();
}

void BattlEyeClient::Command( char* command )
{
	R_ASSERT( m_module );
	R_ASSERT( pfnCommand );
	pfnCommand( command );
}

void BattlEyeClient::NewPacket( void* packet, int len )
{
	R_ASSERT( m_module );
	R_ASSERT( pfnNewPacket );
	pfnNewPacket( packet, len );
}

bool BattlEyeClient::IsLoaded()
{
	return m_module != NULL;
}

BattlEyeClient::~BattlEyeClient()
{
	ReleaseDLL();
}

void BattlEyeClient::ReleaseDLL()
{
	if ( m_succefull )
	{
		if ( !pfnExit() )
		{
			Msg( "! Error unloading data in %s", BATTLEYE_CLIENT_DLL );
		}
	}
	if ( m_module )
	{
		if ( !FreeLibrary( m_module ) )
		{
			Msg( "! Error FreeLibrary for %s", BATTLEYE_CLIENT_DLL );
		}
	}
	m_module     = NULL;
	pfnRun       = NULL;
	pfnCommand   = NULL;
	pfnNewPacket = NULL;
	m_succefull  = false;
}

#endif // BATTLEYE
