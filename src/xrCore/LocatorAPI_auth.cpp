#include "stdafx.h"
#pragma hdrstop // huh?
#include "xrCore/Threading/Lock.hpp"

struct auth_options
{
    xr_vector<shared_str> ignore;
    xr_vector<shared_str> important;
};

void auth_entry(void* p) { FS.auth_runtime(p); }
void CLocatorAPI::auth_generate(xr_vector<shared_str>& ignore, xr_vector<shared_str>& important)
{
    auth_options* _o = new auth_options();
    _o->ignore = ignore;
    _o->important = important;

#if 1
    FS.auth_runtime(_o);
#else
    thread_spawn(auth_entry, "checksum", 0, _o);
#endif
}

u64 CLocatorAPI::auth_get()
{
    m_auth_lock->Enter(); // huh? What's the point of enter+leave, except slow things down?
    m_auth_lock->Leave();
    return m_auth_code;
}

void CLocatorAPI::auth_runtime(void* params)
{
	m_auth_lock->Enter();
    auth_options* _o = (auth_options*)params;

    CMemoryWriter writer;
    pSettingsAuth->save_as(writer);
    m_auth_code = crc32(writer.pointer(), writer.size());

#ifdef DEBUG
    if (strstr(Core.Params, "auth_debug"))
    {
        string_path tmp_path;
        update_path(tmp_path, "$app_data_root$", "auth_psettings.ltx");
        IWriter* tmp_dst = w_open(tmp_path);
        pSettingsAuth->save_as(*tmp_dst, false);
        w_close(tmp_dst);
    }
#endif

    bool do_break = false;

#ifdef DEBUG
    bool b_extern_auth = !!strstr(Core.Params, "asdf");
    if (!b_extern_auth)
#endif // DEBUG
    {
        for (auto it = m_files.begin(); it != m_files.end(); ++it)
        {
            const file& f = *it;

            // test for skip
            BOOL bSkip = FALSE;
            for (u32 s = 0; s < _o->ignore.size(); s++)
            {
                if (strstr(f.name, _o->ignore[s].c_str()))
                    bSkip = TRUE;
            }

            if (bSkip)
                continue;

            // test for important
            for (u32 s = 0; s < _o->important.size(); s++)
            {
                if ((f.size_real != 0) && strstr(f.name, _o->important[s].c_str()))
                {
                    // crc for file
                    IReader* r = FS.r_open(f.name);
                    if (!r)
                    {
                        do_break = true;
                        break;
                    }
                    u32 crc = crc32(r->pointer(), r->length());

#ifdef DEBUG
                    if (strstr(Core.Params, "auth_debug"))
                        Msg("auth %s = 0x%08x", f.name, crc);
#endif // DEBUG

                    FS.r_close(r);
                    m_auth_code ^= u64(crc);
                }
            }

            if (do_break)
                break;
        }
#ifdef DEBUG
        Msg("auth_code = %d", m_auth_code);
#endif // DEBUG
    }
#ifdef DEBUG
    else
    {
        string64 c_auth_code;
        sscanf(strstr(Core.Params, "asdf ") + 5, "%[^ ] ", c_auth_code);
        m_auth_code = atoll(c_auth_code);
    }
#endif // DEBUG
    xr_delete(_o);

	m_auth_lock->Leave();
}
