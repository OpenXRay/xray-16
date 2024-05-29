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
    CSoundRender_Source* S = xr_new<CSoundRender_Source>();

    if (!S->load(id))
    {
        // XXX: Make CSoundRender_Source movable and make allocation only if S->load succeeded.
        xr_delete(S);
    }
    else
    {
        ScopeLock scope(&s_sources_lock);
        s_sources.insert({ id, S });
    }

    return S;
}

void CSoundRender_Core::i_destroy_source(CSoundRender_Source* S)
{
    // No actual destroy at all
    S->detach();
}
