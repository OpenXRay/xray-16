#ifndef CRYPTO_API_DEF_INCLUDED
#define CRYPTO_API_DEF_INCLUDED

#ifdef CRYPTO_AS_SHARED_LIBRARY

#	ifdef	CRYPTO_BUILD
#		define	CRYPTO_API	__declspec(dllexport)
#	else
#		define	CRYPTO_API	__declspec(dllimport)
#	endif //#ifdef	CRYPTO_BUILD

#else  //if RELEASE compilation - static
#	define	CRYPTO_API
#endif //CRYPTO_AS_SHARED_LIBRARY

#endif //#ifndef CRYPTO_API_DEF_INCLUDED