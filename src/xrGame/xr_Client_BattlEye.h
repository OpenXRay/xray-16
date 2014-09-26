#ifndef	__XR_CLIENT_BATTLEYE_H__
#define __XR_CLIENT_BATTLEYE_H__

#include "battleye.h"

#ifdef BATTLEYE

class BattlEyeClient
{
	HMODULE		m_module;
	bool		m_succefull;
	
	typedef bool ( __cdecl* InitCl_t )
	(
		//		int		iAutoUpdate,
		//in func pointers
		void ( __cdecl *pfnPrintMessage )( char* ),
		void ( __cdecl *pfnSendPacket   )( void*, int ),
		//out func pointers
		bool ( __cdecl **ppfnExit       )( void ),
		bool ( __cdecl **ppfnRun        )( void ),
		void ( __cdecl **ppfnCommand    )( char* ),
		void ( __cdecl **ppfnNewPacket  )( void*, int )
	);

	InitCl_t Init;

	//in func 
	static	void __cdecl PrintMessage( char* );
	static	void __cdecl SendPacket  ( void*, int );

	//out func pointers
			bool ( __cdecl *pfnExit     )( void );
			bool ( __cdecl *pfnRun      )( void );
			void ( __cdecl *pfnCommand  )( char* );
			void ( __cdecl *pfnNewPacket)( void*, int );

			void		InitDLL();
			void		ReleaseDLL();

public:
			bool		Run       ( void );
			void		Command   ( char* );
			void		NewPacket ( void*, int );
	//////////////////////////////////////////////////////////////////////////
						BattlEyeClient();
						~BattlEyeClient();
			bool		IsLoaded();

}; // class BattlEyeClient

#endif // BATTLEYE

#endif // __XR_CLIENT_BATTLEYE_H__
