#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace editor {

	ref class window_ide;
	/// <summary>
	/// Summary for window_view
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class window_view : public WeifenLuo::WinFormsUI::DockContent
	{
	public:
		window_view(window_ide %ide)
		{
			InitializeComponent	();
			//
			//TODO: Add the constructor code here
			//
			custom_init			(ide);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~window_view()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::ToolStrip^  MainToolBar;
	protected: 
	private: System::Windows::Forms::ToolStripButton^  EditButton;
	private: System::Windows::Forms::ToolStripButton^  PauseButton;
	private: System::Windows::Forms::Panel^  ViewPanel;
	private: System::Windows::Forms::ImageList^  imageList1;
	private: System::ComponentModel::IContainer^  components;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(window_view::typeid));
			this->MainToolBar = (gcnew System::Windows::Forms::ToolStrip());
			this->EditButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->PauseButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->ViewPanel = (gcnew System::Windows::Forms::Panel());
			this->imageList1 = (gcnew System::Windows::Forms::ImageList(this->components));
			this->MainToolBar->SuspendLayout();
			this->SuspendLayout();
			// 
			// MainToolBar
			// 
			this->MainToolBar->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {this->EditButton, this->PauseButton});
			this->MainToolBar->Location = System::Drawing::Point(0, 0);
			this->MainToolBar->Name = L"MainToolBar";
			this->MainToolBar->Size = System::Drawing::Size(292, 25);
			this->MainToolBar->TabIndex = 16;
			this->MainToolBar->Text = L"toolStrip1";
			// 
			// EditButton
			// 
			this->EditButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Text;
			this->EditButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"EditButton.Image")));
			this->EditButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->EditButton->Name = L"EditButton";
			this->EditButton->Size = System::Drawing::Size(29, 22);
			this->EditButton->Text = L"Edit";
			this->EditButton->Click += gcnew System::EventHandler(this, &window_view::EditButton_Click);
			// 
			// PauseButton
			// 
			this->PauseButton->Checked = true;
			this->PauseButton->CheckState = System::Windows::Forms::CheckState::Checked;
			this->PauseButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Text;
			this->PauseButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"PauseButton.Image")));
			this->PauseButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->PauseButton->Name = L"PauseButton";
			this->PauseButton->Size = System::Drawing::Size(40, 22);
			this->PauseButton->Text = L"Pause";
			this->PauseButton->Click += gcnew System::EventHandler(this, &window_view::PauseButton_Click);
			// 
			// ViewPanel
			// 
			this->ViewPanel->Dock = System::Windows::Forms::DockStyle::Fill;
			this->ViewPanel->Location = System::Drawing::Point(0, 25);
			this->ViewPanel->Name = L"ViewPanel";
			this->ViewPanel->Size = System::Drawing::Size(292, 248);
			this->ViewPanel->TabIndex = 17;
			this->ViewPanel->DoubleClick += gcnew System::EventHandler(this, &window_view::window_view_DoubleClick);
			this->ViewPanel->MouseLeave += gcnew System::EventHandler(this, &window_view::ViewPanel_MouseLeave);
			this->ViewPanel->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &window_view::ViewPanel_MouseDown);
			this->ViewPanel->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &window_view::ViewPanel_MouseMove);
			this->ViewPanel->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &window_view::ViewPanel_MouseClick);
			// 
			// imageList1
			// 
			this->imageList1->ImageStream = (cli::safe_cast<System::Windows::Forms::ImageListStreamer^  >(resources->GetObject(L"imageList1.ImageStream")));
			this->imageList1->TransparentColor = System::Drawing::Color::Fuchsia;
			this->imageList1->Images->SetKeyName(0, L"color_picker.bmp");
			// 
			// window_view
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(292, 273);
			this->CloseButton = false;
			this->Controls->Add(this->ViewPanel);
			this->Controls->Add(this->MainToolBar);
			this->DockableAreas = WeifenLuo::WinFormsUI::DockAreas::Document;
			this->HideOnClose = true;
			this->KeyPreview = true;
			this->Name = L"window_view";
			this->TabText = L"View";
			this->Text = L"View";
			this->Deactivate += gcnew System::EventHandler(this, &window_view::window_view_Deactivate);
			this->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &window_view::window_view_Paint);
			this->SizeChanged += gcnew System::EventHandler(this, &window_view::window_view_SizeChanged);
			this->Activated += gcnew System::EventHandler(this, &window_view::window_view_Activated);
			this->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &window_view::window_view_KeyUp);
			this->LocationChanged += gcnew System::EventHandler(this, &window_view::window_view_LocationChanged);
			this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &window_view::window_view_KeyDown);
			this->MainToolBar->ResumeLayout(false);
			this->MainToolBar->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
public:
			void	pause						();
			void	on_load_finished			();
			void	property_grid				(PropertyGrid^ property_grid);
			void	on_idle						();
			IntPtr	draw_handle					();
			Void	window_view_LocationChanged	(System::Object^  sender, System::EventArgs^  e);
			Void	window_view_Activated		(System::Object^  sender, System::EventArgs^  e);
			Void	window_view_Deactivate		(System::Object^  sender, System::EventArgs^  e);

private:
	window_ide		^m_ide;
	editor::engine	*m_engine;
	PropertyGrid^	m_property_grid;
	Point			m_previous_location;
	bool			m_loaded;

private:
			void	reclip_cursor				();
			bool	pick_color_cursor			();
			void	pick_color_cursor			(bool value);
			void	check_cursor				();

private:
			void	custom_init					(window_ide %ide);
			Void	window_view_DoubleClick		(Object^ sender, EventArgs^ e);
			Void	window_view_SizeChanged		(Object^ sender, EventArgs^ e);
			Void	window_view_KeyUp			(Object^ sender, KeyEventArgs^ e);
			Void	EditButton_Click			(Object^ sender, EventArgs^ e);
			Void	PauseButton_Click			(Object^ sender, EventArgs^ e);
			Void	window_view_Paint			(Object^ sender, PaintEventArgs^ e);
			Void	ViewPanel_MouseMove			(Object^ sender, MouseEventArgs^ e);
			Void	ViewPanel_MouseDown			(Object^ sender, MouseEventArgs^ e);
			Void	ViewPanel_MouseLeave		(Object^ sender, EventArgs^ e);
			Void	ViewPanel_MouseClick		(Object^ sender, MouseEventArgs^ e);
			Void	window_view_KeyDown			(Object^ sender, KeyEventArgs^ e);
}; // ref class window_view

} // namespace editor