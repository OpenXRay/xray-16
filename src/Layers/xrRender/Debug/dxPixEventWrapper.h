#pragma once

#if defined(MASTER_GOLD)
#   define PIX_EVENT(Name) do { } while (false)
#else
#   define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(L#Name)

class dxPixEventWrapper
{
public:
    dxPixEventWrapper(LPCWSTR wszName) { HW.BeginPixEvent(wszName); }
    ~dxPixEventWrapper() { HW.EndPixEvent(); }
};
#endif // MASTER_GOLD

