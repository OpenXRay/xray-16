#include "pch.hpp"
#include "window_weather_editor.h"
#include "engine_include.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"
#include "property_color_base.hpp"
#include "window_ide.h"
#include "window_view.h"

using System::Windows::Forms::GridItem;
using System::String;
using System::Windows::Forms::Clipboard;

using Flobbster::Windows::Forms::PropertyBag;
typedef PropertyBag::PropertySpecDescriptor		PropertySpecDescriptor;

using editor::window_weather_editor;
using editor::engine;

using Microsoft::Win32::Registry;
using Microsoft::Win32::RegistryKey;
using Microsoft::Win32::RegistryValueKind;

struct engine_pauser_guard : private boost::noncopyable {
	engine&		m_engine;
	bool		m_weather_paused;

	inline	engine_pauser_guard	(engine& engine, bool const& value) :
		m_engine				(engine),
		m_weather_paused		(engine.weather_paused())
	{
		engine.weather_paused	(value);
	}

	inline	engine_pauser_guard	(engine& engine) :
		m_engine				(engine),
		m_weather_paused		(engine.weather_paused())
	{
	}

	inline	~engine_pauser_guard()
	{
		m_engine.weather_paused	(m_weather_paused);
	}
}; // struct engine_pauser_guard

void window_weather_editor::save									(Microsoft::Win32::RegistryKey^ root)
{
	RegistryKey				^weather_editor = root->CreateSubKey("weather_editor");
	current->save			(weather_editor, "property_grid_current");
	blend->save				(weather_editor, "property_grid_blend");
	target->save			(weather_editor, "property_grid_target");
	weather_editor->Close	();
	delete					(weather_editor);
}

void window_weather_editor::save									()
{
	RegistryKey				^product = m_ide->base_registry_key();
	VERIFY					(product);

	RegistryKey				^windows = product->CreateSubKey("windows");
	
	save					(windows);
	
	windows->Close			();
	delete					(windows);
	
	product->Close			();
	delete					(product);
}

void window_weather_editor::load									(Microsoft::Win32::RegistryKey^ root)
{
	RegistryKey				^weather_editor = root->OpenSubKey("weather_editor");
	if (!weather_editor)
		return;

	current->load			(weather_editor, "property_grid_current");
	blend->load				(weather_editor, "property_grid_blend");
	target->load			(weather_editor, "property_grid_target");

	weather_editor->Close	();
	delete					(weather_editor);
}

void window_weather_editor::load				()
{
	RegistryKey				^product = m_ide->base_registry_key();
	RegistryKey				^windows = product->OpenSubKey("windows");
	if (!windows)
		return;

	load					(windows);

	windows->Close			();
	delete					(windows);

	product->Close			();
	delete					(product);
}

void window_weather_editor::weathers_ids							(
		weathers_getter_type const& weathers_getter,
		weathers_size_getter_type const& weathers_size_getter,
		frames_getter_type const& frames_getter,
		frames_size_getter_type const& frames_size_getter
	)
{
	VERIFY							(!m_weathers_getter);
	m_weathers_getter				= new weathers_getter_type(weathers_getter);

	VERIFY							(!m_weathers_size_getter);
	m_weathers_size_getter			= new weathers_size_getter_type(weathers_size_getter);

	VERIFY							(!m_frames_getter);
	m_frames_getter					= new frames_getter_type(frames_getter);

	VERIFY							(!m_frames_size_getter);
	m_frames_size_getter			= new frames_size_getter_type(frames_size_getter);

	fill_weathers					();
}

void window_weather_editor::fill_weathers							()
{
	TimeFactorNumericUpDown->Value	= System::Decimal(m_engine.weather_time_factor());

	VERIFY							(m_weathers_getter);
	VERIFY							(m_weathers_size_getter);

	WeathersComboBox->Items->Clear();

	LPCSTR const*					weathers_ids = (*m_weathers_getter)();
	for (u32 i=0, n=(*m_weathers_size_getter)(); i<n; ++i)
		WeathersComboBox->Items->Add(
			to_string(
				weathers_ids[i]
			)
		);

	LPCSTR							current_weather_id = m_engine.weather();
	int								index = WeathersComboBox->Items->IndexOf(to_string(current_weather_id));
	if ((index == -1) && WeathersComboBox->Items->Count)
		index						= 0;

	WeathersComboBox->SelectedIndex	= index;

	FramesComboBox->Items->Clear();

	if (index == -1)
		return;

	fill_frames						(current_weather_id);
}

