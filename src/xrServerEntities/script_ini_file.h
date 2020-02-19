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
    typedef CInifile inherited;

public:
    CScriptIniFile(IReader* F, const char* path = nullptr);
    CScriptIniFile(const char* szFileName, bool ReadOnly = TRUE, bool bLoadAtStart = TRUE, bool SaveAtEnd = TRUE);
    virtual ~CScriptIniFile();
    bool line_exist(const char* S, const char* L);
    bool section_exist(const char* S);
    int r_clsid(const char* S, const char* L);
    bool r_bool(const char* S, const char* L);
    int r_token(const char* S, const char* L, const CScriptTokenList& token_list);
    const char* r_string_wb(const char* S, const char* L);
    const char* update(const char* file_name);
    u32 line_count(const char* S);
    const char* r_string(const char* S, const char* L);
    u32 r_u32(const char* S, const char* L);
    int r_s32(const char* S, const char* L);
    float r_float(const char* S, const char* L);
    Fvector r_fvector3(const char* S, const char* L);

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
    u32 section_count();
};

#include "script_ini_file_inline.h"
