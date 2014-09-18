#include "pch.hpp"
#include "window_weather_editor.h"
#include "window_ide.h"
#include "window_view.h"
#include "window_levels.h"
#include "window_weather.h"

using editor::window_ide;
using editor::window_view;
using editor::window_levels;
using editor::window_weather;
using editor::window_weather_editor;

using Microsoft::Win32::Registry;
using Microsoft::Win32::RegistryKey;
using Microsoft::Win32::RegistryValueKind;

#define COMPANY_NAME	"GSC Game World"
#define PRODUCT_NAME	"S.T.A.L.K.E.R.: CLear Sky"

template <typename T>
inline static T registry_value				(RegistryKey ^key, String ^value_id, const T &default_value)
{
	array<String^>		^names = key->GetValueNames();
	if (names->IndexOf(names,value_id) >= 0)
		return			((T)key->GetValue(value_id));

	return				(default_value);
}

RegistryKey ^window_ide::base_registry_key	()
{
	RegistryKey			^software = Registry::CurrentUser->OpenSubKey("Software",true);
	VERIFY				(software);

	RegistryKey			^company = software->OpenSubKey(COMPANY_NAME,true);
	if (!company)
		company			= software->CreateSubKey(COMPANY_NAME);
	VERIFY				(company);
	software->Close		();

	RegistryKey			^product = company->OpenSubKey(PRODUCT_NAME,true);
	if (!product)
		product			= company->CreateSubKey(PRODUCT_NAME);

	VERIFY				(product);
	company->Close		();

	return				(product);
}

void window_ide::save_on_exit				()
{
	RegistryKey			^product = base_registry_key();
	VERIFY				(product);

	RegistryKey			^windows = product->CreateSubKey("windows");
	using System::IO::MemoryStream;
	MemoryStream		^stream = gcnew MemoryStream();
	Editor->SaveAsXml	(stream, System::Text::Encoding::Unicode, true);
	stream->Seek		(0, System::IO::SeekOrigin::Begin);
	windows->SetValue	("editor",stream->ToArray());
	delete				stream;

	RegistryKey				^ide = windows->CreateSubKey("ide");
	{
		RegistryKey			^position = ide->CreateSubKey("position");
		position->SetValue	("left",	m_window_rectangle->Left);
		position->SetValue	("top",		m_window_rectangle->Top);
		position->SetValue	("width",	m_window_rectangle->Width);
		position->SetValue	("height",	m_window_rectangle->Height);
		position->Close		();
	}

	switch (WindowState) {
		case FormWindowState::Maximized : {
			ide->SetValue	("window_state", 1);
			break;
		}
		default : {
			ide->SetValue	("window_state", 2);
			break;
		}
	}

	ide->Close				();

	m_weather_editor->save	(windows);

	windows->Close			();
	product->Close			();
}

WeifenLuo::WinFormsUI::IDockContent ^window_ide::reload_content	(System::String ^persist_string)
{
	if (persist_string == "editor.window_view")
		return			(m_view);

	if (persist_string == "editor.window_levels")
		return			(m_levels);

	if (persist_string == "editor.window_weather")
		return			(m_weather);

	if (persist_string == "editor.window_weather_editor")
		return			(m_weather_editor);

	return				(nullptr);
}

void window_ide::load_on_create				()
{
	Width				= 800;
	Height				= 600;

	m_window_rectangle	= gcnew Drawing::Rectangle(Location,Size);

	RegistryKey			^product = base_registry_key();
	VERIFY				(product);

	RegistryKey			^windows = product->OpenSubKey("windows");
	if (windows) {
//		m_weather_editor->load	(windows);

		RegistryKey			^ide = windows->OpenSubKey("ide");
		if (ide) {
			RegistryKey		^position = ide->OpenSubKey("position");
			if (position) {
				Left		= (int)registry_value(position,"left",Left);
				Top			= (int)registry_value(position,"top",Top);
				Width		= (int)registry_value(position,"width",Width);
				Height		= (int)registry_value(position,"height",Height);
				position->Close	();
			}

			m_window_rectangle	= gcnew Drawing::Rectangle(Location,Size);

			switch ((int)registry_value(ide,"window_state",2)) {
				case 1 : {
					WindowState	= FormWindowState::Maximized;
					break;
				}
				case 2 : {
					WindowState	= FormWindowState::Normal;
					break;
				}
				default : NODEFAULT;
			}

			ide->Close		();
		}

		Object				^temp = windows->GetValue("editor");
		
		if (temp) {
			System::Array	^object = safe_cast<System::Array^>(windows->GetValue("editor"));
			
			windows->Close	();
			delete			(windows);

			product->Close	();
			delete			(product);

			using System::IO::MemoryStream;
			MemoryStream	^stream = gcnew MemoryStream();
			stream->Write	(safe_cast<array<unsigned char,1>^>(object),0,object->Length);
			stream->Seek	(0,System::IO::SeekOrigin::Begin);
			Editor->LoadFromXml	(
				stream,
				gcnew WeifenLuo::WinFormsUI::DeserializeDockContent(
					this,
					&window_ide::reload_content
				)
			);
			delete			(stream);
			return;
		}

		windows->Close		();
		delete				(windows);
	}

	product->Close		();
	delete				(product);

	m_view->Show					(Editor, WeifenLuo::WinFormsUI::DockState::Document);
	m_levels->Show					(Editor, WeifenLuo::WinFormsUI::DockState::DockRight);
	m_weather->Show					(Editor, WeifenLuo::WinFormsUI::DockState::DockRight);
	m_weather_editor->Show			(Editor, WeifenLuo::WinFormsUI::DockState::DockRight);

	this->WindowState	= FormWindowState::Maximized;
}