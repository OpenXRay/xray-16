#pragma once

#include "xr_types.h"
#include "xrMemory.h"

#pragma pack(push, 4)

struct str_value;

struct XRCORE_API str_value_cmp
{
    // less
    IC bool operator()(const str_value* A, const str_value* B) const;
};

struct str_container_impl;
class IWriter;

class XRCORE_API str_container
{
public:
    str_container();
    ~str_container();
    u32 dock(pcstr value) const;
    str_value* get_string(u32 index) const;
    void clean() const;
    void dump() const;
    void dump(IWriter* W) const;
    void verify() const;

    [[nodiscard]]
    std::pair<size_t, size_t> stat_economy() const;

private:
    str_container_impl* impl;
};

XRCORE_API extern str_container* g_pStringContainer;

#pragma pack(pop)
