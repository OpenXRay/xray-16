#ifndef _MB_HELPERS_H_INCLUDED
#define _MB_HELPERS_H_INCLUDED

#define MAX_MB_CHARS 4096

typedef unsigned short int wide_char;

ENGINE_API unsigned short int mbhMulti2Wide
	( wide_char *WideStr , wide_char *WidePos , const unsigned short int WideStrSize , const char *MultiStr  );

__inline BOOL IsNeedSpaceCharacter( wide_char wc )
{
	return ( 
		( wc == 0x0020 )  ||

		( wc == 0x3000 )  ||

		( wc == 0xFF01 )  ||
		( wc == 0xFF0C )  ||
//		( wc == 0xFF0D )  ||
		( wc == 0xFF0E )  ||
		( wc == 0xFF1A )  ||
		( wc == 0xFF1B )  ||
		( wc == 0xFF1F )  ||

		( wc == 0x2026 )  ||

		( wc == 0x3002 )  ||
		( wc == 0x3001 ) 
	);
}

__inline BOOL IsBadStartCharacter( wide_char wc )
{
	return ( 
		IsNeedSpaceCharacter( wc ) ||

		( wc == 0x0021 )  ||
		( wc == 0x002C )  ||
//		( wc == 0x002D )  ||
		( wc == 0x002E )  ||
		( wc == 0x003A )  ||
		( wc == 0x003B )  ||
		( wc == 0x003F )  ||

		( wc == 0x0029 )  ||
		( wc == 0xFF09 )
	);
}

__inline BOOL IsBadEndCharacter( wide_char wc )
{
	return ( 
		( wc == 0x0028 )  ||

		( wc == 0xFF08 )  ||

		( wc == 0x4E00 ) 
	);
}

__inline BOOL IsAlphaCharacter( wide_char wc )
{
	return ( 
		( ( wc >= 0x0030 ) && ( wc <= 0x0039 ) ) ||
		( ( wc >= 0x0041 ) && ( wc <= 0x005A ) ) ||
		( ( wc >= 0x0061 ) && ( wc <= 0x007A ) ) ||

		( ( wc >= 0xFF10 ) && ( wc <= 0xFF19 ) ) ||
		( ( wc >= 0xFF21 ) && ( wc <= 0xFF3A ) ) ||
		( ( wc >= 0xFF41 ) && ( wc <= 0xFF5A ) )
	);
}


#endif // _MB_HELPERS_H_INCLUDED
