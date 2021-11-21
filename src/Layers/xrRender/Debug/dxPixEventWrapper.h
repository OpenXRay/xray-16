#pragma once

#if defined(MASTER_GOLD)
#   define PIX_EVENT(Name) do { } while (false)
#else
#if defined(USE_DX9) || defined(USE_DX11)
#   define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(L#Name)

class dxPixEventWrapper
{
public:
    dxPixEventWrapper(const wchar_t* wszName) { HW.BeginPixEvent(wszName); }
    ~dxPixEventWrapper() { HW.EndPixEvent(); }
};
#elif defined(USE_OGL)
#   define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(#Name)

class dxPixEventWrapper
{
public:
    dxPixEventWrapper(const char* name) { HW.BeginPixEvent(name); }
    ~dxPixEventWrapper() { HW.EndPixEvent(); }
};
#else
#    error No graphics API selected or enabled!
#endif // USE_OGL
#endif // MASTER_GOLD
