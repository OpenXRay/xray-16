////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_inline.h
//	Created 	: 01.04.2004
//  Modified 	: 01.04.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC void CScriptEngine::add_script_process(const ScriptProcessor &process_id, CScriptProcess *script_process)
{
    CScriptProcessStorage::const_iterator I = m_script_processes.find(process_id);
    VERIFY (I == m_script_processes.end());
    m_script_processes.insert(std::make_pair(process_id, script_process));
}

CScriptProcess *CScriptEngine::script_process(const ScriptProcessor &process_id) const
{
    CScriptProcessStorage::const_iterator I = m_script_processes.find(process_id);
    if ((I != m_script_processes.end()))
        return ((*I).second);
    return nullptr;
}

IC void CScriptEngine::parse_script_namespace(const char *name, char *ns, u32 nsSize, char *func, u32 funcSize)
{
    auto p = strrchr(name, '.');
    if (!p)
    {
        xr_strcpy(ns, nsSize, GlobalNamespace);
        p = name-1;
    }
    else
    {
        VERIFY(u32(p-name+1)<=nsSize);
        strncpy(ns, name, p-name);
        ns[p-name] = 0;
    }
    xr_strcpy(func, funcSize, p+1);
}

template<typename TResult>
IC bool CScriptEngine::functor(LPCSTR function_to_call, luabind::functor<TResult> &lua_function)
{
    luabind::object object;
    if (!function_object(function_to_call, object))
        return false;
    lua_function = object;
    return true;
}
