////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_detail.cpp
//	Created 	: 11.01.2008
//  Modified 	: 11.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment detail namespace
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_detail.hpp"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

using editor::environment::detail::logical_string_predicate;

static HRESULT AnsiToUnicode						(LPCSTR pszA, LPVOID buffer, u32 const& buffer_size)
{
	VERIFY			(pszA);
	VERIFY			(buffer);
	VERIFY			(buffer_size);

	u32				cCharacters =  xr_strlen(pszA)+1;
	VERIFY			(cCharacters*2 <= buffer_size);

	if (MultiByteToWideChar(CP_ACP, 0, pszA, cCharacters, (LPOLESTR)buffer, cCharacters))
		return		(NOERROR);

	return			(HRESULT_FROM_WIN32(GetLastError()));
}

bool logical_string_predicate::operator()			(LPCSTR const& first, LPCSTR const& second) const
{
	u32				buffer_size0 = (xr_strlen(first) + 1)*2;
	LPCWSTR			buffer0 = (LPCWSTR)_alloca(buffer_size0);
	AnsiToUnicode	(first, (LPVOID)buffer0, buffer_size0);

	u32				buffer_size1 = (xr_strlen(second) + 1)*2;
	LPCWSTR			buffer1 = (LPCWSTR)_alloca(buffer_size1);
	AnsiToUnicode	(second, (LPVOID)buffer1, buffer_size1);

	return			(StrCmpLogicalW(buffer0, buffer1) < 0);
}

bool logical_string_predicate::operator()			(shared_str const& first, shared_str const& second) const
{
	u32				buffer_size0 = (first.size() + 1)*2;
	LPCWSTR			buffer0 = (LPCWSTR)_alloca(buffer_size0);
	AnsiToUnicode	(first.c_str(), (LPVOID)buffer0, buffer_size0);

	u32				buffer_size1 = (second.size() + 1)*2;
	LPCWSTR			buffer1 = (LPCWSTR)_alloca(buffer_size1);
	AnsiToUnicode	(second.c_str(), (LPVOID)buffer1, buffer_size1);

	return			(StrCmpLogicalW(buffer0, buffer1) < 0);
}

shared_str editor::environment::detail::real_path	(LPCSTR folder, LPCSTR path)
{
	string_path		result;
	FS.update_path	(result, folder, path);
	return			(result);
}

#endif // #ifdef INGAME_EDITOR
