#include "stdafx.h"
#pragma hdrstop // huh?
#include "xrCore/Threading/Lock.hpp"

#ifdef DEBUG
#include "command_line_key.h"
static command_line_key<bool> auth_debug("auth_debug", "auth_debug", false);
static command_line_key<int> b_extern_auth("asdf", "b_extern_auth", 0);
#endif


struct auth_options
{
    xr_vector<shared_str> ignore;
    xr_vector<shared_str> important;
};

void auth_entry(void* p) { FS.auth_runtime(p); }
void CLocatorAPI::auth_generate(xr_vector<shared_str>& ignore, xr_vector<shared_str>& important)
{
    auth_options* _o = xr_new<auth_options>();
    _o->ignore = ignore;
    _o->important = important;

    FS.auth_runtime(_o);
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
    if (auth_debug.OptionValue())
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
    if (!b_extern_auth.OptionValue())
#endif // DEBUG
    {
        for (auto it = m_files.begin(); it != m_files.end(); ++it)
        {
            const file& f = *it;

            // test for skip
            BOOL bSkip = FALSE;
            for (size_t s = 0; s < _o->ignore.size(); s++)
            {
                if (strstr(f.name, _o->ignore[s].c_str()))
                    bSkip = TRUE;
            }

            if (bSkip)
                continue;

            // test for important
            for (size_t s = 0; s < _o->important.size(); s++)
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
                    if (auth_debug.OptionValue())
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
        m_auth_code = b_extern_auth.OptionValue();
    }
#endif // DEBUG
    xr_delete(_o);

	m_auth_lock->Leave();
}
