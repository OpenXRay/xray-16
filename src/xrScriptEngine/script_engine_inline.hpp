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

IC void CScriptEngine::parse_script_namespace(LPCSTR function_to_call, LPSTR name_space,
    u32 const namespace_size, LPSTR function, u32 const function_size)
{
    LPCSTR I = function_to_call, J = nullptr;
    for (; ; J = I , ++I)
    {
        I = strchr(I, '.');
        if (!I)
            break;
    }
    xr_strcpy(name_space, namespace_size, "_G");
    if (!J)
        xr_strcpy(function, function_size, function_to_call);
    else
    {
        CopyMemory (name_space,function_to_call, u32(J - function_to_call)*sizeof(char)) ;
        name_space[u32(J - function_to_call)] = 0;
        xr_strcpy(function, function_size, J + 1);
    }
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
