#pragma once
#include "xrCore/_types.h"
#include "xrCore/xr_vector_defs.h"
#include "xrCore/xrstring.h"

// refs
struct xr_token;
using RStringVec = xr_vector<shared_str>;
using string64Vec = xr_vector<string64>;


#ifdef __BORLANDC__
XRCORE_API AnsiString& _Trim(AnsiString& str);
XRCORE_API LPCSTR _GetItem(LPCSTR src, int, AnsiString& p, char separator = ',', LPCSTR = "", bool trim = true);
XRCORE_API LPCSTR _CopyVal(LPCSTR src, AnsiString& dst, char separator = ',');
XRCORE_API AnsiString _ListToSequence(const AStringVec& lst);
XRCORE_API AnsiString _ListToSequence2(const AStringVec& lst);
XRCORE_API void _SequenceToList(AStringVec& lst, LPCSTR in, char separator = ',');
XRCORE_API AnsiString FloatTimeToStrTime(float v, bool h = true, bool m = true, bool s = true, bool ms = false);
XRCORE_API float StrTimeToFloatTime(LPCSTR buf, bool h = true, bool m = true, bool s = true, bool ms = false);
#endif

XRCORE_API int _GetItemCount(LPCSTR, char separator = ',');
XRCORE_API LPSTR _GetItem(LPCSTR, int, LPSTR, u32 const dst_size, char separator = ',', LPCSTR = "", bool trim = true);

template <int count>
inline LPSTR _GetItem(
    LPCSTR src, int index, char (&dst)[count], char separator = ',', LPCSTR def = "", bool trim = true)
{
    return _GetItem(src, index, dst, count, separator, def, trim);
}

XRCORE_API LPSTR _GetItems(LPCSTR, int, int, LPSTR, char separator = ',');
XRCORE_API pcstr _GetItems(pcstr, int, int, xr_string&, char);
XRCORE_API LPCSTR _SetPos(LPCSTR src, u32 pos, char separator = ',');
XRCORE_API LPCSTR _CopyVal(LPCSTR src, LPSTR dst, char separator = ',');
XRCORE_API LPSTR _Trim(LPSTR str, char whatToTrim = ' ');
XRCORE_API LPSTR _TrimLeft(LPSTR str, char whatToTrim = ' ');
XRCORE_API LPSTR _TrimRight(LPSTR str, char whatToTrim = ' ');
XRCORE_API LPSTR _ChangeSymbol(LPSTR name, char src, char dest);
XRCORE_API u32 _ParseItem(LPCSTR src, xr_token* token_list);
XRCORE_API u32 _ParseItem(LPSTR src, int ind, xr_token* token_list);
XRCORE_API LPSTR _ReplaceItem(LPCSTR src, int index, LPCSTR new_item, LPSTR dst, char separator);
XRCORE_API xr_string& _ReplaceItem(pcstr src, int index, pcstr new_item, xr_string& dst, char separator);
XRCORE_API LPSTR _ReplaceItems(LPCSTR src, int idx_start, int idx_end, LPCSTR new_items, LPSTR dst, char separator);
XRCORE_API xr_string& _ReplaceItems(pcstr src, int idx_start, int idx_end, pcstr new_items, xr_string& dst, char separator);
XRCORE_API void _SequenceToList(LPSTRVec& lst, LPCSTR in, char separator = ',');
XRCORE_API void _SequenceToList(RStringVec& lst, LPCSTR in, char separator = ',');
XRCORE_API void _SequenceToList(SStringVec& lst, LPCSTR in, char separator = ',');

XRCORE_API xr_string& _Trim(xr_string& src);
XRCORE_API xr_string& _TrimLeft(xr_string& src);
XRCORE_API xr_string& _TrimRight(xr_string& src);
XRCORE_API xr_string& _ChangeSymbol(xr_string& name, char src, char dest);
XRCORE_API LPCSTR _CopyVal(LPCSTR src, xr_string& dst, char separator = ',');
XRCORE_API LPCSTR _GetItem(LPCSTR src, int, xr_string& p, char separator = ',', LPCSTR = "", bool trim = true);
XRCORE_API xr_string _ListToSequence(const SStringVec& lst);
XRCORE_API shared_str _ListToSequence(const RStringVec& lst);

