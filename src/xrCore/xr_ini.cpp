#include "stdafx.h"

#include "FileSystem.h"
#include "ParsingUtils.hpp"
#include "xrCore/xr_token.h"

XRCORE_API CInifile const* pSettings = nullptr;
XRCORE_API CInifile const* pSettingsAuth = nullptr;
XRCORE_API CInifile const* pSettingsOpenXRay = nullptr;

XRCORE_API bool ltx_multiline_values_enabled = true;

#if defined(XR_PLATFORM_LINUX) || defined(XR_PLATFORM_BSD) || defined(XR_PLATFORM_APPLE)
#include <cstdint>
#define MSVCRT_EINVAL 22
#define MSVCRT_ERANGE 34

#define MSVCRT_UI64_MAX   (((uint64_t)0xffffffff << 32) | 0xffffffff)

/**
 * from wine@dlls/msvcrt/string.c
 *
 * @param fileName
 * @param readOnly
 * @return
 */
int _cdecl _i64toa_s(int64_t value, char *str, size_t size, int radix)
{
    uint64_t val;
    unsigned int digit;
    int is_negative;
    char buffer[65], *pos;
    size_t len;

    if (!(str != NULL))
        return MSVCRT_EINVAL;
    if (!(size > 0))
        return MSVCRT_EINVAL;
    if (!(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    if (value < 0 && radix == 10)
    {
        is_negative = 1;
        val = -value;
    }
    else
    {
        is_negative = 0;
        val = value;
    }

    pos = buffer + 64;
    *pos = '\0';

    do
    {
        digit = val % radix;
        val /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    } while (val != 0);

    if (is_negative)
        *--pos = '-';

    len = buffer + 65 - pos;
    if (len > size)
    {
        size_t i;
        char *p = str;

        /* Copy the temporary buffer backwards up to the available number of
         * characters. Don't copy the negative sign if present. */

        if (is_negative)
        {
            p++;
            size--;
        }

        for (pos = buffer + 63, i = 0; i < size; i++)
            *p++ = *pos--;

        str[0] = '\0';
        return MSVCRT_ERANGE;
    }

    memcpy(str, pos, len);
    return 0;
}

int _cdecl _ui64toa_s(uint64_t value, char *str, size_t size, int radix)
{
    char buffer[65], *pos;
    int digit;

    if (!(str != NULL))
        return MSVCRT_EINVAL;
    if (!(size > 0))
        return MSVCRT_EINVAL;
    if (!(radix >= 2 && radix <= 36))
    {
        str[0] = '\0';
        return MSVCRT_EINVAL;
    }

    pos = buffer + 64;
    *pos = '\0';

    do
    {
        digit = value % radix;
        value /= radix;

        if (digit < 10)
            *--pos = '0' + digit;
        else
            *--pos = 'a' + digit - 10;
    } while (value != 0);

    if (static_cast<size_t>(buffer - pos + 65) > size)
    {
        return MSVCRT_EINVAL;
    }

    memcpy(str, pos, buffer - pos + 65);
    return 0;
}

LARGE_INTEGER _cdecl _atoi64(const char *str)
{
    ULARGE_INTEGER RunningTotal = 0;
    char bMinus = 0;

    while (*str == ' ' || (*str >= '\011' && *str <= '\015'))
    {
        str++;
    } /* while */

    if (*str == '+')
        str++;
    else if (*str == '-')
    {
        bMinus = 1;
        str++;
    } /* if */

    while (*str >= '0' && *str <= '9')
    {
        RunningTotal = RunningTotal * 10 + *str - '0';
        str++;
    } /* while */

    return bMinus ? -RunningTotal : RunningTotal;
}

uint64_t _cdecl _strtoui64_l(const char *nptr, char **endptr, int base, locale_t locale)
{
    BOOL negative = FALSE;
    uint64_t ret = 0;

    if (!(nptr != NULL))
        return 0;
    if (!(base == 0 || base >= 2))
        return 0;
    if (!(base <= 36))
        return 0;

    while (isspace(*nptr))
        nptr++;

    if (*nptr == '-')
    {
        negative = TRUE;
        nptr++;
    }
    else if (*nptr == '+')
        nptr++;

    if ((base == 0 || base == 16) && *nptr == '0' && tolower(*(nptr + 1)) == 'x')
    {
        base = 16;
        nptr += 2;
    }

    if (base == 0)
    {
        if (*nptr == '0')
            base = 8;
        else
            base = 10;
    }

    while (*nptr)
    {
        char cur = tolower(*nptr);
        int v;

        if (isdigit(cur))
        {
            if (cur >= '0' + base)
                break;
            v = *nptr - '0';
        }
        else
        {
            if (cur < 'a' || cur >= 'a' + base - 10)
                break;
            v = cur - 'a' + 10;
        }

        nptr++;

        if (ret > MSVCRT_UI64_MAX / base || ret * base > MSVCRT_UI64_MAX - v)
            ret = MSVCRT_UI64_MAX;
        else
            ret = ret * base + v;
    }

    if (endptr)
        *endptr = (char*) nptr;

    return negative ? -ret : ret;
}

uint64_t _cdecl _strtoui64(const char *nptr, char **endptr, int base)
{
    return _strtoui64_l(nptr, endptr, base, NULL);
}
#endif

CInifile* CInifile::Create(pcstr fileName, bool readOnly)
{
    return xr_new<CInifile>(fileName, readOnly);
}

void CInifile::Destroy(CInifile* ini) { xr_delete(ini); }

bool CInifile::isBool(pcstr str)
{
    VERIFY(str);

    return xr_strcmp(str, "on") == 0 || xr_strcmp(str, "yes") == 0 || xr_strcmp(str, "true") == 0 ||
        xr_strcmp(str, "1") == 0;
}

bool sect_pred(const CInifile::Sect* x, pcstr val)
{
    VERIFY(x);

    return xr_strcmp(*x->Name, val) < 0;
}

bool item_pred(const CInifile::Item& x, pcstr val)
{
    if (!x.name || !val)
        return x.name < val;

    return xr_strcmp(*x.name, val) < 0;
}

XRCORE_API bool _parse(pstr dest, pcstr src)
{
    VERIFY(dest);
    VERIFY(src);

    bool bInsideSTR = false;

    while (*src)
    {
        if (std::isspace(*src))
        {
            if (bInsideSTR)
            {
                *dest++ = *src++;
                continue;
            }

            src = ParseAllSpaces(src);

            continue;
        }

        if (*src == '"')
            bInsideSTR = !bInsideSTR;

        *dest++ = *src++;
    }
    *dest = 0;
    return bInsideSTR;
}

XRCORE_API void _decorate(pstr dest, pcstr src)
{
    VERIFY(dest);
    VERIFY(src);

    bool bInsideSTR = false;
    while (*src)
    {
        if (*src == ',')
        {
            if (bInsideSTR)
                *dest++ = *src++;
            else
            {
                *dest++ = *src++;
                *dest++ = ' ';
            }

            continue;
        }
        if (*src == '"')
            bInsideSTR = !bInsideSTR;

        *dest++ = *src++;
    }
    *dest = 0;
}

//------------------------------------------------------------------------------

bool CInifile::Sect::line_exist(pcstr line_name, pcstr* value)
{
    VERIFY(line_name);
    VERIFY(value);

    auto iterator = std::lower_bound(Data.begin(), Data.end(), line_name, item_pred);
    if (iterator != Data.end() && xr_strcmp(*iterator->name, line_name) == 0)
    {
        if (value)
            *value = *iterator->value;

        return true;
    }

    return false;
}

//------------------------------------------------------------------------------

CInifile::CInifile(IReader* reader, pcstr path, allow_include_func_t allow_include_func)
{
    m_file_name[0] = 0;
    m_flags.zero();
    m_flags.set(eSaveAtEnd, false);
    m_flags.set(eReadOnly, true);
    m_flags.set(eOverrideNames, false);

    Load(reader, path, allow_include_func, 0);
}

CInifile::CInifile(pcstr fileName, bool readOnly, bool loadAtStart, bool saveAtEnd, u32 sect_count,
    allow_include_func_t allow_include_func)
{
    if (fileName && strstr(fileName, "system"))
        Msg("-----loading %s", fileName);

    if (fileName)
        xr_strcpy(m_file_name, sizeof(m_file_name), fileName);
    else
        m_file_name[0] = 0;

    m_flags.zero();
    m_flags.set(eSaveAtEnd, saveAtEnd);
    m_flags.set(eReadOnly, readOnly);

    if (!loadAtStart)
        return;

    IReader* reader = FS.r_open(fileName);
    if (!reader)
    {
        Msg("! Ini File: Can't open file: '%s'", fileName);
        return;
    }

    const xr_string path = EFS_Utils::ExtractFilePath(m_file_name);
    DATA.reserve(sect_count);

    Load(reader, path.c_str(), allow_include_func, 0);
    FS.r_close(reader);
}

CInifile::~CInifile()
{
    if (!m_flags.test(eReadOnly) && m_flags.test(eSaveAtEnd) && !save_as())
        Log("! Ini File[%s]: Can't save file:", m_file_name);

    for (auto* val : DATA)
        xr_delete(val);
}

void insert_item(CInifile::Sect* target, const CInifile::Item& item)
{
    VERIFY(target);

    auto sect_it = std::lower_bound(target->Data.begin(), target->Data.end(), *item.name, item_pred);
    if (sect_it != target->Data.end() && sect_it->name.equal(item.name))
        sect_it->value = item.value;
    else
        target->Data.emplace(sect_it, std::move(item));
}

void CInifile::Load(IReader* reader, pcstr path, allow_include_func_t allow_include_func, u8 include_depth)
{
    VERIFY(reader);

    if (include_depth >= 128)
    {
        Msg("! Ini File[%s]: parsing failed. Maximum include depth reached (> 128)", path);
        return;
    }

    Sect* Current = nullptr;
    while (!reader->eof())
    {
        string4096 str;
        if (!reader->try_r_string(str, sizeof(str)))
        {
            xr_delete(Current);
            Msg("! Ini File[%s]: parsing failed. Line is too long (>= 4096)", path);
            return;
        }

        // Skip any leading whitespaces
        pstr ptr = ParseAllSpaces(str);

        // Stop parsing if the line is empty
        if (*ptr == '\0')
            continue;

        // Stop parsing if the line is a comment
        if (*ptr == ';' || (*ptr == '/' && ptr[1] == '/'))
            continue;

        // Parse includes
        pcstr include_name;
        switch (ParseInclude(ptr, include_name))
        {
        case ParseIncludeResult::Success:
            R_ASSERT(path && path[0]);

            string_path file_path;
            strconcat(sizeof(file_path), file_path, path, include_name);

            if (!allow_include_func || allow_include_func(file_path))
            {
                IReader* I = FS.r_open(file_path);
#ifndef XR_PLATFORM_WINDOWS // XXX: replace with runtime check for case-sensitivity
                if (I == nullptr)
                {
                    xr_fs_nostrlwr(const_cast<pstr>(include_name));
                    strconcat(file_path, path, include_name);
                    I = FS.r_open(file_path);
                }
#endif
                if (!I)
                {
                    xr_delete(Current);
                    Msg("! Ini File[%s]: Can't open include file: '%s', file path: '%s'", path, include_name, file_path);
                    return;
                }

                const xr_string include_path = EFS_Utils::ExtractFilePath(file_path);
                Load(I, include_path.c_str(), allow_include_func, include_depth + 1);
                FS.r_close(I);
            }
            continue;

        case ParseIncludeResult::NoInclude:
            // Continue parsing further down
            break;

        case ParseIncludeResult::Error:
            xr_delete(Current);
            Msg("! Ini File[%s]: invalid include directive: '%s'", path, str);
            return;
        }

        // Parse new section
        if (*ptr == '[')
        {
            ++ptr;

            // insert previous filled section if theres one, if that fails just stop parsing
            if (Current && !insert_new_section(Current))
                return;

            // Start a new section
            Current = xr_new<Sect>();
            VERIFY(Current);

            // Parse section name and convert it to lowercase
            pcstr name = ptr;
            while (*ptr != '\0')
            {
                if (*ptr == ']')
                    break;
                else
                {
                    *ptr = std::tolower(*ptr);
                    ++ptr;
                }
            }

            // Check for missing closing bracket
            if (*ptr == '\0')
            {
                xr_delete(Current);
                Msg("! Ini File[%s]: Bad ini section found. Missing closing bracket: '%s'", path, str);
                return;
            }

            // Replace the closing bracket with a zero to correctly null terminate name
            *ptr = '\0';
            ++ptr;

            Current->Name = name;

            // Skip any whitespace after the closing bracket
            ptr = ParseAllSpaces(ptr);

            // Parse inherited sections
            if (*ptr == ':')
            {
                VERIFY2(m_flags.test(eReadOnly), "Allow for readonly mode only.");

                ++ptr;

                // Skip any whitespace after the colon
                ptr = ParseAllSpaces(ptr);

                static constexpr size_t max_inherited_sections = 128;

                pcstr inherited_section_name = ptr;
                pcstr inherited_sections[max_inherited_sections];
                u32 count = 0;
                u32 total_count = 0;

                auto add_inherited_section = [&]() -> bool {
                    // Just skip empty section names
                    if (*inherited_section_name == '\0')
                        return true;

                    // Check that the inherited section exists
                    const Sect* inherited_section = try_read_section(inherited_section_name);
                    if (!inherited_section)
                    {
                        xr_delete(Current);
                        Msg("! Ini File[%s]: Bad inherited section not found: '%s'", path, inherited_section_name);
                        return false;
                    }
                    total_count += inherited_section->Data.size();

                    inherited_sections[count++] = inherited_section_name;
                    if (count >= max_inherited_sections)
                    {
                        xr_delete(Current);
                        Msg("! Ini File[%s]: Too many inherited sections (>= 128)", path);
                        return false;
                    }

                    return true;
                };

                // Parse all inherited sections
                while (*ptr != '\0')
                {
                    // Commas separate different inherited sections
                    if (*ptr == ',')
                    {
                        *ptr = '\0';

                        if (!add_inherited_section())
                            return;

                        inherited_section_name = ParseAllSpaces(++ptr);
                    }
                    else if (std::isspace(*ptr))
                    {
                        // Replace space characters with nulls to ensure that the string ends at the first non-space
                        // character
                        *ptr = '\0';
                        ++ptr;
                    }
                    else if (*ptr == ';' || (*ptr == '/' && (ptr[1] == '/')))
                    {
                        // Start of a comment so everything after this can be ignored
                        *ptr = '\0';
                        break;
                    }
                    else
                    {
                        // Section names need to be converted to lowercase
                        *ptr = std::tolower(*ptr);
                        ++ptr;
                    }
                }

                // Add the trailing inherited section
                if (!add_inherited_section())
                    return;

                // Reserve enough space for all inherited sections
                Current->Data.reserve(total_count);

                // Copy inherited sections to the new section
                for (u32 i = 0; i < count; ++i)
                {
                    const Sect& inherited_section = unchecked_read_section(inherited_sections[i]);
                    for (auto it = inherited_section.Data.begin(); it != inherited_section.Data.end(); ++it)
                        insert_item(Current, *it);
                }
            }
        }
        else // name = value
        {
            if (!Current)
                continue;

            pstr name = ptr;
            pstr value = nullptr;
            pstr last_non_space = ptr;

            // Parse name
            while (*ptr != '\0')
            {
                if (*ptr == '=')
                    break;
                else if (*ptr == ';' || (*ptr == '/' && ptr[1] == '/'))
                {
                    // Start of a comment so everything after this can be ignored
                    *ptr = '\0';
                    break;
                }
                else
                {
                    if (!std::isspace(*ptr))
                        last_non_space = ptr;

                    ++ptr;
                }
            }

            // Ensure name is correctly null-terminated with trailing spaces
            if (std::isspace(last_non_space[1]))
                last_non_space[1] = '\0';

            // Check for empty name
            if (*name == '\0')
            {
                xr_delete(Current);
                Msg("! Ini File[%s]: Bad ini item found. Missing name: '%s'", path, str);
                return;
            }

            // Parse value
            if (*ptr == '=')
            {
                // Replace equal sign with a zero to create a null terminated string in name
                *ptr = '\0';
                ++ptr;

                ptr = ParseAllSpaces(ptr);

                // Parse actual value as string
                value = ptr;
                pstr value_write_ptr = ptr;
                bool inside_quoted_value = false;
                while (*ptr != '\0')
                {
                    if (inside_quoted_value)
                    {
                        if (*ptr == '"')
                            inside_quoted_value = false;

                        *value_write_ptr = *ptr;

                        ++ptr;
                        ++value_write_ptr;
                    }
                    else
                    {
                        // In non quoted strings comments terminate our value
                        if (*ptr == ';' || (*ptr == '/' && ptr[1] == '/'))
                        {
                            *value_write_ptr = '\0';
                            break;
                        }
                        else if (*ptr == '"')
                        {
                            inside_quoted_value = true;

                            *value_write_ptr = *ptr;

                            ++ptr;
                            ++value_write_ptr;
                        }
                        else
                        {
                            if (!std::isspace(*ptr))
                            {
                                *value_write_ptr = *ptr;
                                ++value_write_ptr;
                            }

                            ++ptr;
                        }
                    }
                }

                // Ensure the value is correctly null terminated
                if (value_write_ptr < ptr)
                    *value_write_ptr = '\0';


                // Handle multiline strings
                if (!ltx_multiline_values_enabled && inside_quoted_value)
                {
                    // There is no closing quote and multiline values are disabled so for compatibility we need append a closing quote
                    if (ptr >= str + sizeof(str))
                    {
                        // Not enough space to fit the closing quote
                        xr_delete(Current);
                        Msg("! Ini File[%s]: parsing failed. Line is too long", path);
                        return;
                    }

                    // Append closing quote
                    ptr[0] = '"';
                    ptr[1] = '\0';

                    Msg("~ Ini File[%s]: Multiline values are not supported assuming single line with missing closing quote. '%s'", path, value);
                }
                else
                {
                    // Normal handling of multiline values
                    while (inside_quoted_value)
                    {
                        if (reader->eof())
                        {
                            xr_delete(Current);
                            Msg("! Ini File[%s]: parsing failed. Unterminated multiline value", path);
                            return;
                        }

                        // Check if theres enough space for '\r\n'
                        if (ptr + 2 > str + sizeof(str))
                        {
                            xr_delete(Current);
                            Msg("! Ini File[%s]: parsing failed. Multiline string is too long", path);
                            return;
                        }

                        // Append '\r\n' at the end
                        ptr[0] = '\r';
                        ptr[1] = '\n';
                        ptr += 2;

                        const size_t size_left = sizeof(str) - (ptr - str);
                        if (!reader->try_r_string(ptr, size_left))
                        {
                            xr_delete(Current);
                            Msg("! Ini File[%s]: parsing failed. Multiline string is too long", path);
                            return;
                        }

                        // Check if this line contains the closing quote
                        ptr = ParseUntil(ptr, '"');
                        if (*ptr == '"')
                        {
                            // Put null terminator after the closing quote
                            ptr[1] = '\0';
                            inside_quoted_value = false;
                        }
                    }
                }
            }

            // Copy parsed name and value into item
            Item item;
            item.name = name;
            item.value = (value && *value) ? value : nullptr;

            if (m_flags.test(eReadOnly))
                insert_item(Current, item);
            else if (*item.value)
                insert_item(Current, item);
        }
    }

    if (Current)
        insert_new_section(Current);
}

bool CInifile::insert_new_section(Sect* section)
{
    VERIFY(section);

    auto iterator = std::lower_bound(DATA.begin(), DATA.end(), *section->Name, sect_pred);
    if (iterator != DATA.end() && (*iterator)->Name == section->Name)
    {
        Msg("! Ini File[%s]: Duplicate section '%s' found.", m_file_name, *section->Name);
        xr_delete(section);
        return false;
    }

    DATA.insert(iterator, section);

    return true;
}

CInifile::Sect& CInifile::unchecked_read_section(pcstr section)
{
    VERIFY(section);

    auto iterator = std::lower_bound(DATA.cbegin(), DATA.cend(), section, sect_pred);
    VERIFY3(iterator != DATA.cend() && (*iterator)->Name == section, "Section not found", section);

    return **iterator;
}

const CInifile::Sect& CInifile::unchecked_read_section(pcstr section) const
{
    return const_cast<CInifile*>(this)->unchecked_read_section(section);
}

void CInifile::save_as(IWriter& writer, bool bcheck) const
{
    string4096 temp;
    string4096 val;
    for (auto r_it = DATA.begin(); r_it != DATA.end(); ++r_it)
    {
        xr_sprintf(temp, sizeof(temp), "[%s]", (*r_it)->Name.c_str());
        writer.w_string(temp);
        if (bcheck)
        {
            xr_sprintf(temp, sizeof(temp), "; %d %d %d", (*r_it)->Name._get()->dwCRC, (*r_it)->Name._get()->dwReference,
                (*r_it)->Name._get()->dwLength);
            writer.w_string(temp);
        }

        for (auto s_it = (*r_it)->Data.begin(); s_it != (*r_it)->Data.end(); ++s_it)
        {
            const Item& I = *s_it;
            VERIFY(*I.name);

            if (*I.value)
            {
                _decorate(val, *I.value);
                // only name and value
                xr_sprintf(temp, sizeof(temp), "%-32s = %-32s", " ", I.name.c_str(), val);
            }
            else
            {
                // only name
                xr_sprintf(temp, sizeof(temp), "%-32s", " ", I.name.c_str());
            }

            _TrimRight(temp);

            if (temp[0])
                writer.w_string(temp);
        }

        writer.w_string("");
    }
}

bool CInifile::save_as(pcstr new_fname)
{
    // save if needed
    if (new_fname && new_fname[0])
        xr_strcpy(m_file_name, sizeof(m_file_name), new_fname);

    R_ASSERT(m_file_name[0]);
    convert_path_separators(m_file_name);
    IWriter* F = FS.w_open_ex(m_file_name);
    if (!F)
    {
        Msg("! Ini File[%s]: Can't open file for saving '%s'", m_file_name, new_fname);
        return false;
    }

    save_as(*F);
    FS.w_close(F);

    return true;
}

void CInifile::set_override_names(bool value) noexcept { m_flags.set(eOverrideNames, value); }

void CInifile::save_at_end(bool value) noexcept { m_flags.set(eSaveAtEnd, value); }

void CInifile::set_readonly(bool value) /*noexcept*/ { m_flags.set(eReadOnly, value); }

bool CInifile::section_exist(pcstr section_name) const
{
    VERIFY(section_name);

    auto I = std::lower_bound(DATA.begin(), DATA.end(), section_name, sect_pred);
    return I != DATA.end() && xr_strcmp(*(*I)->Name, section_name) == 0;
}

pcstr CInifile::fname() const /*noexcept*/ { return m_file_name; };

bool CInifile::line_exist(pcstr section_name, pcstr line_name) const
{
    VERIFY(section_name);
    VERIFY(line_name);

    if (!section_exist(section_name))
        return false;

    const Sect& section = r_section(section_name);

    auto iterator = std::lower_bound(section.Data.begin(), section.Data.end(), line_name, item_pred);
    return iterator != section.Data.end() && xr_strcmp(*iterator->name, line_name) == 0;
}

u32 CInifile::line_count(pcstr section_name) const
{
    VERIFY(section_name);

    const Sect& section = r_section(section_name);
    return section.Data.size();
}

u32 CInifile::section_count() const { return DATA.size(); }

CInifile::Root& CInifile::sections() { return DATA; }

const CInifile::Root& CInifile::sections() const { return DATA; }

//--------------------------------------------------------------------------------------

CInifile::Sect& CInifile::r_section(const shared_str& section_name) { return r_section(*section_name); }

const CInifile::Sect& CInifile::r_section(const shared_str& section_name) const { return r_section(*section_name); }

bool CInifile::line_exist(const shared_str& section_name, const shared_str& line_name) const
{
    return line_exist(*section_name, *line_name);
}

u32 CInifile::line_count(const shared_str& section_name) const { return line_count(*section_name); }

bool CInifile::section_exist(const shared_str& section_name) const { return section_exist(*section_name); }

//--------------------------------------------------------------------------------------
// Read functions
//--------------------------------------------------------------------------------------

CInifile::Sect& CInifile::r_section(pcstr section_name)
{
    VERIFY(section_name);

    string256 section_lower;
    StringCopyLowercase(section_lower, section_name, sizeof(section_lower));

    auto I = std::lower_bound(DATA.cbegin(), DATA.cend(), section_lower, sect_pred);
    if (I == DATA.cend())
        xrDebug::Fatal(DEBUG_INFO, "Ini File[%s]: Can't find section '%s'.", m_file_name, section_name);
    else if (xr_strcmp(*(*I)->Name, section_lower))
    {
        // g_pStringContainer->verify();

        // string_path ini_dump_fn, path;
        // strconcat (sizeof(ini_dump_fn), ini_dump_fn, Core.ApplicationName, "_", Core.UserName, ".ini_log");
        //
        // FS.update_path (path, "$logs$", ini_dump_fn);
        // IWriter* F = FS.w_open_ex(path);
        // save_as (*F);
        // F->w_string ("shared strings:");
        // g_pStringContainer->dump(F);
        // FS.w_close (F);
        xrDebug::Fatal(DEBUG_INFO,
            "Can't open section '%s' (only '%s' avail). Please attach [*.ini_log] file to your bug report",
            section_lower, *(*I)->Name);
    }
    return **I;
}

const CInifile::Sect& CInifile::r_section(pcstr section_name) const
{
    return const_cast<CInifile*>(this)->r_section(section_name);
}

pcstr CInifile::r_string(pcstr section_name, pcstr line_name) const
{
    VERIFY(section_name);
    VERIFY(line_name);

    const Sect& section = r_section(section_name);
    auto iterator = std::lower_bound(section.Data.cbegin(), section.Data.cend(), line_name, item_pred);

    if (iterator != section.Data.cend() && xr_strcmp(*iterator->name, line_name) == 0)
        return *iterator->value;

    xrDebug::Fatal(DEBUG_INFO, "Can't find variable '%s' in [%s]", line_name, section_name);
    return nullptr;
}

pcstr CInifile::r_string(const shared_str& section_name, pcstr line_name) const
{
    return r_string(*section_name, line_name);
}

shared_str CInifile::r_string_wb(pcstr section_name, pcstr line_name) const
{
    pcstr base = r_string(section_name, line_name);

    if (base == nullptr)
        return shared_str(nullptr);

    string4096 original;
    xr_strcpy(original, sizeof(original), base);

    const u32 length = xr_strlen(original);
    if (length == 0)
        return shared_str("");

    if (original[length - 1] == '"')
        original[length - 1] = '\0'; // skip end

    if (original[0] == '"')
        return shared_str(&original[0] + 1); // skip begin

    return shared_str(original);
}

shared_str CInifile::r_string_wb(const shared_str& section_name, pcstr line_name) const
{
    return r_string_wb(*section_name, line_name);
}

u8 CInifile::r_u8(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return u8(atoi(string));
}

u8 CInifile::r_u8(const shared_str& section_name, pcstr line_name) const { return r_u8(*section_name, line_name); }

u16 CInifile::r_u16(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return u16(atoi(string));
}

u16 CInifile::r_u16(const shared_str& section_name, pcstr line_name) const { return r_u16(*section_name, line_name); }

u32 CInifile::r_u32(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return u32(atoi(string));
}

u32 CInifile::r_u32(const shared_str& section_name, pcstr line_name) const { return r_u32(*section_name, line_name); }

u64 CInifile::r_u64(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
#ifndef _EDITOR
    return _strtoui64(string, nullptr, 10);
#else
    return (u64)_atoi64(string);
#endif
}

u64 CInifile::r_u64(const shared_str& section_name, pcstr line_name) const { return r_u64(*section_name, line_name); }

s8 CInifile::r_s8(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return s8(atoi(string));
}

s8 CInifile::r_s8(const shared_str& section_name, pcstr line_name) const { return r_s8(*section_name, line_name); }

s16 CInifile::r_s16(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return s16(atoi(string));
}

s16 CInifile::r_s16(const shared_str& section_name, pcstr line_name) const { return r_s16(*section_name, line_name); }

s32 CInifile::r_s32(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return s32(atoi(string));
}

s32 CInifile::r_s32(const shared_str& section_name, pcstr line_name) const { return r_s32(*section_name, line_name); }

s64 CInifile::r_s64(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return _atoi64(string);
}

s64 CInifile::r_s64(const shared_str& section_name, pcstr line_name) const { return r_s64(*section_name, line_name); }

float CInifile::r_float(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return float(atof(string));
}

float CInifile::r_float(const shared_str& section_name, pcstr line_name) const
{
    return r_float(*section_name, line_name);
}

Fcolor CInifile::r_fcolor(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    Fcolor V = {0, 0, 0, 0};
    sscanf(string, "%f,%f,%f,%f", &V.r, &V.g, &V.b, &V.a);
    return V;
}

Fcolor CInifile::r_fcolor(const shared_str& section_name, pcstr line_name) const
{
    return r_fcolor(*section_name, line_name);
}

u32 CInifile::r_color(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    u32 r = 0;
    u32 g = 0;
    u32 b = 0;
    u32 a = 255;
    sscanf(string, "%u,%u,%u,%u", &r, &g, &b, &a);
    return color_rgba(r, g, b, a);
}

u32 CInifile::r_color(const shared_str& section_name, pcstr line_name) const
{
    return r_color(*section_name, line_name);
}

Ivector2 CInifile::r_ivector2(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    Ivector2 vector = {0, 0};
    sscanf(string, "%d,%d", &vector.x, &vector.y);
    return vector;
}

Ivector2 CInifile::r_ivector2(const shared_str& section_name, pcstr line_name) const
{
    return r_ivector2(*section_name, line_name);
}

Ivector3 CInifile::r_ivector3(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    Ivector vector = {0, 0, 0};
    sscanf(string, "%d,%d,%d", &vector.x, &vector.y, &vector.z);
    return vector;
}

Ivector3 CInifile::r_ivector3(const shared_str& section_name, pcstr line_name) const
{
    return r_ivector3(*section_name, line_name);
}

Ivector4 CInifile::r_ivector4(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    Ivector4 vector = {0, 0, 0, 0};
    sscanf(string, "%d,%d,%d,%d", &vector.x, &vector.y, &vector.z, &vector.w);
    return vector;
}

Ivector4 CInifile::r_ivector4(const shared_str& section_name, pcstr line_name) const
{
    return r_ivector4(*section_name, line_name);
}

Fvector2 CInifile::r_fvector2(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    Fvector2 vector = {0.f, 0.f};
    sscanf(string, "%f,%f", &vector.x, &vector.y);
    return vector;
}

Fvector2 CInifile::r_fvector2(const shared_str& section_name, pcstr line_name) const
{
    return r_fvector2(*section_name, line_name);
}

Fvector3 CInifile::r_fvector3(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    Fvector3 vector = {0.f, 0.f, 0.f};
    sscanf(string, "%f,%f,%f", &vector.x, &vector.y, &vector.z);
    return vector;
}

Fvector3 CInifile::r_fvector3(const shared_str& section_name, pcstr line_name) const
{
    return r_fvector3(*section_name, line_name);
}

Fvector4 CInifile::r_fvector4(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    Fvector4 vector = {0.f, 0.f, 0.f, 0.f};
    sscanf(string, "%f,%f,%f,%f", &vector.x, &vector.y, &vector.z, &vector.w);
    return vector;
}

Fvector4 CInifile::r_fvector4(const shared_str& section_name, pcstr line_name) const
{
    return r_fvector4(*section_name, line_name);
}

bool CInifile::r_bool(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    VERIFY2(string && xr_strlen(string) <= 5,
        make_string("! Ini File[%s]: \"%s\" is not a valid bool value, section[%s], line[%s]", m_file_name, string, section_name, line_name));

    char buffer[8];
    StringCopyLowercase(buffer, string, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = 0;

    return isBool(buffer);
}

bool CInifile::r_bool(const shared_str& section_name, pcstr line_name) const
{
    return r_bool(*section_name, line_name);
}

CLASS_ID CInifile::r_clsid(pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return TEXT2CLSID(string);
}

CLASS_ID CInifile::r_clsid(const shared_str& section_name, pcstr line_name) const
{
    return r_clsid(*section_name, line_name);
}

int CInifile::r_token(pcstr section_name, pcstr line_name, const xr_token* token_list) const
{
    pcstr string = r_string(section_name, line_name);
    for (size_t i = 0; token_list[i].name; ++i)
        if (!xr_stricmp(string, token_list[i].name))
            return token_list[i].id;

    return 0;
}

bool CInifile::r_line(pcstr section_name, int line_number, pcstr* name_out, pcstr* value_out) const
{
    const Sect& section = r_section(section_name);
    if (line_number >= (int)section.Data.size() || line_number < 0)
        return false;

    for (auto I = section.Data.cbegin(); I != section.Data.cend(); ++I)
        if (!line_number--)
        {
            *name_out = *I->name;
            *value_out = *I->value;
            return true;
        }

    return false;
}

bool CInifile::r_line(const shared_str& section_name, int line_number, pcstr* name_out, pcstr* value_out) const
{
    return r_line(*section_name, line_number, name_out, value_out);
}

//--------------------------------------------------------------------------------------
// Write functions
//--------------------------------------------------------------------------------------

void CInifile::w_string(pcstr section_name, pcstr line_name, pcstr value, pcstr comment)
{
    R_ASSERT(!m_flags.test(eReadOnly));

    // section
    string256 section;
    _parse(section, section_name);
    xr_strlwr(section);

    if (!section_exist(section))
    {
        // create _new_ section
        Sect* NEW = xr_new<Sect>();
        NEW->Name = section;
        auto I = std::lower_bound(DATA.begin(), DATA.end(), section, sect_pred);
        DATA.emplace(I, std::move(NEW));
    }

    // parse line/value
    string4096 line;
    _parse(line, line_name);
    string4096 value_parsed;
    _parse(value_parsed, value);

    // duplicate & insert
    Item item;
    Sect& data = r_section(section);
    item.name = line[0] ? line : 0;
    item.value = value_parsed[0] ? value_parsed : 0;

    auto it = std::lower_bound(data.Data.begin(), data.Data.end(), *item.name, item_pred);

    if (it != data.Data.end())
    {
        // Check for "name" matching
        if (xr_strcmp(*it->name, *item.name) == 0)
        {
            R_ASSERT2(m_flags.test(eOverrideNames),
                make_string("! Ini File[%s]: name[%s] already exist in section[%s]", m_file_name, line, section).c_str());
            *it = item;
        }
        else
            data.Data.emplace(it, std::move(item));
    }
    else
        data.Data.emplace(it, std::move(item));
}
void CInifile::w_u8(pcstr section_name, pcstr line_name, u8 value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d", value);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_u16(pcstr section_name, pcstr line_name, u16 value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d", value);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_u32(pcstr section_name, pcstr line_name, u32 value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d", value);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_u64(pcstr section_name, pcstr line_name, u64 value, pcstr comment)
{
    string128 temp;
#ifndef _EDITOR
    _ui64toa_s(value, temp, sizeof(temp), 10);
#else
    _ui64toa(V, temp, 10);
#endif
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_s8(pcstr section_name, pcstr line_name, s8 value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d", value);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_s16(pcstr section_name, pcstr line_name, s16 value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d", value);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_s32(pcstr section_name, pcstr line_name, s32 value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d", value);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_s64(pcstr section_name, pcstr line_name, s64 value, pcstr comment)
{
    string128 temp;
#ifndef _EDITOR
    _i64toa_s(value, temp, sizeof(temp), 10);
#else
    _i64toa(V, temp, 10);
#endif
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_float(pcstr section_name, pcstr line_name, float value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%f", value);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_fcolor(pcstr section_name, pcstr line_name, const Fcolor& value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%f,%f,%f,%f", value.r, value.g, value.b, value.a);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_color(pcstr section_name, pcstr line_name, u32 value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d,%d,%d,%d", color_get_R(value), color_get_G(value), color_get_B(value),
        color_get_A(value));
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_ivector2(pcstr section_name, pcstr line_name, const Ivector2& value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d,%d", value.x, value.y);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_ivector3(pcstr section_name, pcstr line_name, const Ivector3& value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d,%d,%d", value.x, value.y, value.z);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_ivector4(pcstr section_name, pcstr line_name, const Ivector4& value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%d,%d,%d,%d", value.x, value.y, value.z, value.w);
    w_string(section_name, line_name, temp, comment);
}
void CInifile::w_fvector2(pcstr section_name, pcstr line_name, const Fvector2& value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%f,%f", value.x, value.y);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_fvector3(pcstr section_name, pcstr line_name, const Fvector3& value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%f,%f,%f", value.x, value.y, value.z);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_fvector4(pcstr section_name, pcstr line_name, const Fvector4& value, pcstr comment)
{
    string128 temp;
    xr_sprintf(temp, sizeof(temp), "%f,%f,%f,%f", value.x, value.y, value.z, value.w);
    w_string(section_name, line_name, temp, comment);
}

void CInifile::w_bool(pcstr section_name, pcstr line_name, bool value, pcstr comment)
{
    w_string(section_name, line_name, value ? "on" : "off", comment);
}

void CInifile::remove_line(pcstr section_name, pcstr line_name)
{
    R_ASSERT(!m_flags.test(eReadOnly));

    if (line_exist(section_name, line_name))
    {
        Sect& data = r_section(section_name);

        auto iterator = std::lower_bound(data.Data.begin(), data.Data.end(), line_name, item_pred);
        R_ASSERT(iterator != data.Data.end() && xr_strcmp(*iterator->name, line_name) == 0);
        data.Data.erase(iterator);
    }
}

template<>
XRCORE_API pcstr CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_string(section_name, line_name);
}

template<>
XRCORE_API u8 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_u8(section_name, line_name);
}

template<>
XRCORE_API u16 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_u16(section_name, line_name);
}

template<>
XRCORE_API u32 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_u32(section_name, line_name);
}

template<>
XRCORE_API u64 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_u64(section_name, line_name);
}

template<>
XRCORE_API s8 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_s8(section_name, line_name);
}

template<>
XRCORE_API s16 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_s16(section_name, line_name);
}

