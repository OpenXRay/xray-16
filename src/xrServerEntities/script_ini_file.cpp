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

CScriptIniFile::CScriptIniFile(IReader *F, LPCSTR path) :
inherited(F, path)
{}

CScriptIniFile::CScriptIniFile(LPCSTR szFileName, BOOL ReadOnly, BOOL bLoadAtStart, BOOL SaveAtEnd, LPCSTR path) :
inherited(path?path:update(szFileName), ReadOnly, bLoadAtStart, SaveAtEnd)
{}

CScriptIniFile::~CScriptIniFile()
{}

LPCSTR	CScriptIniFile::update(LPCSTR file_name)
{
    string_path			S1;
    FS.update_path(S1, "$game_config$", file_name);
    return				(*shared_str(S1));
}

bool CScriptIniFile::line_exist(LPCSTR S, LPCSTR L)
{
    return		(!!inherited::line_exist(S, L));
}

bool CScriptIniFile::section_exist(LPCSTR S)
{
    return		(!!inherited::section_exist(S));
}

int	 CScriptIniFile::r_clsid(LPCSTR S, LPCSTR L)
{
    return		(object_factory().script_clsid(inherited::r_clsid(S, L)));
}

bool CScriptIniFile::r_bool(LPCSTR S, LPCSTR L)
{
    return		(!!inherited::r_bool(S, L));
}

int	 CScriptIniFile::r_token(LPCSTR S, LPCSTR L, const CScriptTokenList &token_list)
{
    return		(inherited::r_token(S, L, &*token_list.tokens().begin()));
}

LPCSTR CScriptIniFile::r_string_wb(LPCSTR S, LPCSTR L)
{
    return		(*inherited::r_string_wb(S, L));
}

u32	 CScriptIniFile::line_count(LPCSTR S)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    return		(inherited::line_count(S));
}

LPCSTR CScriptIniFile::r_string(LPCSTR S, LPCSTR L)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    return		(inherited::r_string(S, L));
}

u32	 CScriptIniFile::r_u32(LPCSTR S, LPCSTR L)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    return		(inherited::r_u32(S, L));
}

int	 CScriptIniFile::r_s32(LPCSTR S, LPCSTR L)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    return		(inherited::r_s32(S, L));
}

float CScriptIniFile::r_float(LPCSTR S, LPCSTR L)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    return		(inherited::r_float(S, L));
}

Fvector CScriptIniFile::r_fvector3(LPCSTR S, LPCSTR L)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    return		(inherited::r_fvector3(S, L));
}

//AVO: additional methods to allow writing to ini files
#ifdef INI_FILE_EXTENDED_EXPORTS
void CScriptIniFile::w_bool(LPCSTR S, LPCSTR L, bool V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_bool(S, L, V, comment);
}

void CScriptIniFile::w_color(LPCSTR S, LPCSTR L, u32 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_color(S, L, V, comment);
}

void CScriptIniFile::w_fcolor(LPCSTR S, LPCSTR L, const Fcolor& V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_fcolor(S, L, V, comment);
}

void CScriptIniFile::w_float(LPCSTR S, LPCSTR L, float V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_float(S, L, V, comment);
}

void CScriptIniFile::w_fvector2(LPCSTR S, LPCSTR L, const Fvector2& V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_fvector2(S, L, V, comment);
}

void CScriptIniFile::w_fvector3(LPCSTR S, LPCSTR L, const Fvector3& V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_fvector3(S, L, V, comment);
}

void CScriptIniFile::w_fvector4(LPCSTR S, LPCSTR L, const Fvector4& V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_fvector4(S, L, V, comment);
}

void CScriptIniFile::w_s16(LPCSTR S, LPCSTR L, s16 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_s16(S, L, V, comment);
}

void CScriptIniFile::w_s32(LPCSTR S, LPCSTR L, s32 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_s32(S, L, V, comment);
}

void CScriptIniFile::w_s64(LPCSTR S, LPCSTR L, s64 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_s64(S, L, V, comment);
}

void CScriptIniFile::w_s8(LPCSTR S, LPCSTR L, s8 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_s8(S, L, V, comment);
}

void CScriptIniFile::w_string(LPCSTR S, LPCSTR L, LPCSTR V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_string(S, L, V, comment);
}

void CScriptIniFile::w_u16(LPCSTR S, LPCSTR L, u16 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_u16(S, L, V, comment);
}

void CScriptIniFile::w_u32(LPCSTR S, LPCSTR L, u32 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_u32(S, L, V, comment);
}

void CScriptIniFile::w_u64(LPCSTR S, LPCSTR L, u64 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_u64(S, L, V, comment);
}

void CScriptIniFile::w_u8(LPCSTR S, LPCSTR L, u8 V, LPCSTR comment)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::w_u8(S, L, V, comment);
}

bool CScriptIniFile::save_as(LPCSTR new_fname)
{
    THROW2(new_fname, "File name is null");
    return(inherited::save_as(new_fname));
}

void CScriptIniFile::save_at_end(bool b)
{
    inherited::save_at_end(b);
}

void CScriptIniFile::remove_line(LPCSTR S, LPCSTR L)
{
    THROW3(inherited::section_exist(S), "Cannot find section", S);
    THROW3(inherited::line_exist(S, L), "Cannot find line", L);
    inherited::remove_line(S, L);
}

void CScriptIniFile::set_override_names(bool b)
{
    inherited::set_override_names(b);
}

void CScriptIniFile::set_readonly(bool b)
{
	inherited::m_flags.set(eReadOnly, b);
}

u32 CScriptIniFile::section_count()
{
	return (inherited::section_count());
}
#endif