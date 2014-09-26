// stdafx.cpp : source file that includes just the standard includes
// xrSound.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#ifdef __BORLANDC__
#	pragma comment(lib,	"eaxB.lib"			)
#	pragma comment(lib,	"vorbisfileB.lib"	)
#	pragma comment(lib,	"xrCoreB.lib"		)
#	pragma comment(lib,	"EToolsB.lib"		)
#	pragma comment(lib,	"OpenAL32B.lib"		)
#	pragma comment(lib,	"dsoundb.lib" 		)
//#	pragma comment(lib,	"xrapi.lib" 		)
#else
#	pragma comment(lib,	"eax.lib"			)
#	pragma comment(lib,	"xrCore.lib"		)
#	pragma comment(lib,	"xrCDB.lib"			)
#	pragma comment(lib,	"dsound.lib" 		)
#	pragma comment(lib,	"xrapi.lib" 		)
#endif