template<>
XRCORE_API s32 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_s32(section_name, line_name);
}

template<>
XRCORE_API s64 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_s64(section_name, line_name);
}

template<>
XRCORE_API float CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_float(section_name, line_name);
}

template<>
XRCORE_API Fcolor CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_fcolor(section_name, line_name);
}

template<>
XRCORE_API Ivector2 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_ivector2(section_name, line_name);
}

template<>
XRCORE_API Ivector3 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_ivector3(section_name, line_name);
}

template<>
XRCORE_API Ivector4 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_ivector4(section_name, line_name);
}

template<>
XRCORE_API bool CInifile::try_read(Ivector4& out_value, pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return sscanf(string, "%d,%d,%d,%d", &out_value.x, &out_value.y, &out_value.z, &out_value.w) == 4;
}

template<>
XRCORE_API Fvector2 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_fvector2(section_name, line_name);
}

template<>
XRCORE_API bool CInifile::try_read(Fvector2& out_value, pcstr section_name, pcstr line_name) const
{
    pcstr string = r_string(section_name, line_name);
    return sscanf(string, "%f,%f", &out_value.x, &out_value.y) == 2;
}

template<>
XRCORE_API Fvector3 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_fvector3(section_name, line_name);
}

template<>
XRCORE_API Fvector4 CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_fvector4(section_name, line_name);
}

template<>
XRCORE_API bool CInifile::read(pcstr section_name, pcstr line_name) const
{
    return r_bool(section_name, line_name);
}

XRCORE_API CInifile::Sect* CInifile::try_read_section(pcstr section_name)
{
    auto I = std::lower_bound(DATA.cbegin(), DATA.cend(), section_name, sect_pred);

    if (I != DATA.cend() && xr_strcmp(*(*I)->Name, section_name) == 0)
        return *I;
    else
        return nullptr;
}

XRCORE_API const CInifile::Sect* CInifile::try_read_section(pcstr section_name) const
{
    return const_cast<CInifile*>(this)->try_read_section(section_name);
}
