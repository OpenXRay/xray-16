#pragma once
#include "_flags.h"

#pragma pack(push, 1)
struct XRCORE_API xr_shortcut
{
    enum
    {
        flShift = 0x20,
        flCtrl = 0x40,
        flAlt = 0x80,
    };

    union
    {
        struct
        {
            u8 key;
            Flags8 ext;
        } ext_key;
        u16 hotkey;
    };

    xr_shortcut(u8 k, bool a, bool c, bool s)
    {
        ext_key.key = k;
        ext_key.ext.assign(u8((a ? flAlt : 0) | (c ? flCtrl : 0) | (s ? flShift : 0)));
    }

    xr_shortcut()
    {
        ext_key.ext.zero();
        ext_key.key = 0;
    }

    bool similar(const xr_shortcut& v) const { return ext_key.ext.equal(v.ext_key.ext) && (ext_key.key == v.ext_key.key); }
};
#pragma pack(pop)
