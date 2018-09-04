#include "pch.hpp"

#pragma managed(push, off)
#include "Include/editor/engine.hpp"
#include "ide_impl.hpp"
#pragma managed(pop)

#include "window_ide.h"

#pragma comment(lib, "user32.lib")

namespace XRay
{
namespace Editor
{
private ref class window_ide_final : public editor::window_ide
{
public:
    window_ide_final(ide_base*& ide, engine_base* engine) : editor::window_ide(engine)
    {
        m_ide = ide;
        System::Windows::Forms::Application::Idle += gcnew System::EventHandler(this, &window_ide_final::on_idle);
    }

    ~window_ide_final()
    {
        System::Windows::Forms::Application::Idle -= gcnew System::EventHandler(this, &window_ide_final::on_idle);
        m_engine = nullptr;
        m_ide = nullptr;
    }

protected:
    virtual void WndProc(System::Windows::Forms::Message % m) override
    {
        LONG_PTR result;
#ifdef XR_X64
        if (m_engine && m_engine->on_message((HWND)m.HWnd.ToInt64(), m.Msg, m.WParam.ToInt64(), m.LParam.ToInt64(), result))
            return;
#else
        if (m_engine && m_engine->on_message((HWND)m.HWnd.ToInt32(), m.Msg, m.WParam.ToInt32(), m.LParam.ToInt32(), result))
            return;
#endif

        editor::window_ide::WndProc(m);
    }

private:
    void on_idle(System::Object ^ sender, System::EventArgs ^ event_args)
    {
        ide_impl* impl = dynamic_cast<ide_impl*>(m_ide);
        impl->on_idle_start();

        MSG message;
        do
        {
            m_engine->on_idle();
            impl->on_idle();
        } while (m_engine && !m_engine->quit_requested() && !PeekMessage(&message, HWND(0), 0, 0, 0));

        impl->on_idle_end();
    }
};

ide_impl* g_ide = nullptr;

static void initialize_impl(ide_base*& ide, engine_base* engine)
{
    VERIFY(!g_ide);
    g_ide = new ide_impl(engine);
    ide = g_ide;
    g_ide->window(gcnew window_ide_final(ide, engine));
}
} // namespace Editor
} // namespace XRay

#pragma unmanaged
#include <objbase.h>
WINOLEAPI CoInitializeEx(IN LPVOID pvReserved, IN DWORD dwCoInit);
#pragma comment(lib, "ole32.lib")

extern "C" XR_EXPORT void initialize(XRay::Editor::ide_base*& ide, XRay::Editor::engine_base* engine)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    XRay::Editor::initialize_impl(ide, engine);
}

extern "C" XR_EXPORT void finalize(XRay::Editor::ide_base*& ide)
{
    delete (ide);
    ide = nullptr;
    XRay::Editor::g_ide = nullptr;
}
#pragma managed
