#ifndef	__XR_SERVER_BATTLEYE_H__
#define __XR_SERVER_BATTLEYE_H__

#include "battleye.h"

#ifdef BATTLEYE

class xrServer;
class xrClientData;

typedef bool  ( __cdecl* InitSrv_t )
	(
		char*	pstrGameVersion,
//		int		iAutoUpdate,
		//in func pointers
		void (__cdecl *pfnPrintMessage   )( char* ),
		void (__cdecl *pfnSendPacket     )( int, void*, int ),
		void (__cdecl *pfnKickPlayer     )( int, char* ),
		//out func pointers
		bool (__cdecl **ppfnExit         )( void ),
		bool (__cdecl **ppfnRun          )( void ),
		void (__cdecl **ppfnCommand      )( char* ),
		void (__cdecl **ppfnAddPlayer    )( int, char*, void*, int ),
		void (__cdecl **ppfnRemovePlayer )( int ),
		void (__cdecl **ppfnNewPacket    )( int, void*, int )
	);

class BattlEyeServer
{
	HMODULE			m_module;
	bool			m_succefull;
	InitSrv_t		Init;

	xrServer*		m_pServer;

	//in func
	static	void __cdecl PrintMessage (char *);
	static	void __cdecl SendPacket   (int, void *, int);
	static	void __cdecl KickPlayer   (int, char *);

	//out func pointers
	bool (__cdecl *pfnExit         )(void);
	bool (__cdecl *pfnRun          )(void);
	void (__cdecl *pfnCommand      )(char *);
	void (__cdecl *pfnAddPlayer    )(int, char *, void *, int);
	void (__cdecl *pfnRemovePlayer )(int);
	void (__cdecl *pfnNewPacket    )(int, void *, int);

	void ReleaseDLL();

public:
	bool Run          (void);
	void Command      (char *);
	void AddPlayer    (int, char *, void *, int);
	void RemovePlayer (int);
	void NewPacket    (int, void *, int);
	
	//////////////////////////////////////////////////////////////////////////
					BattlEyeServer( xrServer* Server );
					~BattlEyeServer();
			bool	IsLoaded();
			void	AddConnectedPlayers();
			void	AddConnected_OnePlayer( xrClientData* CL );

}; // class BattlEyeServer

#endif // BATTLEYE

#endif // __XR_SERVER_BATTLEYE_H__
