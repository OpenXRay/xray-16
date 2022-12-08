#include "stdafx.h"

#include "SoundRender_Core.h"
#include "SoundRender_Source.h"

#include "xrCore/Threading/ParallelForEach.hpp"

bool CSoundRender_Core::i_create_source(CSound_source*& result, pcstr name, bool replaceWithNoSound /*= true*/)
{
    CSoundRender_Source* source = nullptr;
    const bool found = i_create_source(source, name, replaceWithNoSound);
    result = source;
    return found;
}

bool CSoundRender_Core::i_create_source(CSoundRender_Source*& result, pcstr name, bool replaceWithNoSound /*= true*/)
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
            result = it->second;
            return true;
        }
    }

    // Load a _new one
    CSoundRender_Source* S = xr_new<CSoundRender_Source>();
    const bool itIsFound = S->load(id, replaceWithNoSound);

    if (!itIsFound && !replaceWithNoSound)
    {
        xr_delete(S);
    }
    else
    {
        ScopeLock scope(&s_sources_lock);
        s_sources.insert({ id, S });
    }

    result = S;
    return itIsFound;
}

CSoundRender_Source* CSoundRender_Core::i_create_source(pcstr name, bool replaceWithNoSound /*= true*/)
{
    CSoundRender_Source* result = nullptr;
    i_create_source(result, name, replaceWithNoSound);
    return result;
}

void CSoundRender_Core::i_destroy_source(CSoundRender_Source* S)
{
    // No actual destroy at all
}

void CSoundRender_Core::i_create_all_sources()
{
#ifndef MASTER_GOLD
    CTimer T;
    T.Start();
#endif
    FS_FileSet flist;
    FS.file_list(flist, "$game_sounds$", FS_ListFiles, "*.ogg");
    const size_t sizeBefore = s_sources.size();

    const auto processFile = [&](const FS_File& file)
    {
        string256 id;
        xr_strcpy(id, file.name.c_str());

        xr_strlwr(id);
        if (strext(id))
            *strext(id) = 0;

        {
            ScopeLock scope(&s_sources_lock);
            const auto it = s_sources.find(id);
            if (it != s_sources.end())
                return;
        }

        CSoundRender_Source* S = xr_new<CSoundRender_Source>();
        if (!S->load(id, false, false))
        {
            xr_delete(S);
            return;
        }

        ScopeLock scope(&s_sources_lock);
        s_sources.insert({ id, S });
    };

    xr_parallel_for_each(flist, processFile);

#ifndef MASTER_GOLD
    Msg("Finished creating %d sound sources. Duration: %d ms",
        s_sources.size() - sizeBefore, T.GetElapsed_ms());
#endif
}