void window_weather_editor::fill_frames								(LPCSTR current_weather_id)
{
	FramesComboBox->Items->Clear	();

	LPCSTR const*					frames_ids = (*m_frames_getter)(current_weather_id);
	for (u32 i=0, n=(*m_frames_size_getter)(current_weather_id); i<n; ++i)
		FramesComboBox->Items->Add(
			to_string(
				frames_ids[i]
			)
		);

	int								index = FramesComboBox->Items->IndexOf(to_string(m_engine.current_weather_frame()));
	if ((index == -1) && FramesComboBox->Items->Count)
		index						= 0;

	VERIFY							(!m_update_frames_combo_box);
	m_update_frames_combo_box		= true;
	FramesComboBox->SelectedIndex	= index;
	m_update_frames_combo_box		= false;
}

Void window_weather_editor::window_weather_editor_Enter				(Object^ sender, EventArgs^ e)
{
	if (!m_weathers_getter)
		return;

	fill_weathers					();
}

void window_weather_editor::on_load_finished						()
{
	fill_weathers					();

	::property_holder*				properties = dynamic_cast<::property_holder*>(m_engine.blend_frame_property_holder());
	VERIFY							(properties);
	blend->SelectedObject			= properties->container();

	PauseButton_Click				(nullptr, nullptr);

	load							();
	m_load_finished					= true;

	CurrentTimeTrackBar_ValueChanged(nullptr, nullptr);
}

Void window_weather_editor::WeathersComboBox_SelectedIndexChanged	(Object^ sender, EventArgs^ e)
{
	if (WeathersComboBox->SelectedIndex == -1)
		return;

	String^							new_weather = safe_cast<String^>(WeathersComboBox->SelectedItem);
	LPSTR							new_weather_id = to_string(new_weather);
	m_engine.weather				(new_weather_id);
	free							(new_weather_id);

	WeathersComboBox->SelectedIndex	= WeathersComboBox->Items->IndexOf(to_string(m_engine.weather()));
	fill_frames						(m_engine.weather());
}

Void window_weather_editor::FramesComboBox_SelectedIndexChanged		(Object^ sender, EventArgs^ e)
{
	if (m_update_frames_combo_box)
		return;

	if (FramesComboBox->SelectedIndex == -1)
		return;

	engine_pauser_guard				guard(m_engine, false);

	String^							new_weather_frame = safe_cast<String^>(FramesComboBox->SelectedItem);
	LPSTR							new_weather_frame_id = to_string(new_weather_frame);
	m_engine.current_weather_frame	(new_weather_frame_id);
	free							(new_weather_frame_id);

	update_frame					();
}

Void window_weather_editor::PreviousFrameButton_Click				(Object^ sender, EventArgs^ e)
{
	if (FramesComboBox->SelectedIndex == -1)
		return;

	if (FramesComboBox->SelectedIndex == 0)
		FramesComboBox->SelectedIndex	= FramesComboBox->Items->Count - 1;
	else
		FramesComboBox->SelectedIndex	= FramesComboBox->SelectedIndex - 1;
}

Void window_weather_editor::NextFrameButton_Click					(Object^ sender, EventArgs^ e)
{
	if (FramesComboBox->SelectedIndex == -1)
		return;

	if (FramesComboBox->SelectedIndex == FramesComboBox->Items->Count - 1)
		FramesComboBox->SelectedIndex	= 0;
	else
		FramesComboBox->SelectedIndex	= FramesComboBox->SelectedIndex + 1;
}

