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
        *strext(id) = 0;
    auto it = s_sources.find(id);
    if (it != s_sources.end())
    {
        return it->second;
    }

    // Load a _new one
    CSoundRender_Source* S = new CSoundRender_Source();
    S->load(id);
    s_sources.insert({id, S});
    return S;
}

void CSoundRender_Core::i_destroy_source(CSoundRender_Source* S)
{
    // No actual destroy at all
}

void CSoundRender_Core::i_create_all_sources()
{
    CTimer T;
    T.Start();

    FS_FileSet flist;
    FS.file_list(flist, "$game_sounds$", FS_ListFiles, "*.ogg");
    for (const FS_File& file : flist)
    {
        string256 id;
        xr_strcpy(id, file.name.c_str());

        xr_strlwr(id);
        if (strext(id))
            *strext(id) = 0;

        CSoundRender_Source* S = new CSoundRender_Source();
        S->load(id);
        s_sources.insert({id, S});
    }

    Msg("Finished creating %d sound sources. Duration: %d ms", flist.size(), T.GetElapsed_ms());
}
