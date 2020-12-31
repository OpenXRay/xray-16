#include "pch.h"
#include "screenshots_writer.h"
#include "screenshots_common.h"

namespace screenshots
{
writer::writer(u8* jpeg_data, u32 jpeg_size, u32 jpeg_buffer_size) : m_info_data(NULL, FALSE, FALSE, FALSE)
{
    VERIFY(jpeg_buffer_size - jpeg_size >= info_max_size);
    m_buffer = jpeg_data;
    m_buffer_size = jpeg_buffer_size;
    m_buffer_info_pos = jpeg_size;
}

writer::~writer() {}
char const* ss_info_secion = "screenshot_info";
char const* ss_player_name_key = "player_name";
char const* ss_player_digest_key = "player_digest";
char const* ss_admin_name_key = "admin_name";
char const* ss_digital_sign_key = "digital_sign";
char const* ss_creation_date = "creation_date";

void writer::set_player_name(shared_str const& pname)
{
    m_info_data.w_string(ss_info_secion, ss_player_name_key, pname.c_str());
}
void writer::set_player_cdkey_digest(shared_str const& cdkeydigest)
{
    m_info_data.w_string(ss_info_secion, ss_player_digest_key, cdkeydigest.c_str());
}
/*void writer::set_admin_name				(shared_str const & admin_name)
{
    m_info_data.w_string(ss_info_secion, ss_admin_name_key, admin_name.c_str());
}*/

static char const* current_time(string64& dest_time)
{
    time_t tmp_curr_time;

    dest_time[0] = 0;
    _time64(&tmp_curr_time);
    tm* tmp_tm = _localtime64(&tmp_curr_time);

    xr_sprintf(dest_time, sizeof(dest_time), "%02d.%02d.%d_%02d:%02d:%02d", tmp_tm->tm_mday, tmp_tm->tm_mon + 1,
        tmp_tm->tm_year + 1900, tmp_tm->tm_hour, tmp_tm->tm_min, tmp_tm->tm_sec);
    return dest_time;
}

u32 const writer::write_info(crypto::yielder_t* yielder)
{
    string64 time_string;
    m_info_data.w_string(ss_info_secion, ss_creation_date, current_time(time_string));

    char* info_start = static_cast<char*>((void*)(m_buffer + m_buffer_info_pos));

    info_start[0] = 0;
    xr_strcat(info_start, info_max_size, m_info_data.r_string(ss_info_secion, ss_player_name_key));
    xr_strcat(info_start, info_max_size, m_info_data.r_string(ss_info_secion, ss_player_digest_key));
    // xr_strcat(info_start, info_max_size, m_info_data.r_string(ss_info_secion, ss_admin_name_key));
    xr_strcat(info_start, info_max_size, time_string);

    u32 info_size = xr_strlen(info_start) + 1;
    u32 jpeg_data_size = m_buffer_info_pos + info_size;

    shared_str tmp_sign_res;
    if (yielder && *yielder)
    {
        tmp_sign_res = m_signer.sign_mt(m_buffer, jpeg_data_size, *yielder);
    }
    else
    {
        tmp_sign_res = m_signer.sign(m_buffer, jpeg_data_size);
    }

    m_info_data.w_string(ss_info_secion, ss_digital_sign_key, tmp_sign_res.c_str());

    CMemoryWriter tmp_writer;
    m_info_data.save_as(tmp_writer);
    CopyMemory(m_buffer + m_buffer_info_pos, tmp_writer.pointer(), tmp_writer.size());

    return m_buffer_info_pos + tmp_writer.size();
}

// signer

signer::signer() : xr_dsa_signer(p_number, q_number, g_number) { feel_private_dsa_key(); }
signer::~signer() {}
void signer::feel_private_dsa_key()
{
    // Private key:
    m_private_key.m_value[0] = 0xa6;
    m_private_key.m_value[1] = 0x07;
    m_private_key.m_value[2] = 0x3b;
    m_private_key.m_value[3] = 0x2b;
    m_private_key.m_value[4] = 0x1d;
    m_private_key.m_value[5] = 0xfe;
    m_private_key.m_value[6] = 0xdf;
    m_private_key.m_value[7] = 0x48;
    m_private_key.m_value[8] = 0x36;
    m_private_key.m_value[9] = 0x8e;
    m_private_key.m_value[10] = 0xad;
    m_private_key.m_value[11] = 0x95;
    m_private_key.m_value[12] = 0xf8;
    m_private_key.m_value[13] = 0x4e;
    m_private_key.m_value[14] = 0x9a;
    m_private_key.m_value[15] = 0xd0;
    m_private_key.m_value[16] = 0x55;
    m_private_key.m_value[17] = 0xbb;
    m_private_key.m_value[18] = 0xa2;
    m_private_key.m_value[19] = 0x3a;
}
}
