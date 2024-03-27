#pragma once

#if defined(MASTER_GOLD)
#   define PIX_EVENT(Name) do { } while (false)
#   define PIX_EVENT_CTX(C,Name) do { } while (false)
#else
#if defined(USE_DX9) || defined(USE_DX11) || defined(USE_DX12)
#   define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(RCache,L ## #Name)
#   define PIX_EVENT_CTX(C,Name) dxPixEventWrapper pixEvent##Name(C,L ## #Name)

class dxPixEventWrapper
{
    CBackend& cmd_list;
public:
    dxPixEventWrapper(CBackend& cmd_list_in, const wchar_t* wszName)
    : cmd_list(cmd_list_in)
    {
        cmd_list.gpu_mark_begin(wszName);
    }
    ~dxPixEventWrapper() { cmd_list.gpu_mark_end(); }
};
#elif defined(USE_OGL)
#   define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(#Name)
#   define PIX_EVENT_CTX(C,Name) dxPixEventWrapper pixEvent##Name(#Name)

class dxPixEventWrapper
{
public:
    dxPixEventWrapper(const char* name) { HW.BeginPixEvent(name); }
    ~dxPixEventWrapper() { HW.EndPixEvent(); }
};
#else
#   error No graphics API selected or enabled!
#endif // USE_OGL
#endif // MASTER_GOLD
