#pragma once

namespace xray::render::fg
{
#if defined(MASTER_GOLD)
#   define PIX_EVENT(Name) do { } while (false)
#   define PIX_EVENT_CTX(C,Name) do { } while (false)
#elif defined(USE_DX11)
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
#else
#   define PIX_EVENT(Name) do { } while (false)
#   define PIX_EVENT_CTX(C,Name) do { } while (false)
#endif
} // namespace xray::render::fg
