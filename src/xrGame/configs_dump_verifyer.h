#ifndef CONFIGS_DUMP_VERIFYER
#define CONFIGS_DUMP_VERIFYER

#include "mp_config_sections.h"
#include "xrCore/Crypto/xr_dsa_verifyer.h"

namespace mp_anticheat
{
class dump_verifyer : public xr_dsa_verifyer
{
public:
    dump_verifyer();
    ~dump_verifyer();
}; // class dump_verifyer

class configs_verifyer
{
public:
    configs_verifyer();
    ~configs_verifyer();
    bool verify(u8* data, u32 data_size, string256& diff);

private:
    bool verify_dsign(u8* data, u32 data_size, crypto::xr_sha1::hash_t& sha_checksum);
    LPCSTR get_diff(CInifile& received, CInifile& active_params, string256& dst_diff);
    LPCSTR get_section_diff(CInifile::Sect* sect_ptr, CInifile& active_params, string256& dst_diff);
    CMemoryWriter m_orig_config_body;
    u32 m_orig_config_end_pos;

    dump_verifyer m_verifyer;

    mp_config_sections m_original_config;
    mp_active_params m_original_ap;
}; // configs_verifyer

} // namespace mp_anticheat

#endif //#ifndef CONFIGS_DUMP_VERIFYER
