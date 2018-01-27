#pragma once

// TODO: tamlin: Get rid of _std_extensions.h as compile-time dependency, if possible.
#include "_types.h"
#include "_std_extensions.h"

#pragma pack(push, 4)
#pragma warning(push)
#pragma warning(disable : 4200)
struct XRCORE_API str_value
{
    u32 dwReference;
    u32 dwLength;
    u32 dwCRC;
    str_value* next;
    char value[];
};

struct XRCORE_API str_value_cmp
{
    // less
    IC bool operator()(const str_value* A, const str_value* B) const { return A->dwCRC < B->dwCRC; };
};

struct XRCORE_API str_hash_function
{
    IC u32 operator()(str_value const* const value) const { return value->dwCRC; };
};

#pragma warning(pop)

struct str_container_impl;
class IWriter;
//////////////////////////////////////////////////////////////////////////
class XRCORE_API str_container
{
public:
    str_container();
    ~str_container();

    str_value* dock(pcstr value);
    void clean();
    void dump();
    void dump(IWriter* W);
    void verify();
    u32 stat_economy();

private:
    str_container_impl* impl;
};
XRCORE_API extern str_container* g_pStringContainer;

//////////////////////////////////////////////////////////////////////////
class shared_str
{
    str_value* p_;

protected:
    // ref-counting
    void _dec()
    {
        if (0 == p_)
            return;
        p_->dwReference--;
        if (0 == p_->dwReference)
            p_ = 0;
    }

public:
    void _set(pcstr rhs)
    {
        str_value* v = g_pStringContainer->dock(rhs);
        if (0 != v)
            v->dwReference++;
        _dec();
        p_ = v;
    }
    void _set(shared_str const& rhs)
    {
        str_value* v = rhs.p_;
        if (0 != v)
            v->dwReference++;
        _dec();
        p_ = v;
    }
    // void _set (shared_str const &rhs) { str_value* v = g_pStringContainer->dock(rhs.c_str()); if (0!=v)
    // v->dwReference++; _dec(); p_ = v; }

    const str_value* _get() const { return p_; }
public:
    // construction
    shared_str() { p_ = 0; }
    shared_str(pcstr rhs)
    {
        p_ = 0;
        _set(rhs);
    }
    shared_str(shared_str const& rhs)
    {
        p_ = 0;
        _set(rhs);
    }
    ~shared_str() { _dec(); }
    // assignment & accessors
    shared_str& operator=(pcstr rhs)
    {
        _set(rhs);
        return (shared_str&)*this;
    }
    shared_str& operator=(shared_str const& rhs)
    {
        _set(rhs);
        return (shared_str&)*this;
    }
    // XXX tamlin: Remove operator*(). It may be convenient, but it's dangerous. Use 
    pcstr operator*() const { return p_ ? p_->value : 0; }
    bool operator!() const { return p_ == 0; }
    char operator[](size_t id) { return p_->value[id]; }
    pcstr c_str() const { return p_ ? p_->value : 0; }
    // misc func
    u32 size() const
    {
        if (0 == p_)
            return 0;
        else
            return p_->dwLength;
    }
    void swap(shared_str& rhs)
    {
        str_value* tmp = p_;
        p_ = rhs.p_;
        rhs.p_ = tmp;
    }
    bool equal(const shared_str& rhs) const { return (p_ == rhs.p_); }
    shared_str& __cdecl printf(const char* format, ...)
    {
        string4096 buf;
        va_list p;
        va_start(p, format);
        int vs_sz = vsnprintf(buf, sizeof(buf) - 1, format, p);
        buf[sizeof(buf) - 1] = 0;
        va_end(p);
        if (vs_sz)
            _set(buf);
        return (shared_str&)*this;
    }
};

// res_ptr == res_ptr
// res_ptr != res_ptr
// const res_ptr == ptr
// const res_ptr != ptr
// ptr == const res_ptr
// ptr != const res_ptr
// res_ptr < res_ptr
// res_ptr > res_ptr
IC bool operator==(shared_str const& a, shared_str const& b) { return a._get() == b._get(); }
IC bool operator!=(shared_str const& a, shared_str const& b) { return a._get() != b._get(); }
IC bool operator<(shared_str const& a, shared_str const& b) { return a._get() < b._get(); }
IC bool operator>(shared_str const& a, shared_str const& b) { return a._get() > b._get(); }
// externally visible standard functionality
IC void swap(shared_str& lhs, shared_str& rhs) { lhs.swap(rhs); }
IC u32 xr_strlen(const shared_str& a) throw() { return a.size(); }
IC int xr_strcmp(const shared_str& a, const char* b) throw() { return xr_strcmp(*a, b); }
IC int xr_strcmp(const char* a, const shared_str& b) throw() { return xr_strcmp(a, *b); }
IC int xr_strcmp(const shared_str& a, const shared_str& b) throw()
{
    if (a.equal(b))
        return 0;
    else
        return xr_strcmp(*a, *b);
}

IC void xr_strlwr(shared_str& src)
{
    if (*src)
    {
        char* lp = xr_strdup(*src);
        xr_strlwr(lp);
        src = lp;
        xr_free(lp);
    }
}

#pragma pack(pop)
