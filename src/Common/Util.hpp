#pragma once

#define IMPLEMENT_ENUM_FLAG_OPERATORS(TEnum, TBaseType)\
    inline TEnum operator|(TEnum a, TEnum b) { return (TEnum)((TBaseType)a | (TBaseType)b); }\
    inline TEnum operator&(TEnum a, TEnum b) { return (TEnum)((TBaseType)a & (TBaseType)b); }\
    inline TEnum& operator|=(TEnum& a, TEnum b) { return a = a | b; }\
    inline TEnum& operator&=(TEnum& a, TEnum b) { return a = a & b; }
#define _RELEASE(x)\
    {\
        if ((x))\
        {\
            (x)->Release();\
            (x) = nullptr;\
        }\
    }
#define _SHOW_REF(msg, x)\
    {\
        if ((x))\
        {\
            (x)->AddRef();\
            Log(msg, u32((x)->Release()));\
        }\
    }
