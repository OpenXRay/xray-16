#ifndef xr_trimsH
#define xr_trimsH

// refs
struct xr_token;

#ifdef __BORLANDC__
	XRCORE_API 	AnsiString&	_Trim					( AnsiString& str );
	XRCORE_API 	LPCSTR		_GetItem				( LPCSTR src, int, AnsiString& p, char separator=',', LPCSTR ="", bool trim=true);
	XRCORE_API 	LPCSTR		_GetItems 				( LPCSTR src, int idx_start, int idx_end, AnsiString& dst, char separator );
	XRCORE_API 	LPCSTR		_CopyVal 				( LPCSTR src, AnsiString& dst, char separator=',' );
	XRCORE_API 	AnsiString	_ListToSequence			( const AStringVec& lst );
	XRCORE_API 	AnsiString	_ListToSequence2		( const AStringVec& lst );
	XRCORE_API 	void 		_SequenceToList			( AStringVec& lst, LPCSTR in, char separator=',' );
	XRCORE_API 	AnsiString&	_ReplaceItem 			( LPCSTR src, int index, LPCSTR new_item, AnsiString& dst, char separator );
	XRCORE_API 	AnsiString&	_ReplaceItems 			( LPCSTR src, int idx_start, int idx_end, LPCSTR new_items, AnsiString& dst, char separator );
	XRCORE_API 	AnsiString 	FloatTimeToStrTime		(float v, bool h=true, bool m=true, bool s=true, bool ms=false);
	XRCORE_API 	float 		StrTimeToFloatTime		(LPCSTR buf, bool h=true, bool m=true, bool s=true, bool ms=false);
#endif

XRCORE_API int		    	_GetItemCount			( LPCSTR , char separator=',');
XRCORE_API LPSTR	    	_GetItem				( LPCSTR, int, LPSTR, u32 const dst_size, char separator=',', LPCSTR ="", bool trim=true );

template <int count>
inline LPSTR	    		_GetItem				( LPCSTR src, int index, char (&dst)[count], char separator=',', LPCSTR def="", bool trim=true )
{
	return					_GetItem(src,index,dst,count,separator,def,trim);
}

XRCORE_API LPSTR	    	_GetItems				( LPCSTR, int, int, LPSTR, char separator=',');
XRCORE_API LPCSTR	    	_SetPos					( LPCSTR src, u32 pos, char separator=',' );
XRCORE_API LPCSTR	    	_CopyVal				( LPCSTR src, LPSTR dst, char separator=',' );
XRCORE_API LPSTR	    	_Trim					( LPSTR str );
XRCORE_API LPSTR	    	_TrimLeft				( LPSTR str );
XRCORE_API LPSTR	    	_TrimRight				( LPSTR str );
XRCORE_API LPSTR	    	_ChangeSymbol			( LPSTR name, char src, char dest );
XRCORE_API u32		    	_ParseItem				( LPCSTR src, xr_token* token_list );
XRCORE_API u32		    	_ParseItem				( LPSTR src, int ind, xr_token* token_list );
XRCORE_API LPSTR 	    	_ReplaceItem 			( LPCSTR src, int index, LPCSTR new_item, LPSTR dst, char separator );
XRCORE_API LPSTR 	    	_ReplaceItems 			( LPCSTR src, int idx_start, int idx_end, LPCSTR new_items, LPSTR dst, char separator );
XRCORE_API void 	    	_SequenceToList			( LPSTRVec& lst, LPCSTR in, char separator=',' );
XRCORE_API void 			_SequenceToList			( RStringVec& lst, LPCSTR in, char separator=',' );
XRCORE_API void 			_SequenceToList			( SStringVec& lst, LPCSTR in, char separator=',' );

XRCORE_API xr_string& 		_Trim					( xr_string& src );
XRCORE_API xr_string& 		_TrimLeft				( xr_string& src );
XRCORE_API xr_string&		_TrimRight				( xr_string& src );
XRCORE_API xr_string&   	_ChangeSymbol			( xr_string& name, char src, char dest );
XRCORE_API LPCSTR		 	_CopyVal 				( LPCSTR src, xr_string& dst, char separator=',' );
XRCORE_API LPCSTR			_GetItem				( LPCSTR src, int, xr_string& p, char separator=',', LPCSTR ="", bool trim=true );
XRCORE_API xr_string		_ListToSequence			( const SStringVec& lst );
XRCORE_API shared_str		_ListToSequence			( const RStringVec& lst );

#endif