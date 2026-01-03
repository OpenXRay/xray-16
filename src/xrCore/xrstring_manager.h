#pragma once

#include "xr_types.h"
#include "xrMemory.h"

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
#pragma warning(pop)

struct str_container_impl;
class IWriter;

class XRCORE_API str_container
{
public:
    str_container();
    ~str_container();

    str_value* dock(pcstr value) const;
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
