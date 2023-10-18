////////////////////////////////////////////////////////////////////////////
//	Module 		: script_ini_file.h
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script ini file class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_token_list.h"

class CScriptIniFile : public CInifile
{
protected:
    using inherited = CInifile;

public:
    CScriptIniFile(IReader* F, LPCSTR path = nullptr);
    CScriptIniFile(LPCSTR szFileName, BOOL ReadOnly = TRUE, BOOL bLoadAtStart = TRUE, BOOL SaveAtEnd = TRUE);
    CScriptIniFile(LPCSTR initial, LPCSTR szFileName, BOOL ReadOnly = TRUE, BOOL bLoadAtStart = TRUE, BOOL SaveAtEnd = TRUE);

    int r_clsid(LPCSTR S, LPCSTR L);
    int r_token(LPCSTR S, LPCSTR L, const CScriptTokenList& token_list);
    LPCSTR update(LPCSTR initial, LPCSTR file_name);
    u32 line_count(LPCSTR S);
    LPCSTR r_string(LPCSTR S, LPCSTR L);
    u32 r_u32(LPCSTR S, LPCSTR L);
    int r_s32(LPCSTR S, LPCSTR L);
    float r_float(LPCSTR S, LPCSTR L);
    Fvector r_fvector3(LPCSTR S, LPCSTR L);

    //AVO: additional methods to allow writing to ini files
    void w_bool(pcstr S, pcstr L, bool V, pcstr comment /* = nullptr */);
    void w_color(pcstr S, pcstr L, u32 V, pcstr comment /* = nullptr */);
    void w_fcolor(pcstr S, pcstr L, const Fcolor& V, pcstr comment /* = nullptr */);
    void w_float(pcstr S, pcstr L, float V, pcstr comment /* = nullptr */);
    void w_fvector2(pcstr S, pcstr L, const Fvector2& V, pcstr comment /* = nullptr */);
    void w_fvector3(pcstr S, pcstr L, const Fvector3& V, pcstr comment /* = nullptr */);
    void w_fvector4(pcstr S, pcstr L, const Fvector4& V, pcstr comment /* = nullptr */);
    void w_s16(pcstr S, pcstr L, s16 V, pcstr comment /* = nullptr */);
    void w_s32(pcstr S, pcstr L, s32 V, pcstr comment /* = nullptr */);
    void w_s64(pcstr S, pcstr L, s64 V, pcstr comment /* = nullptr */);
    void w_s8(pcstr S, pcstr L, s8 V, pcstr comment /* = nullptr */);
    void w_string(pcstr S, pcstr L, pcstr V, pcstr comment /* = nullptr */);
    void w_u16(pcstr S, pcstr L, u16 V, pcstr comment /* = nullptr */);
    void w_u32(pcstr S, pcstr L, u32 V, pcstr comment /* = nullptr */);
    void w_u64(pcstr S, pcstr L, u64 V, pcstr comment /* = nullptr */);
    void w_u8(pcstr S, pcstr L, u8 V, pcstr comment /* = nullptr */);
    bool save_as(pcstr new_fname /* = nullptr */);
    void save_at_end(bool b);
    void remove_line(pcstr S, pcstr L);
    void set_override_names(bool b);
};
