#pragma once
#ifndef xr_iniH
#define xr_iniH

#include "fastdelegate.h"
#include "xrCore/xrstring.h"
#include "xrCore/_flags.h"
#include "xrCore/_color.h"
#include "xrCore/_vector2.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_vector4.h"
#include "xrCore/clsid.h"
#include "xrCommon/xr_vector.h"

constexpr pcstr OPENXRAY_INI_SECTION = "openxray";

// refs
class CInifile;
struct xr_token;
class IReader;

class XRCORE_API CInifile
{
public:
    struct XRCORE_API Item
    {
        shared_str first;
        shared_str second;
        //#ifdef DEBUG
        // shared_str comment;
        //#endif
        Item()
            : first(nullptr), second(nullptr)
              //#ifdef DEBUG
              // , comment(0)
              //#endif
              {};
    };

    using Items = xr_vector<Item>;

    struct XRCORE_API Sect
    {
        shared_str Name;
        Items Data;

        bool line_exist(pcstr line, pcstr* value = nullptr);
    };

    using Root = xr_vector<Sect*>;

    using allow_include_func_t = fastdelegate::FastDelegate1<pcstr, bool>;

    static CInifile* Create(pcstr fileName, bool readOnly = true);
    static void Destroy(CInifile*);
    static bool isBool(pcstr str)
    {
        return xr_strcmp(str, "on") == 0 || xr_strcmp(str, "yes") == 0 || xr_strcmp(str, "true") == 0 || xr_strcmp(str, "1") == 0;
    }

private:
    enum
    {
        eSaveAtEnd = 1 << 0,
        eReadOnly = 1 << 1,
        eOverrideNames = 1 << 2,
    };
    Flags8 m_flags;
    string_path m_file_name;
    Root DATA;

    void Load(IReader* F, pcstr path, allow_include_func_t allow_include_func = nullptr);

public:
    CInifile(IReader* F, pcstr path = nullptr, allow_include_func_t allow_include_func = nullptr);

    CInifile(pcstr fileName, bool readOnly = true,
             bool loadAtStart = true, bool saveAtEnd = true,
             u32 sect_count = 0, allow_include_func_t allow_include_func = nullptr);

    virtual ~CInifile();
    bool save_as(pcstr new_fname = nullptr);
    void save_as(IWriter& writer, bool bcheck = false) const;
    void set_override_names(bool b) noexcept { m_flags.set(eOverrideNames, b); }
    void save_at_end(bool b) noexcept { m_flags.set(eSaveAtEnd, b); }
    pcstr fname() const noexcept { return m_file_name; };
    Sect& r_section(pcstr S) const;
    Sect& r_section(const shared_str& S) const;
    bool line_exist(pcstr S, pcstr L)const;
    bool line_exist(const shared_str& S, const shared_str& L)const;
    u32 line_count(pcstr S) const;
    u32 line_count(const shared_str& S) const;
    u32 section_count() const;
    bool section_exist(pcstr S) const;
    bool section_exist(const shared_str& S) const;
    Root& sections() { return DATA; }
    Root const& sections() const { return DATA; }

    template<typename T>
    T read_if_exists(pcstr section, pcstr line, T defaultValue) const;

    template<typename T>
    T read_if_exists(const shared_str& section, pcstr line, T defaultValue) const;

