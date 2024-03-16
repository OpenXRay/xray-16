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
struct xr_token;
class IReader;

class XRCORE_API CInifile
{
public:
    struct XRCORE_API Item
    {
        shared_str name = nullptr;
        shared_str value = nullptr;
    };

    using Items = xr_vector<Item>;

    struct XRCORE_API Sect
    {
        shared_str Name;
        Items Data;

        [[nodiscard]] bool line_exist(pcstr line_name, pcstr* value = nullptr);
    };

    using Root = xr_vector<Sect*>;

    using allow_include_func_t = fastdelegate::FastDelegate1<pcstr, bool>;

    static CInifile* Create(pcstr fileName, bool readOnly = true);
    static void Destroy(CInifile*);
    static bool isBool(pcstr str);

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

    void Load(IReader* reader, pcstr path, allow_include_func_t allow_include_func, u8 include_depth);

    bool insert_new_section(Sect* section);

    [[nodiscard]] Sect& unchecked_read_section(pcstr section_name);
    [[nodiscard]] const Sect& unchecked_read_section(pcstr section_name) const;

public:
    CInifile(IReader* reader, pcstr path = nullptr, allow_include_func_t allow_include_func = nullptr);

    CInifile(pcstr fileName, bool readOnly = true,
             bool loadAtStart = true, bool saveAtEnd = true,
             u32 sect_count = 0, allow_include_func_t allow_include_func = nullptr);

    virtual ~CInifile();

    bool save_as(pcstr new_fname = nullptr);
    void save_as(IWriter& writer, bool bcheck = false) const;
    void set_override_names(bool value) noexcept;
    void save_at_end(bool value) noexcept;
    void set_readonly(bool value) /*noexcept*/;

    [[nodiscard]] Sect& r_section(pcstr section_name);
    [[nodiscard]] const Sect& r_section(pcstr section_name) const;
    [[nodiscard]] Sect& r_section(const shared_str& section_name);
    [[nodiscard]] const Sect& r_section(const shared_str& section_name) const;

    [[nodiscard]] pcstr fname() const /*noexcept*/;
    [[nodiscard]] bool line_exist(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] bool line_exist(const shared_str& section_name, const shared_str& line_name) const;
    [[nodiscard]] u32 line_count(pcstr section_name) const;
    [[nodiscard]] u32 line_count(const shared_str& section_name) const;
    [[nodiscard]] u32 section_count() const;
    [[nodiscard]] bool section_exist(pcstr section_name) const;
    [[nodiscard]] bool section_exist(const shared_str& section_name) const;
    [[nodiscard]] Root& sections();
    [[nodiscard]] const Root& sections() const;

    // Generic reading templated functions
    template<typename T>
    [[nodiscard]] T read(pcstr section_name, pcstr line_name) const;

    template<typename T>
    [[nodiscard]] T read(const shared_str& section_name, pcstr line_name) const
    {
        VERIFY(line_name);

        return read<T>(section_name.c_str(), line_name);
    }

    template<typename T>
    bool try_read(T& out_value, pcstr section_name, pcstr line_name) const;

    template<typename T>
    bool try_read(T& out_value, const shared_str& section_name, pcstr line_name) const
    {
        VERIFY(line_name);

        return try_read<T>(out_value, section_name.c_str(), line_name);
    }

    [[nodiscard]] Sect* try_read_section(pcstr section_name);
    [[nodiscard]] const Sect* try_read_section(pcstr section_name) const;

    // Returns value if it exist, or returns default value
    template<typename T>
    T read_if_exists(pcstr section_name, pcstr line_name, T default_value) const
    {
        VERIFY(section_name);
        VERIFY(line_name);

        if (line_exist(section_name, line_name))
        {
            return read<T>(section_name, line_name);
        }

        return default_value;
    }

    template<typename T>
    T read_if_exists(const shared_str& section_name, pcstr line_name, T default_value) const
    {
        return read_if_exists<T>(section_name.c_str(), line_name, default_value);
    }

    // Returns true if value is exist and assigns it or returns false
    template<typename T>
    bool read_if_exists(T& out_value, pcstr section_name, pcstr line_name) const
    {
        VERIFY(section_name);
        VERIFY(line_name);

        if (line_exist(section_name, line_name))
        {
            out_value = read<T>(section_name, line_name);
            return true;
        }

        return false;
    }

    template<typename T>
    bool read_if_exists(T& out_value, const shared_str& section_name, pcstr line_name) const
    {
        return read_if_exists(out_value, section_name.c_str(), line_name);
    }

    // Returns true if value or fallback value exist, crashes otherwise
    template<typename T>
    bool read_if_exists(
        T& out_value, pcstr section_name, pcstr line_name, pcstr line2_name, bool at_least_one = false) const
    {
        VERIFY(section_name);
        VERIFY(line_name);
        VERIFY(line2_name);

        if (line_exist(section_name, line_name))
        {
            out_value = read<T>(section_name, line_name);
            return true;
        }

        if (line_exist(section_name, line2_name))
        {
            out_value = read<T>(section_name, line2_name);
            return true;
        }

        if (at_least_one)
            xrDebug::Fatal(DEBUG_INFO, "! Ini File[%s]: Can't find line '%s' or '%s' in section '%s'", m_file_name, line_name, line2_name, section_name);

        return false;
    }

