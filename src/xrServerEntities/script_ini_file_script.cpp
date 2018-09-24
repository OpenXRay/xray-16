////////////////////////////////////////////////////////////////////////////
//	Module 		: script_ini_file_script.cpp
//	Created 	: 25.06.2004
//  Modified 	: 25.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script ini file class export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_ini_file.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;

CScriptIniFile* get_system_ini() { return ((CScriptIniFile*)pSettings); }
bool r_line(CScriptIniFile* self, LPCSTR S, int L, luabind::string& N, luabind::string& V)
{
    THROW3(self->section_exist(S), "Cannot find section", S);
    THROW2((int)self->line_count(S) > L, "Invalid line number");

    N = "";
    V = "";

    LPCSTR n, v;
    bool result = !!self->r_line(S, L, &n, &v);
    if (!result)
        return (false);

    N = n;
    if (v)
        V = v;
    return (true);
}

#pragma warning(push)
#pragma warning(disable : 4238)
CScriptIniFile* create_ini_file(LPCSTR ini_string)
{
    IReader reader((void*)ini_string, xr_strlen(ini_string));
    return ((CScriptIniFile*)new CInifile(&reader, FS.get_path("$game_config$")->m_Path));
}
#pragma warning(pop)

//Alundaio: The extended ability to reload system ini after application launch
CScriptIniFile* reload_system_ini()
{
    pSettings->Destroy(const_cast<CInifile*>(pSettings));
    string_path fname;
    FS.update_path(fname, "$game_config$", "system.ltx");
    pSettings = new CInifile(fname);
    return (CScriptIniFile*)pSettings;
}
//Alundaio: END

#ifdef XRGAME_EXPORTS
CScriptIniFile* get_game_ini() { return (CScriptIniFile*)pGameIni; }
#endif

static void CScriptIniFile_Export(lua_State* luaState)
{
    module(luaState)
    [
        class_<CScriptIniFile>("ini_file")
            .def(constructor<LPCSTR>())
            //Alundaio: Extend script ini file
            .def("w_bool", &CScriptIniFile::w_bool)
            .def("w_color", &CScriptIniFile::w_color)
            .def("w_fcolor", &CScriptIniFile::w_fcolor)
            .def("w_float", &CScriptIniFile::w_float)
            .def("w_fvector2", &CScriptIniFile::w_fvector2)
            .def("w_fvector3", &CScriptIniFile::w_fvector3)
            .def("w_fvector4", &CScriptIniFile::w_fvector4)
            .def("w_s16", &CScriptIniFile::w_s16)
            .def("w_s32", &CScriptIniFile::w_s32)
            .def("w_s64", &CScriptIniFile::w_s64)
            .def("w_s8", &CScriptIniFile::w_s8)
            .def("w_string", &CScriptIniFile::w_string)
            .def("w_u16", &CScriptIniFile::w_u16)
            .def("w_u32", &CScriptIniFile::w_u32)
            .def("w_u64", &CScriptIniFile::w_u64)
            .def("w_u8", &CScriptIniFile::w_u8)
            .def("save_as", &CScriptIniFile::save_as)
            .def("save_at_end", &CScriptIniFile::save_at_end)
            .def("remove_line", &CScriptIniFile::remove_line)
            .def("set_override_names", &CScriptIniFile::set_override_names)
            .def("section_count", &CScriptIniFile::section_count)
            //Alundaio: END
            .def("section_exist", &CScriptIniFile::section_exist)
            .def("line_exist", (bool (CScriptIniFile::*)(LPCSTR, LPCSTR) const)&CScriptIniFile::line_exist)
            .def("r_clsid", &CScriptIniFile::r_clsid)
            .def("r_bool", &CScriptIniFile::r_bool)
            .def("r_token", &CScriptIniFile::r_token)
            .def("r_string_wq", &CScriptIniFile::r_string_wb)
            .def("line_count", &CScriptIniFile::line_count)
            .def("r_string", &CScriptIniFile::r_string)
            .def("r_u32", &CScriptIniFile::r_u32)
            .def("r_s32", &CScriptIniFile::r_s32)
            .def("r_float", &CScriptIniFile::r_float)
            .def("r_vector", &CScriptIniFile::r_fvector3)
            .def("r_line", &::r_line, policy_list<out_value<4>, out_value<5>>()),
#ifdef XRGAME_EXPORTS
            def("game_ini", &get_game_ini),
#endif
            //Alundaio: extend
            def("reload_system_ini", &reload_system_ini),
            //Alundaio:: END
            def("system_ini", &get_system_ini), def("create_ini_file", &create_ini_file, adopt<0>())
    ];
}

SCRIPT_EXPORT_FUNC(CScriptIniFile, (), CScriptIniFile_Export);
