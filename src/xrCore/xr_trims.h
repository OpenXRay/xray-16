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
XRCORE_API pcstr _GetItem(pcstr src, int, AnsiString& p, char separator = ',', pcstr = "", bool trim = true);
XRCORE_API pcstr _CopyVal(pcstr src, AnsiString& dst, char separator = ',');
XRCORE_API AnsiString _ListToSequence(const AStringVec& lst);
XRCORE_API AnsiString _ListToSequence2(const AStringVec& lst);
XRCORE_API void _SequenceToList(AStringVec& lst, pcstr in, char separator = ',');
XRCORE_API AnsiString FloatTimeToStrTime(float v, bool h = true, bool m = true, bool s = true, bool ms = false);
XRCORE_API float StrTimeToFloatTime(pcstr buf, bool h = true, bool m = true, bool s = true, bool ms = false);
#endif

XRCORE_API int _GetItemCount(pcstr, char separator = ',');
XRCORE_API LPSTR _GetItem(pcstr, int, LPSTR, u32 const dst_size, char separator = ',', pcstr = "", bool trim = true);

template <int count>
inline LPSTR _GetItem(
    pcstr src, int index, char (&dst)[count], char separator = ',', pcstr def = "", bool trim = true)
{
    return _GetItem(src, index, dst, count, separator, def, trim);
}

XRCORE_API LPSTR _GetItems(pcstr, int, int, LPSTR, char separator = ',');
XRCORE_API pcstr _GetItems(pcstr, int, int, xr_string&, char);
XRCORE_API pcstr _SetPos(pcstr src, u32 pos, char separator = ',');
XRCORE_API pcstr _CopyVal(pcstr src, LPSTR dst, char separator = ',');
XRCORE_API LPSTR _Trim(LPSTR str);
XRCORE_API LPSTR _TrimLeft(LPSTR str);
XRCORE_API LPSTR _TrimRight(LPSTR str);
XRCORE_API LPSTR _ChangeSymbol(LPSTR name, char src, char dest);
XRCORE_API u32 _ParseItem(pcstr src, xr_token* token_list);
XRCORE_API u32 _ParseItem(LPSTR src, int ind, xr_token* token_list);
XRCORE_API LPSTR _ReplaceItem(pcstr src, int index, pcstr new_item, LPSTR dst, char separator);
XRCORE_API xr_string& _ReplaceItem(pcstr src, int index, pcstr new_item, xr_string& dst, char separator);
XRCORE_API LPSTR _ReplaceItems(pcstr src, int idx_start, int idx_end, pcstr new_items, LPSTR dst, char separator);
XRCORE_API xr_string& _ReplaceItems(pcstr src, int idx_start, int idx_end, pcstr new_items, xr_string& dst, char separator);
XRCORE_API void _SequenceToList(LPSTRVec& lst, pcstr in, char separator = ',');
XRCORE_API void _SequenceToList(RStringVec& lst, pcstr in, char separator = ',');
XRCORE_API void _SequenceToList(SStringVec& lst, pcstr in, char separator = ',');

XRCORE_API xr_string& _Trim(xr_string& src);
XRCORE_API xr_string& _TrimLeft(xr_string& src);
XRCORE_API xr_string& _TrimRight(xr_string& src);
XRCORE_API xr_string& _ChangeSymbol(xr_string& name, char src, char dest);
XRCORE_API pcstr _CopyVal(pcstr src, xr_string& dst, char separator = ',');
XRCORE_API pcstr _GetItem(pcstr src, int, xr_string& p, char separator = ',', pcstr = "", bool trim = true);
XRCORE_API xr_string _ListToSequence(const SStringVec& lst);
XRCORE_API shared_str _ListToSequence(const RStringVec& lst);