    CLASS_ID r_clsid(pcstr S, pcstr L) const;
    CLASS_ID r_clsid(const shared_str& S, pcstr L) const { return r_clsid(*S, L); }
    pcstr r_string(pcstr S, pcstr L) const; // Left quotes in place
    pcstr r_string(const shared_str& S, pcstr L) const { return r_string(*S, L); } // Left quotes in place
    shared_str r_string_wb(pcstr S, pcstr L) const; // Remove quotes
    shared_str r_string_wb(const shared_str& S, pcstr L) const { return r_string_wb(*S, L); } // Remove quotes
    u8 r_u8(pcstr S, pcstr L) const;
    u8 r_u8(const shared_str& S, pcstr L) const { return r_u8(*S, L); }
    u16 r_u16(pcstr S, pcstr L) const;
    u16 r_u16(const shared_str& S, pcstr L) const { return r_u16(*S, L); }
    u32 r_u32(pcstr S, pcstr L) const;
    u32 r_u32(const shared_str& S, pcstr L) const { return r_u32(*S, L); }
    u64 r_u64(pcstr S, pcstr L) const;
    s8 r_s8(pcstr S, pcstr L) const;
    s8 r_s8(const shared_str& S, pcstr L) const { return r_s8(*S, L); }
    s16 r_s16(pcstr S, pcstr L) const;
    s16 r_s16(const shared_str& S, pcstr L) const { return r_s16(*S, L); }
    s32 r_s32(pcstr S, pcstr L) const;
    s32 r_s32(const shared_str& S, pcstr L) const { return r_s32(*S, L); }
    s64 r_s64(pcstr S, pcstr L) const;
    float r_float(pcstr S, pcstr L) const;
    float r_float(const shared_str& S, pcstr L) const { return r_float(*S, L); }
    Fcolor r_fcolor(pcstr S, pcstr L) const;
    Fcolor r_fcolor(const shared_str& S, pcstr L) const { return r_fcolor(*S, L); }
    u32 r_color(pcstr S, pcstr L) const;
    u32 r_color(const shared_str& S, pcstr L) const { return r_color(*S, L); }
    Ivector2 r_ivector2(pcstr S, pcstr L) const;
    Ivector2 r_ivector2(const shared_str& S, pcstr L) const { return r_ivector2(*S, L); }
    Ivector3 r_ivector3(pcstr S, pcstr L) const;
    Ivector3 r_ivector3(const shared_str& S, pcstr L) const { return r_ivector3(*S, L); }
    Ivector4 r_ivector4(pcstr S, pcstr L) const;
    Ivector4 r_ivector4(const shared_str& S, pcstr L) const { return r_ivector4(*S, L); }
    Fvector2 r_fvector2(pcstr S, pcstr L) const;
    Fvector2 r_fvector2(const shared_str& S, pcstr L) const { return r_fvector2(*S, L); }
    Fvector3 r_fvector3(pcstr S, pcstr L) const;
    Fvector3 r_fvector3(const shared_str& S, pcstr L) const { return r_fvector3(*S, L); }
    Fvector4 r_fvector4(pcstr S, pcstr L) const;
    Fvector4 r_fvector4(const shared_str& S, pcstr L) const { return r_fvector4(*S, L); }
    bool r_bool(pcstr S, pcstr L) const;
    bool r_bool(const shared_str& S, pcstr L) const { return r_bool(*S, L); }
    int r_token(pcstr S, pcstr L, const xr_token* token_list) const;
    bool r_line(pcstr S, int L, pcstr* N, pcstr* V) const;
    bool r_line(const shared_str& S, int L, pcstr* N, pcstr* V) const;

    void w_string(pcstr S, pcstr L, pcstr V, pcstr comment = nullptr);
    void w_u8(pcstr S, pcstr L, u8 V, pcstr comment = nullptr);
    void w_u16(pcstr S, pcstr L, u16 V, pcstr comment = nullptr);
    void w_u32(pcstr S, pcstr L, u32 V, pcstr comment = nullptr);
    void w_u64(pcstr S, pcstr L, u64 V, pcstr comment = nullptr);
    void w_s64(pcstr S, pcstr L, s64 V, pcstr comment = nullptr);
    void w_s8(pcstr S, pcstr L, s8 V, pcstr comment = nullptr);
    void w_s16(pcstr S, pcstr L, s16 V, pcstr comment = nullptr);
    void w_s32(pcstr S, pcstr L, s32 V, pcstr comment = nullptr);
    void w_float(pcstr S, pcstr L, float V, pcstr comment = nullptr);
    void w_fcolor(pcstr S, pcstr L, const Fcolor& V, pcstr comment = nullptr);
    void w_color(pcstr S, pcstr L, u32 V, pcstr comment = nullptr);
    void w_ivector2(pcstr S, pcstr L, const Ivector2& V, pcstr comment = nullptr);
    void w_ivector3(pcstr S, pcstr L, const Ivector3& V, pcstr comment = nullptr);
    void w_ivector4(pcstr S, pcstr L, const Ivector4& V, pcstr comment = nullptr);
    void w_fvector2(pcstr S, pcstr L, const Fvector2& V, pcstr comment = nullptr);
    void w_fvector3(pcstr S, pcstr L, const Fvector3& V, pcstr comment = nullptr);
    void w_fvector4(pcstr S, pcstr L, const Fvector4& V, pcstr comment = nullptr);
    void w_bool(pcstr S, pcstr L, bool V, pcstr comment = nullptr);

    void remove_line(pcstr S, pcstr L);
};

#define READ_IF_EXISTS(ltx, method, section, name, default_value) \
    (((ltx)->line_exist(section, name)) ? ((ltx)->method(section, name)) : (default_value))

// Main configuration file
extern XRCORE_API CInifile const* pSettings;
extern XRCORE_API CInifile const* pSettingsAuth;
extern XRCORE_API CInifile const* pSettingsOpenXRay;

#endif //__XR_INI_H__
