#include "StdAfx.h"
#include "gsc_dsigned_ltx.h"
#include "xrCore/xr_ini.h"

gsc_dsigned_ltx_writer::gsc_dsigned_ltx_writer(u8 const p_number[crypto::xr_dsa::public_key_length],
    u8 const q_number[crypto::xr_dsa::private_key_length], u8 const g_number[crypto::xr_dsa::public_key_length],
    priv_key_filler_function_t pkf_func)
    : xr_dsa_signer(p_number, q_number, g_number), m_ltx(NULL, FALSE, FALSE, FALSE)
{
    pkf_func(m_private_key);
}

gsc_dsigned_ltx_writer::~gsc_dsigned_ltx_writer() {}
static char const* dsign_secion = "dsign";

void gsc_dsigned_ltx_writer::sign_and_save(IWriter& writer)
{
    string64 dsign_time;
    m_mem_writer.seek(0);
    m_ltx.save_as(m_mem_writer);
    u32 tmp_write_pos = m_mem_writer.tell();
    m_mem_writer.w_stringZ(current_time(dsign_time));

    shared_str tmp_sign_value = sign(m_mem_writer.pointer(), m_mem_writer.tell());

    m_mem_writer.seek(tmp_write_pos);
    LPCSTR append_value = NULL;
    STRCONCAT(append_value, "\r\n[", dsign_secion, "]\r\n	date		=	", dsign_time, "\r\n	sign_hash	=	",
        tmp_sign_value.c_str());
    m_ltx.save_as(writer);
    writer.w_stringZ(append_value);
}

static char* search_dsign_section(u8* buffer, u32 buffer_size)
{
    u32 sstr_size = xr_strlen(dsign_secion);
    VERIFY(buffer_size >= sstr_size);
    u8* rbegin = buffer + (buffer_size - sstr_size);
    int r_size = static_cast<int>(buffer_size - sstr_size);
    do
    {
        if (!memcmp(rbegin, dsign_secion, sstr_size))
        {
            return static_cast<char*>((void*)rbegin);
        }
        --rbegin;
        --r_size;
    } while (r_size > 0);
    return NULL;
}

gsc_dsigned_ltx_reader::gsc_dsigned_ltx_reader(u8 const p_number[crypto::xr_dsa::public_key_length],
    u8 const q_number[crypto::xr_dsa::private_key_length], u8 const g_number[crypto::xr_dsa::public_key_length],
    u8 const public_key[crypto::xr_dsa::public_key_length])
    : xr_dsa_verifyer(p_number, q_number, g_number, public_key), m_ltx(NULL)
{
}

gsc_dsigned_ltx_reader::~gsc_dsigned_ltx_reader() { xr_delete(m_ltx); }
bool gsc_dsigned_ltx_reader::load_and_verify(u8* buffer, u32 const size)
{
    char* dsign_section = search_dsign_section(buffer, size);
    if (dsign_section == NULL)
        return false;

    dsign_section -= 3; // \r\n[
    u32 const dsign_sect_size = size - static_cast<u32>((u8*)dsign_section - buffer);

    IReader sign_reader(dsign_section, dsign_sect_size);
    CInifile tmp_dsign_reader(&sign_reader);

    shared_str ltx_date = tmp_dsign_reader.r_string(dsign_secion, "date");
    shared_str ltx_dsign = tmp_dsign_reader.r_string(dsign_secion, "sign_hash");

    *dsign_section = 0;
    xr_strcat(dsign_section, dsign_sect_size, ltx_date.c_str());
    u32 new_size = size - dsign_sect_size + ltx_date.size() + 1; // xr_strlen(buffer)
    if (!verify(buffer, new_size, ltx_dsign))
    {
        return false;
    }
    *dsign_section = 0;
    IReader tmp_reader(buffer, size);
    m_ltx = new CInifile(&tmp_reader);
    return true;
}
