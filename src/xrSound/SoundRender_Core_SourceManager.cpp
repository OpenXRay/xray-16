#include "stdafx.h"

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"

CSoundRender_Source* CSoundRender_Core::i_create_source(pcstr name)
{
    // Search
    string256 id;
    xr_strcpy(id, name);
    xr_strlwr(id);
    if (strext(id))
        * strext(id) = 0;

    {
        ScopeLock scope(&s_sources_lock);
        const auto it = s_sources.find(id);
        if (it != s_sources.end())
        {
            return it->second;
        }
    }

    // Load a _new one
    CSoundRender_Source source;
    if (source.load(id))
    {
        ScopeLock scope(&s_sources_lock);
        CSoundRender_Source* S = xr_new<CSoundRender_Source>(std::move(source));
        s_sources.emplace(id, S);
        return S;
    }

    return nullptr;
}

void CSoundRender_Core::i_destroy_source(CSoundRender_Source* S)
{
    // No actual destroy at all
}