void window_weather_editor::update_frame							()
{
	VERIFY								(!m_update_text_value);
	m_update_text_value					= true;
	CurrentTimeTextBox->Text			= to_string(m_engine.weather_current_time());
	m_update_text_value					= false;

	{
		VERIFY							(!m_update_weather_time);
		m_update_weather_time			= true;
		
		u32								old_value = WeatherTrackBar->Value;
		u32								current_value = u32(1000*m_engine.track_weather());
		if (old_value != current_value)
			WeatherTrackBar->Value		= current_value;

		m_update_weather_time			= false;
	}
	{
		VERIFY							(!m_update_frame_trackbar);
		m_update_frame_trackbar			= true;
		u32								old_value = CurrentTimeTrackBar->Value;
		u32								current_value = u32(1000*m_engine.track_frame());
		if (old_value != current_value)
			CurrentTimeTrackBar->Value	= current_value;
		
		m_update_frame_trackbar			= false;
	}

	::property_holder*				properties;

	properties						= dynamic_cast<::property_holder*>(m_engine.current_frame_property_holder());
	if (properties) {
		if (safe_cast<property_container^>(current->SelectedObject) != properties->container()) {
			current->SelectedObject	= properties->container();
			blend->Refresh			();
		}
	}

	if (!m_engine.weather_paused() && m_load_finished)
		blend->Refresh				();

	properties						= dynamic_cast<::property_holder*>(m_engine.target_frame_property_holder());
	if (properties) {
		if (safe_cast<property_container^>(target->SelectedObject) != properties->container()) {
			target->SelectedObject	= properties->container();
			blend->Refresh			();
		}
	}

	if (!m_update_enabled)
		return;

	String^							new_frame_id = to_string(m_engine.current_weather_frame());
	if (new_frame_id == FramesComboBox->SelectedItem)
		return;

	VERIFY							(!m_update_frames_combo_box);
	m_update_frames_combo_box		= true;
	FramesComboBox->SelectedIndex	= FramesComboBox->Items->IndexOf(new_frame_id);
	m_update_frames_combo_box		= false;
}

void window_weather_editor::on_idle									()
{
	update_frame					();
	
	if (m_mouse_down)
		blend->Refresh				();

	if (ActiveControl == current)
		m_ide->view().property_grid	(current);
	else
		if (ActiveControl == target)
			m_ide->view().property_grid	(target);
}

Void window_weather_editor::FramesComboBox_DropDown					(Object^ sender, EventArgs^ e)
{
	m_update_enabled				= false;
}

Void window_weather_editor::FramesComboBox_DropDownClosed			(Object^ sender, EventArgs^ e)
{
	m_update_enabled				= true;
}

Void window_weather_editor::PauseButton_Click						(Object^ sender, EventArgs^ e)
{
	PauseButton->ImageIndex			= (PauseButton->ImageIndex ^ 1);
	m_engine.weather_paused			(PauseButton->ImageIndex ? true : false);
}

Void window_weather_editor::TimeFactorNumericUpDown_ValueChanged	(Object^ sender, EventArgs^ e)
{
	m_engine.weather_time_factor	(float(TimeFactorNumericUpDown->Value));
}

Void window_weather_editor::CopyButton_Click						(Object^ sender, EventArgs^ e)
{
	char							buffer[4096];
	if (!m_engine.save_time_frame(buffer, sizeof(buffer)))
		return;

	System::Windows::Forms::Clipboard::SetText(to_string(buffer));
}

Void window_weather_editor::PasteCurrentButton_Click				(Object^ sender, EventArgs^ e)
{
	char*							buffer = to_string(Clipboard::GetText());
	if (buffer)
		m_engine.paste_current_time_frame	(buffer, (strlen(buffer) + 1)*sizeof(char));
	free							(buffer);

	fill_frames						(m_engine.weather());
	current->Refresh				();
}

Void window_weather_editor::PasteTargetButton_Click					(Object^ sender, EventArgs^ e)
{
	char*							buffer = to_string(Clipboard::GetText());
	if (buffer)
		m_engine.paste_target_time_frame(buffer, (strlen(buffer) + 1)*sizeof(char));
	free							(buffer);

	fill_frames						(m_engine.weather());
	target->Refresh					();
}

Void window_weather_editor::CreateFromButton_Click					(Object^ sender, EventArgs^ e)
{
	char							buffer[4096];
	if (!m_engine.save_time_frame(buffer, sizeof(buffer)))
		return;

	m_engine.add_time_frame			(buffer, (strlen(buffer) + 1)*sizeof(char));
	
	fill_frames						(m_engine.weather());
	current->Refresh				();
	blend->Refresh					();
	target->Refresh					();

	CurrentTimeTrackBar->Value		= 1000;
}

Void window_weather_editor::window_weather_editor_SizeChanged		(Object^ sender, EventArgs^ e)
{
	panel12->Width					= int(Width*.4f);
	panel14->Width					= int(Width*.4f);
}

