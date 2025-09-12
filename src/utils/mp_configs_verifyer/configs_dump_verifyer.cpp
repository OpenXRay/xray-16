#include "pch.h"
#include "configs_dump_verifyer.h"
#include "configs_common.h"

char const* cd_info_secion = "config_dump_info";
char const* cd_player_name_key = "player_name";
char const* cd_player_digest_key = "player_digest";
char const* cd_digital_sign_key = "digital_sign";
char const* cd_creation_date = "creation_date";

namespace mp_anticheat
{
dump_verifyer::dump_verifyer() : xr_dsa_verifyer(p_number, q_number, g_number, public_key) {}
dump_verifyer::~dump_verifyer() {}
configs_verifyer::configs_verifyer()
{
    m_original_config.start_dump();
    while (m_original_config.dump_one(m_orig_config_body))
    {
    };
    m_orig_config_end_pos = m_orig_config_body.tell();
}

configs_verifyer::~configs_verifyer() {}
static char* search_info_section(u8* buffer, u32 buffer_size)
{
    u32 sstr_size = xr_strlen(cd_info_secion);
    VERIFY(buffer_size >= sstr_size);
    u8* rbegin = buffer + (buffer_size - sstr_size);
    int r_size = static_cast<int>(buffer_size - sstr_size);
    do
    {
        if (!memcmp(rbegin, cd_info_secion, sstr_size))
        {
            return static_cast<char*>((void*)rbegin);
        }
        --rbegin;
        --r_size;
    } while (r_size > 0);
    return nullptr;
}

bool const configs_verifyer::verify_dsign(u8* data, u32 data_size, crypto::xr_sha1::hash_t& sha_checksum)
{
    char* tmp_info_sect = search_info_section(data, data_size);
    if (!tmp_info_sect)
        return false;

    --tmp_info_sect;
    u32 tmp_info_sect_size = xr_strlen(tmp_info_sect);
    IReader tmp_reader(tmp_info_sect, tmp_info_sect_size);
    CInifile tmp_ini(&tmp_reader);

    if (!tmp_ini.line_exist(cd_info_secion, cd_player_name_key) ||
        !tmp_ini.line_exist(cd_info_secion, cd_player_digest_key) ||
        !tmp_ini.line_exist(cd_info_secion, cd_creation_date) ||
        !tmp_ini.line_exist(cd_info_secion, cd_digital_sign_key))
    {
        return false;
    }

    char* dst_buffer = tmp_info_sect;
    *dst_buffer = 0;
    u32 dst_size = static_cast<u32>((data + data_size) - (u8*)dst_buffer);
    u32 src_data_size = data_size - dst_size;

    LPCSTR add_str = nullptr;
    STRCONCAT(add_str, tmp_ini.r_string(cd_info_secion, cd_player_name_key),
        tmp_ini.r_string(cd_info_secion, cd_player_digest_key), tmp_ini.r_string(cd_info_secion, cd_creation_date));

    shared_str tmp_dsign = tmp_ini.r_string(cd_info_secion, cd_digital_sign_key);

    xr_strcat(dst_buffer, dst_size, add_str);
    src_data_size += xr_strlen(dst_buffer) + 1; // zero ending

    auto hash = m_verifyer.verify(data, src_data_size, tmp_dsign);
    if (!hash)
        return false;

    CopyMemory(sha_checksum.data(), hash.value().data(), crypto::xr_sha1::DigestSize);

    return true;
}

LPCSTR configs_verifyer::get_section_diff(CInifile::Sect* sect_ptr, CInifile& active_params, string256& dst_diff)
{
    pcstr diff_str = nullptr;
    bool tmp_active_param = false;
    if (!strncmp(sect_ptr->Name.c_str(), "ap_", 3))
    {
        tmp_active_param = true;
    }

    for (auto cit = sect_ptr->Data.cbegin(), ciet = sect_ptr->Data.cend(); cit != ciet; ++cit)
    {
        shared_str const& tmp_value = cit->second;
        shared_str real_value;
        if (tmp_active_param)
        {
            if (active_params.line_exist(sect_ptr->Name.c_str(), cit->first))
            {
                real_value = active_params.r_string(sect_ptr->Name.c_str(), cit->first.c_str());
                if (tmp_value != real_value)
                {
                    pcstr tmp_key_str = nullptr;
                    STRCONCAT(tmp_key_str, sect_ptr->Name.c_str(), "::", cit->first.c_str());
                    STRCONCAT(diff_str, tmp_key_str, " = ", tmp_value.c_str(), ",right = ", real_value.c_str());
                    strncpy_s(dst_diff, diff_str, sizeof(dst_diff) - 1);
                    dst_diff[sizeof(dst_diff) - 1] = 0;
                    return dst_diff;
                }
                continue;
            }
        }
        if (!pSettings->line_exist(sect_ptr->Name, cit->first))
        {
            STRCONCAT(diff_str, "line ", sect_ptr->Name.c_str(), "::", cit->first.c_str(), " not found");
            strncpy_s(dst_diff, diff_str, sizeof(dst_diff) - 1);
            dst_diff[sizeof(dst_diff) - 1] = 0;
            return dst_diff;
        }
        real_value = pSettings->r_string(sect_ptr->Name.c_str(), cit->first.c_str());
        if (tmp_value != real_value)
        {
            pcstr tmp_key_str = nullptr;
            STRCONCAT(tmp_key_str, sect_ptr->Name.c_str(), "::", cit->first.c_str());
            STRCONCAT(diff_str, tmp_key_str, " = ", tmp_value.c_str(), ",right = ", real_value.c_str());
            strncpy_s(dst_diff, diff_str, sizeof(dst_diff) - 1);
            dst_diff[sizeof(dst_diff) - 1] = 0;
            return dst_diff;
        }
    }
    return nullptr;
}

LPCSTR configs_verifyer::get_diff(CInifile& received, CInifile& active_params, string256& dst_diff)
{
    pcstr diff_str = nullptr;
    for (auto sit = received.sections().begin(), siet = received.sections().end(); sit != siet; ++sit)
    {
        CInifile::Sect* tmp_sect = *sit;
        if (tmp_sect->Name == cd_info_secion)
            continue;
        if (tmp_sect->Name == active_params_section)
            continue;

        diff_str = get_section_diff(tmp_sect, active_params, dst_diff);
        if (diff_str)
        {
            return diff_str;
        }
    }
    xr_strcpy(dst_diff, "unknown diff or currepted config dump");
    return dst_diff;
}

bool const configs_verifyer::verify(u8* data, u32 data_size, string256& diff)
{
    if (!strstr(static_cast<char const*>((void*)data), cd_info_secion))
    {
        xr_strcpy(diff, "invalid data");
        return false;
    }
    IReader tmp_reader(data, data_size);
    CInifile tmp_ini(&tmp_reader);
    CInifile tmp_active_params(nullptr, FALSE, FALSE, FALSE);

    string16 tmp_digit;
    u32 ap_index = 1;
    xr_sprintf(tmp_digit, "%d", ap_index);
    while (tmp_ini.line_exist(active_params_section, tmp_digit))
    {
        LPCSTR tmp_ap_section = tmp_ini.r_string(active_params_section, tmp_digit);
        tmp_active_params.w_string(active_params_section, tmp_digit, tmp_ap_section);
        m_original_ap.load_to(tmp_ap_section, tmp_active_params);
        ++ap_index;
        xr_sprintf(tmp_digit, "%d", ap_index);
    }

    m_orig_config_body.seek(m_orig_config_end_pos);
    tmp_active_params.save_as(m_orig_config_body);

    if (!tmp_ini.line_exist(cd_info_secion, cd_player_name_key) ||
        !tmp_ini.line_exist(cd_info_secion, cd_player_digest_key) ||
        !tmp_ini.line_exist(cd_info_secion, cd_creation_date) ||
        !tmp_ini.line_exist(cd_info_secion, cd_digital_sign_key))
    {
        xr_strcpy(diff, "invalid dump");
        return false;
    }

    LPCSTR add_str = nullptr;
    STRCONCAT(add_str, tmp_ini.r_string(cd_info_secion, cd_player_name_key),
        tmp_ini.r_string(cd_info_secion, cd_player_digest_key), tmp_ini.r_string(cd_info_secion, cd_creation_date));

    m_orig_config_body.w_stringZ(add_str);

    auto hash = crypto::xr_sha1::calculate(m_orig_config_body.pointer(), m_orig_config_body.tell());

    crypto::xr_sha1::hash_t tmp_checksum{};
    if (!verify_dsign(data, data_size, tmp_checksum))
    {
        xr_strcpy(diff, "invalid digital sign");
        return false;
    }

    if (memcmp(tmp_checksum.data(), hash.data(), crypto::xr_sha1::DigestSize))
    {
        get_diff(tmp_ini, tmp_active_params, diff);
        return false;
    }
    return true;
}

} // namespace mp_anticheat