    template<typename T>
    bool read_if_exists(T& out_value, const shared_str& section_name, pcstr line_name, pcstr line2_name,
        bool at_least_one = false) const
    {
        return read_if_exists(out_value, section_name.c_str(), line_name, line2_name, at_least_one);
    }

    template<typename T>
    bool try_read_if_exists(T& out_value, pcstr section_name, pcstr line_name) const
    {
        VERIFY(section_name);
        VERIFY(line_name);

        if (line_exist(section_name, line_name))
        {
            return try_read<T>(out_value, section_name, line_name);
        }

        return false;
    }

    template<typename T>
    bool try_read_if_exists(T& out_value, const shared_str& section_name, pcstr line_name) const
    {
        return try_read_if_exists(out_value, section_name.c_str(), line_name);
    }

    // Generic reading functions
    [[nodiscard]] CLASS_ID r_clsid(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] CLASS_ID r_clsid(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] pcstr r_string(pcstr section_name, pcstr line_name) const; // Left quotes in place
    [[nodiscard]] pcstr r_string(const shared_str& section_name, pcstr line_name) const; // Left quotes in place
    [[nodiscard]] shared_str r_string_wb(pcstr section_name, pcstr line_name) const; // Remove quotes
    [[nodiscard]] shared_str r_string_wb(const shared_str& section_name, pcstr line_name) const; // Remove quotes
    [[nodiscard]] u8 r_u8(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] u8 r_u8(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] u16 r_u16(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] u16 r_u16(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] u32 r_u32(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] u32 r_u32(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] u64 r_u64(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] u64 r_u64(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] s8 r_s8(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] s8 r_s8(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] s16 r_s16(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] s16 r_s16(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] s32 r_s32(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] s32 r_s32(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] s64 r_s64(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] s64 r_s64(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] float r_float(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] float r_float(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] Fcolor r_fcolor(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] Fcolor r_fcolor(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] u32 r_color(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] u32 r_color(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] Ivector2 r_ivector2(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] Ivector2 r_ivector2(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] Ivector3 r_ivector3(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] Ivector3 r_ivector3(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] Ivector4 r_ivector4(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] Ivector4 r_ivector4(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] Fvector2 r_fvector2(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] Fvector2 r_fvector2(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] Fvector3 r_fvector3(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] Fvector3 r_fvector3(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] Fvector4 r_fvector4(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] Fvector4 r_fvector4(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] bool r_bool(pcstr section_name, pcstr line_name) const;
    [[nodiscard]] bool r_bool(const shared_str& section_name, pcstr line_name) const;
    [[nodiscard]] int r_token(pcstr section_name, pcstr line_name, const xr_token* token_list) const;
    bool r_line(pcstr section_name, int line_number, pcstr* name_out, pcstr* value_out) const;
    bool r_line(const shared_str& section_name, int line_number, pcstr* name_out, pcstr* value_out) const;

    void w_string(pcstr section_name, pcstr line_name, pcstr value, pcstr comment = nullptr);
    void w_u8(pcstr section_name, pcstr line_name, u8 value, pcstr comment = nullptr);
    void w_u16(pcstr section_name, pcstr line_name, u16 value, pcstr comment = nullptr);
    void w_u32(pcstr section_name, pcstr line_name, u32 value, pcstr comment = nullptr);
    void w_u64(pcstr section_name, pcstr line_name, u64 value, pcstr comment = nullptr);
    void w_s8(pcstr section_name, pcstr line_name, s8 value, pcstr comment = nullptr);
    void w_s16(pcstr section_name, pcstr line_name, s16 value, pcstr comment = nullptr);
    void w_s32(pcstr section_name, pcstr line_name, s32 value, pcstr comment = nullptr);
    void w_s64(pcstr section_name, pcstr line_name, s64 value, pcstr comment = nullptr);
    void w_float(pcstr section_name, pcstr line_name, float value, pcstr comment = nullptr);
    void w_fcolor(pcstr section_name, pcstr line_name, const Fcolor& value, pcstr comment = nullptr);
    void w_color(pcstr section_name, pcstr line_name, u32 value, pcstr comment = nullptr);
    void w_ivector2(pcstr section_name, pcstr line_name, const Ivector2& value, pcstr comment = nullptr);
    void w_ivector3(pcstr section_name, pcstr line_name, const Ivector3& value, pcstr comment = nullptr);
    void w_ivector4(pcstr section_name, pcstr line_name, const Ivector4& value, pcstr comment = nullptr);
    void w_fvector2(pcstr section_name, pcstr line_name, const Fvector2& value, pcstr comment = nullptr);
    void w_fvector3(pcstr section_name, pcstr line_name, const Fvector3& value, pcstr comment = nullptr);
    void w_fvector4(pcstr section_name, pcstr line_name, const Fvector4& value, pcstr comment = nullptr);
    void w_bool(pcstr section_name, pcstr line_name, bool value, pcstr comment = nullptr);

    void remove_line(pcstr section_name, pcstr line_name);
};

#define READ_IF_EXISTS(ltx, method, section, name, default_value) \
    (((ltx)->line_exist(section, name)) ? ((ltx)->method(section, name)) : (default_value))

// Main configuration file
extern XRCORE_API CInifile const* pSettings;
extern XRCORE_API CInifile const* pSettingsAuth;
extern XRCORE_API CInifile const* pSettingsOpenXRay;

extern XRCORE_API bool ltx_multiline_values_enabled;

#endif //__XR_INI_H__
