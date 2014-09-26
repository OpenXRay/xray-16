#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace WeifenLuo {
namespace WinFormsUI {
	interface class IDockContent;
} // namespace WinFormsUI
} // namespace WeifenLuo

namespace editor {

	class engine;
	class ide;

	ref class window_view;
	ref class window_levels;
	ref class window_weather;
	ref class window_weather_editor;

	/// <summary>
	/// Summary for window_ide
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class window_ide : public System::Windows::Forms::Form
	{
	public:
		window_ide(editor::engine	*engine)
		{
			InitializeComponent	();
			//
			//TODO: Add the constructor code here
			//
			custom_init			(engine);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~window_ide()
		{
			custom_finalize		();
			if (components)
			{
				delete components;
			}
		}

	protected: 

	private: WeifenLuo::WinFormsUI::DockPanel^  Editor;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->Editor = (gcnew WeifenLuo::WinFormsUI::DockPanel());
			this->SuspendLayout();
			// 
			// Editor
			// 
			this->Editor->ActiveAutoHideContent = nullptr;
			this->Editor->Dock = System::Windows::Forms::DockStyle::Fill;
			this->Editor->DocumentStyle = WeifenLuo::WinFormsUI::DocumentStyles::DockingSdi;
			this->Editor->Font = (gcnew System::Drawing::Font(L"Tahoma", 11, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::World));
			this->Editor->Location = System::Drawing::Point(0, 0);
			this->Editor->Name = L"Editor";
			this->Editor->Size = System::Drawing::Size(632, 453);
			this->Editor->TabIndex = 17;
			// 
			// window_ide
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(632, 453);
			this->Controls->Add(this->Editor);
			this->Name = L"window_ide";
			this->StartPosition = System::Windows::Forms::FormStartPosition::Manual;
			this->Text = L"editor";
			this->Deactivate += gcnew System::EventHandler(this, &window_ide::window_ide_Deactivate);
			this->SizeChanged += gcnew System::EventHandler(this, &window_ide::window_ide_SizeChanged);
			this->Activated += gcnew System::EventHandler(this, &window_ide::window_ide_Activated);
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &window_ide::window_ide_FormClosing);
			this->LocationChanged += gcnew System::EventHandler(this, &window_ide::window_ide_LocationChanged);
			this->ResumeLayout(false);

		}
#pragma endregion
protected:
	editor::engine	*m_engine;

private:
	System::Drawing::Rectangle^					m_window_rectangle;

private:
	window_view^			m_view;
	window_levels^			m_levels;
	window_weather^			m_weather;
	window_weather_editor^	m_weather_editor;

protected:
	editor::ide				*m_ide;

public:
	editor::ide&			ide					();
	window_view%			view				();
	editor::engine&			engine				();

public:
	window_levels%			levels				();
	window_weather%			weather				();
	window_weather_editor%	weather_editor		();
	Microsoft::Win32::RegistryKey^ base_registry_key();

private:
			void	custom_init					(editor::engine	*engine);
			void	custom_finalize				();
			void	save_on_exit				();
			void	load_on_create				();

private:
	WeifenLuo::WinFormsUI::IDockContent^reload_content		(System::String ^persist_string);

private:
			Void	window_ide_SizeChanged		(System::Object^  sender, System::EventArgs^  e);
			Void	window_ide_LocationChanged	(System::Object^  sender, System::EventArgs^  e);
			Void	window_ide_FormClosing		(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e);
			Void	window_ide_Activated		(System::Object^  sender, System::EventArgs^  e);
			Void	window_ide_Deactivate		(System::Object^  sender, System::EventArgs^  e);
};

}
