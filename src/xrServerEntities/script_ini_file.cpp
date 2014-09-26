////////////////////////////////////////////////////////////////////////////
//	Module 		: script_ini_file.cpp
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script ini file class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "script_ini_file.h"
#include "script_engine.h"
#include "ai_space.h"
#include "object_factory.h"

CScriptIniFile::CScriptIniFile		(IReader *F, LPCSTR path) :
	inherited	(F,path)
{
}

CScriptIniFile::CScriptIniFile		(LPCSTR szFileName, BOOL ReadOnly, BOOL bLoadAtStart, BOOL SaveAtEnd) :
	inherited	(update(szFileName), ReadOnly, bLoadAtStart, SaveAtEnd)
{
}

CScriptIniFile::~CScriptIniFile		()
{
}

LPCSTR	CScriptIniFile::update		(LPCSTR file_name)
{
	string_path			S1;
	FS.update_path		(S1,"$game_config$",file_name);
	return				(*shared_str(S1));
}

bool CScriptIniFile::line_exist		(LPCSTR S, LPCSTR L)
{
	return		(!!inherited::line_exist(S,L));
}

bool CScriptIniFile::section_exist	(LPCSTR S)
{
	return		(!!inherited::section_exist(S));
}

int	 CScriptIniFile::r_clsid		(LPCSTR S, LPCSTR L)
{
	return		(object_factory().script_clsid(inherited::r_clsid(S,L)));
}

bool CScriptIniFile::r_bool			(LPCSTR S, LPCSTR L)
{
	return		(!!inherited::r_bool(S,L));
}

int	 CScriptIniFile::r_token		(LPCSTR S, LPCSTR L, const CScriptTokenList &token_list)
{
	return		(inherited::r_token(S,L,&*token_list.tokens().begin()));
}

LPCSTR CScriptIniFile::r_string_wb	(LPCSTR S, LPCSTR L)
{
	return		(*inherited::r_string_wb(S,L));
}

u32	 CScriptIniFile::line_count			(LPCSTR S)
{
	THROW3		(inherited::section_exist(S),"Cannot find section",S);
	return		(inherited::line_count(S));
}

LPCSTR CScriptIniFile::r_string			(LPCSTR S, LPCSTR L)
{
	THROW3		(inherited::section_exist(S),"Cannot find section",S);
	THROW3		(inherited::line_exist(S,L),"Cannot find line",L);
	return		(inherited::r_string(S,L));
}

u32	 CScriptIniFile::r_u32				(LPCSTR S, LPCSTR L)
{
	THROW3		(inherited::section_exist(S),"Cannot find section",S);
	THROW3		(inherited::line_exist(S,L),"Cannot find line",L);
	return		(inherited::r_u32(S,L));
}

int	 CScriptIniFile::r_s32				(LPCSTR S, LPCSTR L)
{
	THROW3		(inherited::section_exist(S),"Cannot find section",S);
	THROW3		(inherited::line_exist(S,L),"Cannot find line",L);
	return		(inherited::r_s32(S,L));
}

float CScriptIniFile::r_float			(LPCSTR S, LPCSTR L)
{
	THROW3		(inherited::section_exist(S),"Cannot find section",S);
	THROW3		(inherited::line_exist(S,L),"Cannot find line",L);
	return		(inherited::r_float(S,L));
}

Fvector CScriptIniFile::r_fvector3		(LPCSTR S, LPCSTR L)
{
	THROW3		(inherited::section_exist(S),"Cannot find section",S);
	THROW3		(inherited::line_exist(S,L),"Cannot find line",L);
	return		(inherited::r_fvector3(S,L));
}
