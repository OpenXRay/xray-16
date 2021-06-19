#pragma once

#if defined(MASTER_GOLD)
#   define PIX_EVENT(Name) do { } while (false)
#else
#ifdef USE_OGL
#   define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(#Name)

class dxPixEventWrapper
{
public:
    dxPixEventWrapper(const char* name) { HW.BeginPixEvent(name); }
    ~dxPixEventWrapper() { HW.EndPixEvent(); }
};
#else
#   define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(L#Name)

class dxPixEventWrapper
{
public:
    dxPixEventWrapper(const wchar_t* wszName) { HW.BeginPixEvent(wszName); }
    ~dxPixEventWrapper() { HW.EndPixEvent(); }
};
#endif // USE_OGL
#endif // MASTER_GOLD