Void window_weather_editor::CurrentTimeTrackBar_ValueChanged		(Object^ sender, EventArgs^ e)
{
	if (m_update_frame_trackbar)
		return;

	engine_pauser_guard				guard(m_engine, false);
	m_engine.track_frame			(CurrentTimeTrackBar->Value / 1000.f);
	update_frame					();
	
	if (sender)
		m_engine.on_idle			();
}

Void window_weather_editor::CurrentTimeTrackBar_MouseDown			(Object^ sender, MouseEventArgs^ e)
{
	m_engine.weather_paused			(true);

	Point							point = e->Location;
	int								p = point.X;
	int								margin = 12;
	p								-= margin;
	float							coef = 
		float(CurrentTimeTrackBar->Maximum - CurrentTimeTrackBar->Minimum)/
		float(CurrentTimeTrackBar->ClientSize.Width - 2*margin);
	
	int								value = int(p * coef + CurrentTimeTrackBar->Minimum);
	if (value < CurrentTimeTrackBar->Minimum)
		value						= CurrentTimeTrackBar->Minimum;
	else {
		if (value > CurrentTimeTrackBar->Maximum)
			value						= CurrentTimeTrackBar->Maximum;
	}
	
	CurrentTimeTrackBar->Value		= value;

	m_mouse_down					= true;
}

Void window_weather_editor::CurrentTimeTrackBar_MouseUp				(Object^ sender, MouseEventArgs^ e)
{
	m_engine.weather_paused			(PauseButton->ImageIndex ? true : false);
	blend->Refresh					();
	m_mouse_down					= false;
}

Void window_weather_editor::WeatherTrackBar_ValueChanged			(Object^ sender, EventArgs^ e)
{
	if (m_update_weather_time)
		return;

	engine_pauser_guard				guard(m_engine, false);
	m_engine.track_weather			(WeatherTrackBar->Value / 1000.f);
	update_frame					();
	m_engine.on_idle				();
}

Void window_weather_editor::WeatherTrackBar_MouseDown				(Object^ sender, MouseEventArgs^ e)
{
	m_engine.weather_paused			(true);

	Point							point = e->Location;
	int								p = point.X;
	int								margin = 12;
	p								-= margin;
	float							coef = 
		float(WeatherTrackBar->Maximum - WeatherTrackBar->Minimum)/
		float(WeatherTrackBar->ClientSize.Width - 2*margin);
	
	int								value = int(p * coef + WeatherTrackBar->Minimum);
	if (value < WeatherTrackBar->Minimum)
		value						= WeatherTrackBar->Minimum;
	else {
		if (value > WeatherTrackBar->Maximum)
			value					= WeatherTrackBar->Maximum;
	}
	
	WeatherTrackBar->Value			= value;

	m_mouse_down					= true;
}

Void window_weather_editor::WeatherTrackBar_MouseUp					(Object^ sender, MouseEventArgs^ e)
{
	m_engine.weather_paused			(PauseButton->ImageIndex ? true : false);
	blend->Refresh					();
	m_mouse_down					= false;
}

Void window_weather_editor::current_Leave							(Object^ sender, EventArgs^ e)
{
	m_ide->view().property_grid		(current);
}

Void window_weather_editor::target_Leave							(Object^ sender, EventArgs^ e)
{
	m_ide->view().property_grid		(target);
}

Void window_weather_editor::current_Enter							(Object^ sender, EventArgs^ e)
{
	m_ide->view().property_grid		(current);
}

Void window_weather_editor::target_Enter							(Object^ sender, EventArgs^ e)
{
	m_ide->view().property_grid		(target);
}

Void window_weather_editor::CurrentTimeTextBox_TextChanged			(Object^ sender, EventArgs^ e)
{
	if (m_update_text_value)
		return;

	if (CurrentTimeTextBox->Text->Contains("_"))
		return;

	engine_pauser_guard				guard(m_engine, false);

	char*							string = to_string(CurrentTimeTextBox->Text);
	m_engine.weather_current_time	(string);
	free							(string);

	update_frame					();

	blend->Refresh					();
}

Void window_weather_editor::ReloadCurrentButton_Click				(Object^ sender, EventArgs^ e)
{
	m_engine.reload_current_time_frame	();
	current->Refresh					();
}

Void window_weather_editor::ReloadTargetButton_Click				(Object^ sender, EventArgs^ e)
{
	m_engine.reload_target_time_frame	();
	target->Refresh						();
}