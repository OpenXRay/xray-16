#pragma once

#include "xrCore/xrstring.h"

// refs
struct xr_token;

XRCORE_API int _GetItemCount(pcstr, char separator = ',');
XRCORE_API pstr _GetItem(pcstr, int, pstr, u32 const dst_size, char separator = ',', pcstr = "", bool trim = true);

template <int count>
inline pstr _GetItem(
    pcstr src, int index, char (&dst)[count], char separator = ',', pcstr def = "", bool trim = true)
{
    return _GetItem(src, index, dst, count, separator, def, trim);
}

XRCORE_API pstr _GetItems(pcstr, int, int, pstr, char separator = ',');
XRCORE_API pcstr _GetItems(pcstr, int, int, xr_string&, char);
XRCORE_API pcstr _SetPos(pcstr src, u32 pos, char separator = ',');
XRCORE_API pcstr _CopyVal(pcstr src, pstr dst, char separator = ',');
XRCORE_API pstr _Trim(pstr str, char whatToTrim = ' ');
XRCORE_API pstr _TrimLeft(pstr str, char whatToTrim = ' ');
XRCORE_API pstr _TrimRight(pstr str, char whatToTrim = ' ');
XRCORE_API pstr _ChangeSymbol(pstr name, char src, char dest);
XRCORE_API u32 _ParseItem(pcstr src, xr_token* token_list);
XRCORE_API u32 _ParseItem(pstr src, int ind, xr_token* token_list);
XRCORE_API pstr _ReplaceItem(pcstr src, int index, pcstr new_item, pstr dst, char separator);
XRCORE_API xr_string& _ReplaceItem(pcstr src, int index, pcstr new_item, xr_string& dst, char separator);
XRCORE_API pstr _ReplaceItems(pcstr src, int idx_start, int idx_end, pcstr new_items, pstr dst, char separator);
XRCORE_API xr_string& _ReplaceItems(pcstr src, int idx_start, int idx_end, pcstr new_items, xr_string& dst, char separator);
XRCORE_API void _SequenceToList(xr_vector<pstr>& lst, pcstr in, char separator = ',');
XRCORE_API void _SequenceToList(xr_vector<shared_str>& lst, pcstr in, char separator = ',');
XRCORE_API void _SequenceToList(xr_vector<xr_string>& lst, pcstr in, char separator = ',');

XRCORE_API xr_string& _Trim(xr_string& src);
XRCORE_API xr_string& _TrimLeft(xr_string& src);
XRCORE_API xr_string& _TrimRight(xr_string& src);
XRCORE_API xr_string& _ChangeSymbol(xr_string& name, char src, char dest);
XRCORE_API pcstr _CopyVal(pcstr src, xr_string& dst, char separator = ',');
XRCORE_API pcstr _GetItem(pcstr src, int, xr_string& p, char separator = ',', pcstr = "", bool trim = true);
XRCORE_API xr_string _ListToSequence(const xr_vector<xr_string>& lst);
XRCORE_API shared_str _ListToSequence(const xr_vector<shared_str>& lst);

