#include "pch.hpp"

#pragma unmanaged
#include <windows.h>
#include "../Include/editor/engine.hpp"
#include "ide_impl.hpp"
#pragma managed

#include "window_ide.h"

#pragma comment(lib,"user32.lib")

private ref class window_ide_final : public editor::window_ide {
public:
					window_ide_final	(editor::ide*& ide, editor::engine* engine) :
		editor::window_ide				(engine)
	{
		m_ide							= ide;
		Application::Idle				+= gcnew System::EventHandler(this, &window_ide_final::on_idle);
	}

					~window_ide_final	()
	{
		Application::Idle				-= gcnew System::EventHandler(this, &window_ide_final::on_idle);
		m_engine						= nullptr;
		m_ide							= nullptr;
	}

protected:
	virtual void	WndProc				(Message %m) override
	{
		LRESULT							result;
		if (m_engine && m_engine->on_message((HWND)m.HWnd.ToInt32(), m.Msg, m.WParam.ToInt32(), m.LParam.ToInt32(), result))
			return;

		editor::window_ide::WndProc		(m);
	}

private:
			void	on_idle				(System::Object ^sender, System::EventArgs ^event_args)
	{
		ide_impl*						impl = dynamic_cast<ide_impl*>(m_ide);
		impl->on_idle_start				();

		MSG								message;
		do {
			m_engine->on_idle			();
			impl->on_idle				();
		}
		while (m_engine && !PeekMessage(&message,HWND(0),0,0,0));

		impl->on_idle_end				();
	}
};

ide_impl*			g_ide = nullptr;

static void initialize_impl							(editor::ide*& ide, editor::engine* engine)
{
	VERIFY			(!g_ide);
	g_ide			= new ide_impl(engine);
	ide				= g_ide;
	g_ide->window	(gcnew window_ide_final(ide, engine));
}

#pragma unmanaged
#include <objbase.h>
WINOLEAPI  CoInitializeEx(IN LPVOID pvReserved, IN DWORD dwCoInit);
#pragma comment(lib,"ole32.lib")

extern "C" __declspec(dllexport)	void initialize	(editor::ide*& ide, editor::engine* engine)
{
	CoInitializeEx	(NULL, COINIT_APARTMENTTHREADED);
	initialize_impl	(ide, engine);
}

extern "C" __declspec(dllexport)	void finalize	(editor::ide*& ide)
{
	delete			(ide);
	ide				= nullptr;
	g_ide			= nullptr;
}
#pragma managed