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
#ifdef USE_TBB_PARALLEL
#define FOR_START(type, start, finish, counter)\
tbb::parallel_for(tbb::blocked_range<type>(start, finish), [&](const tbb::blocked_range<type>& range) {\
    for (type counter = range.begin(); counter != range.end(); ++counter)
        
#define FOR_END });
#define ACCELERATED_SORT tbb::parallel_sort
#else
#define FOR_START(type, start, finish, counter)\
    for (type counter = start; counter < finish; counter++)
#define FOR_END
#define ACCELERATED_SORT std::sort
#endif


