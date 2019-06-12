#ifndef SCREENSHOT_READER_INCLUDED
#define SCREENSHOT_READER_INCLUDED

#include "xrCore/Crypto/xr_dsa_verifyer.h"

namespace screenshots
{
class sign_verifyer : public xr_dsa_verifyer
{
public:
    sign_verifyer();
    ~sign_verifyer();
};

class reader
{
public:
    reader(IReader* freader);
    ~reader();

    shared_str const player_name();
    shared_str const player_cdkey_digest();
    // shared_str const admin_name				();
    shared_str const creation_date();

    bool const verify();
    bool const is_valid() const { return m_info_section != NULL; };
private:
    reader() = default;
    u8* m_jpeg_data;
    size_t m_jpeg_data_size;

    size_t m_info_pos;
    size_t m_info_size;

    CInifile* m_info_section;
    sign_verifyer m_verifyer;

}; // class reader

} // namespace screenshots

#endif //#ifndef SCREENSHOT_READER_INCLUDED
