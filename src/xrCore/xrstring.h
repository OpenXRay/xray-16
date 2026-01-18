#pragma once

#include <cstdio>
#include "xr_types.h"
#include "xrMemory.h"
#include <cstring>

struct str_value;

class XRCORE_API shared_str
{
    u32 index_{};

protected:
    void _dec() noexcept;

public:
    void _set(pcstr rhs);
    void _set(shared_str const& rhs) noexcept;
    void _set(std::nullptr_t) noexcept;

    [[nodiscard]]
    const str_value* _get() const;

    friend bool operator==(shared_str const& a, shared_str const& b);
    friend bool operator!=(shared_str const& a, shared_str const& b);
    friend bool operator<(shared_str const& a, shared_str const& b);
    friend bool operator>(shared_str const& a, shared_str const& b);

public:
    // construction
    shared_str() = default;
    shared_str(pcstr rhs);
    shared_str(shared_str const& rhs) noexcept;
    shared_str(shared_str&& rhs) noexcept : index_(rhs.index_)
    {
        rhs.index_ = 0;
    }
    ~shared_str();
    // assignment & accessors
    shared_str& operator=(pcstr rhs);
    shared_str& operator=(shared_str const& rhs) noexcept;
    shared_str& operator=(shared_str&& rhs) noexcept
    {
        index_ = rhs.index_;
        rhs.index_ = 0;
        return *this;
    }
    shared_str& operator=(std::nullptr_t) noexcept;

    [[nodiscard]]
    bool operator!() const
    {
        return index_ == 0;
    }
    [[nodiscard]]
    explicit operator bool() const
    {
        return index_ != 0;
    }
    [[nodiscard]]
    char operator[](size_t id);
    [[nodiscard]]
    char operator[](size_t id) const;
    [[nodiscard]]
    pcstr c_str() const;
    [[nodiscard]]
    size_t size() const;

    [[nodiscard]]
    bool empty() const
    {
        return size() == 0;
    }

    void swap(shared_str& rhs) noexcept
    {
        u32 tmp = index_;
        index_ = rhs.index_;
        rhs.index_ = tmp;
    }

    [[nodiscard]]
    bool equal(const shared_str& rhs) const;

    [[nodiscard]]
    u32 get_crc() const;
};

inline int __cdecl xr_sprintf(shared_str& destination, pcstr format_string, ...)
{
    string4096 buf;
    va_list args;
    va_start(args, format_string);
    const int vs_sz = vsnprintf(buf, sizeof(buf) - 1, format_string, args);
    buf[sizeof(buf) - 1] = 0;
    va_end(args);
    if (vs_sz >= 0)
        destination = buf;
    return vs_sz;
}

template<>
struct std::hash<shared_str>
{
    [[nodiscard]] size_t operator()(const shared_str& str) const noexcept
    {
        return str ? str.get_crc() : std::hash<pcstr>{}(nullptr);
    }
};

bool operator==(const shared_str&, std::nullptr_t) = delete;
bool operator!=(const shared_str&, std::nullptr_t) = delete;

bool operator==(std::nullptr_t, const shared_str&) = delete;
bool operator!=(std::nullptr_t, const shared_str&) = delete;

// res_ptr == res_ptr
// res_ptr != res_ptr
// const res_ptr == ptr
// const res_ptr != ptr
// ptr == const res_ptr
// ptr != const res_ptr
// res_ptr < res_ptr
// res_ptr > res_ptr
IC bool operator==(shared_str const& a, shared_str const& b)
{
    return a.index_ == b.index_;
}

IC bool operator!=(shared_str const& a, shared_str const& b)
{
    return a.index_ != b.index_;
}

IC bool operator<(shared_str const& a, shared_str const& b)
{
    return a.index_ < b.index_;
}

IC bool operator>(shared_str const& a, shared_str const& b)
{
    return a.index_ > b.index_;
}

// externally visible standard functionality
IC void swap(shared_str& lhs, shared_str& rhs) noexcept { lhs.swap(rhs); }
IC size_t xr_strlen(const shared_str& a) noexcept { return a.size(); }

ICF int xr_strcmp(const char* S1, const char* S2)
{
    return strcmp(S1, S2);
}

IC int xr_strcmp(const shared_str& a, const char* b) noexcept { return xr_strcmp(a.c_str(), b); }
IC int xr_strcmp(const char* a, const shared_str& b) noexcept { return xr_strcmp(a, b.c_str()); }
IC int xr_strcmp(const shared_str& a, const shared_str& b) noexcept
{
    if (a.equal(b))
        return 0;
    else
        return xr_strcmp(a.c_str(), b.c_str());
}

IC char* xr_strlwr(char* src)
{
    size_t i = 0;
    while (src[i])
    {
        src[i] = (char)tolower(src[i]);// TODO rewrite locale-independent toupper_l()
        i++;
    }
    return src;
}

IC void xr_strlwr(shared_str& src)
{
    if (src.c_str())
    {
        char* lp = xr_strdup(src.c_str());
        xr_strlwr(lp);
        src = lp;
        xr_free(lp);
    }
}
