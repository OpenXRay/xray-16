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
XRCORE_API const char* _GetItem(const char* src, int, AnsiString& p, char separator = ',', const char* = "", bool trim = true);
XRCORE_API const char* _CopyVal(const char* src, AnsiString& dst, char separator = ',');
XRCORE_API AnsiString _ListToSequence(const AStringVec& lst);
XRCORE_API AnsiString _ListToSequence2(const AStringVec& lst);
XRCORE_API void _SequenceToList(AStringVec& lst, const char* in, char separator = ',');
XRCORE_API AnsiString FloatTimeToStrTime(float v, bool h = true, bool m = true, bool s = true, bool ms = false);
XRCORE_API float StrTimeToFloatTime(const char* buf, bool h = true, bool m = true, bool s = true, bool ms = false);
#endif

XRCORE_API int _GetItemCount(const char*, char separator = ',');
XRCORE_API char* _GetItem(const char*, int, char*, u32 const dst_size, char separator = ',', const char* = "", bool trim = true);

template <int count>
inline char* _GetItem(
    const char* src, int index, char (&dst)[count], char separator = ',', const char* def = "", bool trim = true)
{
    return _GetItem(src, index, dst, count, separator, def, trim);
}

XRCORE_API char* _GetItems(const char*, int, int, char*, char separator = ',');
XRCORE_API pcstr _GetItems(pcstr, int, int, xr_string&, char);
XRCORE_API const char* _SetPos(const char* src, u32 pos, char separator = ',');
XRCORE_API const char* _CopyVal(const char* src, char* dst, char separator = ',');
XRCORE_API char* _Trim(char* str, char whatToTrim = ' ');
XRCORE_API char* _TrimLeft(char* str, char whatToTrim = ' ');
XRCORE_API char* _TrimRight(char* str, char whatToTrim = ' ');
XRCORE_API char* _ChangeSymbol(char* name, char src, char dest);
XRCORE_API u32 _ParseItem(const char* src, xr_token* token_list);
XRCORE_API u32 _ParseItem(char* src, int ind, xr_token* token_list);
XRCORE_API char* _ReplaceItem(const char* src, int index, const char* new_item, char* dst, char separator);
XRCORE_API xr_string& _ReplaceItem(pcstr src, int index, pcstr new_item, xr_string& dst, char separator);
XRCORE_API char* _ReplaceItems(const char* src, int idx_start, int idx_end, const char* new_items, char* dst, char separator);
XRCORE_API xr_string& _ReplaceItems(pcstr src, int idx_start, int idx_end, pcstr new_items, xr_string& dst, char separator);
XRCORE_API void _SequenceToList(LPSTRVec& lst, const char* in, char separator = ',');
XRCORE_API void _SequenceToList(RStringVec& lst, const char* in, char separator = ',');
XRCORE_API void _SequenceToList(SStringVec& lst, const char* in, char separator = ',');

XRCORE_API xr_string& _Trim(xr_string& src);
XRCORE_API xr_string& _TrimLeft(xr_string& src);
XRCORE_API xr_string& _TrimRight(xr_string& src);
XRCORE_API xr_string& _ChangeSymbol(xr_string& name, char src, char dest);
XRCORE_API const char* _CopyVal(const char* src, xr_string& dst, char separator = ',');
XRCORE_API const char* _GetItem(const char* src, int, xr_string& p, char separator = ',', const char* = "", bool trim = true);
XRCORE_API xr_string _ListToSequence(const SStringVec& lst);
XRCORE_API shared_str _ListToSequence(const RStringVec& lst);

