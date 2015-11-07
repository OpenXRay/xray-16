#ifndef CRYPTO_API_DEF_INCLUDED
#define CRYPTO_API_DEF_INCLUDED

#	ifdef	CRYPTO_BUILD
#		define	CRYPTO_API	__declspec(dllexport)
#	else
#		define	CRYPTO_API	__declspec(dllimport)
#	endif //#ifdef	CRYPTO_BUILD

#endif //#ifndef CRYPTO_API_DEF_INCLUDED