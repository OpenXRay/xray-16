#pragma once

#include "ide_impl.hpp"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace editor {

	class engine;
	ref class window_ide;
	/// <summary>
	/// Summary for window_weather_editor
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class window_weather_editor : public WeifenLuo::WinFormsUI::DockContent
	{
	public:
		window_weather_editor(window_ide^ ide, engine* engine) :
			m_ide					(ide),
			m_engine				(*engine),
			m_weathers_getter		(0),
			m_weathers_size_getter	(0),
			m_frames_getter			(0),
			m_frames_size_getter	(0),
			m_update_enabled		(true),
			m_load_finished			(false),
			m_mouse_down			(false),
			m_update_frames_combo_box	(false),
			m_update_frame_trackbar	(false),
			m_update_text_value		(false),
			m_update_weather_time	(false)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~window_weather_editor()
		{
			if (components)
			{
				delete components;
			}
		}

	protected: 




































































	private: System::Windows::Forms::ImageList^  imageList1;









































private: System::Windows::Forms::Panel^  panel23;
private: System::Windows::Forms::Panel^  panel1;
private: System::Windows::Forms::Panel^  panel4;
private: System::Windows::Forms::TrackBar^  CurrentTimeTrackBar;
private: System::Windows::Forms::Panel^  panel10;
private: System::Windows::Forms::Button^  NextFrameButton;
private: System::Windows::Forms::Panel^  panel9;
private: System::Windows::Forms::Button^  PauseButton;
private: System::Windows::Forms::Button^  PreviousFrameButton;
private: System::Windows::Forms::Panel^  panel3;
private: System::Windows::Forms::Panel^  panel8;
private: System::Windows::Forms::ComboBox^  WeathersComboBox;
private: System::Windows::Forms::Panel^  panel7;
private: System::Windows::Forms::Label^  label2;
private: System::Windows::Forms::Panel^  panel6;
private: System::Windows::Forms::Label^  label1;
private: System::Windows::Forms::NumericUpDown^  TimeFactorNumericUpDown;
private: System::Windows::Forms::Panel^  panel5;
private: System::Windows::Forms::Label^  label3;
private: System::Windows::Forms::ComboBox^  FramesComboBox;
private: System::Windows::Forms::TrackBar^  WeatherTrackBar;

private: System::Windows::Forms::Panel^  panel25;
private: System::Windows::Forms::Panel^  panel24;
private: System::Windows::Forms::Label^  label7;

private: System::Windows::Forms::Panel^  panel2;
private: System::Windows::Forms::Panel^  panel13;
private: editor::controls::property_grid^  blend;
private: System::Windows::Forms::Panel^  panel15;
private: System::Windows::Forms::Label^  label6;
private: System::Windows::Forms::Panel^  panel22;
private: System::Windows::Forms::Button^  CopyButton;
private: System::Windows::Forms::Panel^  panel11;
private: System::Windows::Forms::Button^  CreateFromButton;
private: System::Windows::Forms::Splitter^  splitter2;
private: System::Windows::Forms::Panel^  panel14;
private: editor::controls::property_grid^  target;
private: System::Windows::Forms::Panel^  panel20;
private: System::Windows::Forms::Panel^  panel17;
private: System::Windows::Forms::Label^  label5;
private: System::Windows::Forms::Panel^  panel18;
private: System::Windows::Forms::Button^  PasteTargetButton;
private: System::Windows::Forms::Splitter^  splitter1;
private: System::Windows::Forms::Panel^  panel12;
private: editor::controls::property_grid^  current;
private: System::Windows::Forms::Panel^  panel19;
private: System::Windows::Forms::Panel^  panel16;
private: System::Windows::Forms::Label^  label4;
private: System::Windows::Forms::Panel^  panel21;
private: System::Windows::Forms::Button^  PasteCurrentButton;
private: System::Windows::Forms::MaskedTextBox^  CurrentTimeTextBox;
private: System::Windows::Forms::Label^  label8;
private: System::Windows::Forms::Button^  ReloadTargetButton;

private: System::Windows::Forms::Button^  ReloadCurrentButton;


	private: System::ComponentModel::IContainer^  components;





	protected: 




	protected: 

	protected: 

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
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(window_weather_editor::typeid));
			this->imageList1 = (gcnew System::Windows::Forms::ImageList(this->components));
			this->panel23 = (gcnew System::Windows::Forms::Panel());
			this->WeatherTrackBar = (gcnew System::Windows::Forms::TrackBar());
			this->panel25 = (gcnew System::Windows::Forms::Panel());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->panel24 = (gcnew System::Windows::Forms::Panel());
			this->CurrentTimeTextBox = (gcnew System::Windows::Forms::MaskedTextBox());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->panel4 = (gcnew System::Windows::Forms::Panel());
			this->CurrentTimeTrackBar = (gcnew System::Windows::Forms::TrackBar());
			this->panel10 = (gcnew System::Windows::Forms::Panel());
			this->NextFrameButton = (gcnew System::Windows::Forms::Button());
			this->panel9 = (gcnew System::Windows::Forms::Panel());
			this->PauseButton = (gcnew System::Windows::Forms::Button());
			this->PreviousFrameButton = (gcnew System::Windows::Forms::Button());
			this->panel3 = (gcnew System::Windows::Forms::Panel());
			this->panel8 = (gcnew System::Windows::Forms::Panel());
			this->WeathersComboBox = (gcnew System::Windows::Forms::ComboBox());
			this->panel7 = (gcnew System::Windows::Forms::Panel());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->panel6 = (gcnew System::Windows::Forms::Panel());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->TimeFactorNumericUpDown = (gcnew System::Windows::Forms::NumericUpDown());
			this->panel5 = (gcnew System::Windows::Forms::Panel());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->FramesComboBox = (gcnew System::Windows::Forms::ComboBox());
			this->panel2 = (gcnew System::Windows::Forms::Panel());
			this->panel13 = (gcnew System::Windows::Forms::Panel());
			this->blend = (gcnew editor::controls::property_grid());
			this->panel15 = (gcnew System::Windows::Forms::Panel());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->panel22 = (gcnew System::Windows::Forms::Panel());
			this->CopyButton = (gcnew System::Windows::Forms::Button());
			this->panel11 = (gcnew System::Windows::Forms::Panel());
			this->CreateFromButton = (gcnew System::Windows::Forms::Button());
			this->splitter2 = (gcnew System::Windows::Forms::Splitter());
			this->panel14 = (gcnew System::Windows::Forms::Panel());
			this->target = (gcnew editor::controls::property_grid());
			this->panel20 = (gcnew System::Windows::Forms::Panel());
			this->panel17 = (gcnew System::Windows::Forms::Panel());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->panel18 = (gcnew System::Windows::Forms::Panel());
			this->PasteTargetButton = (gcnew System::Windows::Forms::Button());
			this->splitter1 = (gcnew System::Windows::Forms::Splitter());
			this->panel12 = (gcnew System::Windows::Forms::Panel());
			this->current = (gcnew editor::controls::property_grid());
			this->panel19 = (gcnew System::Windows::Forms::Panel());
			this->panel16 = (gcnew System::Windows::Forms::Panel());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->panel21 = (gcnew System::Windows::Forms::Panel());
			this->PasteCurrentButton = (gcnew System::Windows::Forms::Button());
			this->ReloadCurrentButton = (gcnew System::Windows::Forms::Button());
			this->ReloadTargetButton = (gcnew System::Windows::Forms::Button());
			this->panel23->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->WeatherTrackBar))->BeginInit();
			this->panel25->SuspendLayout();
			this->panel24->SuspendLayout();
			this->panel1->SuspendLayout();
			this->panel4->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->CurrentTimeTrackBar))->BeginInit();
			this->panel10->SuspendLayout();
			this->panel9->SuspendLayout();
			this->panel3->SuspendLayout();
			this->panel7->SuspendLayout();
			this->panel6->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->TimeFactorNumericUpDown))->BeginInit();
			this->panel5->SuspendLayout();
			this->panel2->SuspendLayout();
			this->panel13->SuspendLayout();
			this->panel15->SuspendLayout();
			this->panel22->SuspendLayout();
			this->panel11->SuspendLayout();
			this->panel14->SuspendLayout();
			this->panel17->SuspendLayout();
			this->panel18->SuspendLayout();
			this->panel12->SuspendLayout();
			this->panel16->SuspendLayout();
			this->panel21->SuspendLayout();
			this->SuspendLayout();
			// 
			// imageList1
			// 
			this->imageList1->ImageStream = (cli::safe_cast<System::Windows::Forms::ImageListStreamer^  >(resources->GetObject(L"imageList1.ImageStream")));
			this->imageList1->TransparentColor = System::Drawing::Color::Fuchsia;
			this->imageList1->Images->SetKeyName(0, L"DataContainer_PauseHS.bmp");
			this->imageList1->Images->SetKeyName(1, L"DataContainer_MoveNextHS.png");
			// 
			// panel23
			// 
			this->panel23->Controls->Add(this->WeatherTrackBar);
			this->panel23->Controls->Add(this->panel25);
			this->panel23->Controls->Add(this->panel24);
			this->panel23->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel23->Location = System::Drawing::Point(0, 0);
			this->panel23->Name = L"panel23";
			this->panel23->Size = System::Drawing::Size(367, 43);
			this->panel23->TabIndex = 2;
			// 
			// WeatherTrackBar
			// 
			this->WeatherTrackBar->Dock = System::Windows::Forms::DockStyle::Fill;
			this->WeatherTrackBar->LargeChange = 1;
			this->WeatherTrackBar->Location = System::Drawing::Point(76, 21);
			this->WeatherTrackBar->Maximum = 1000;
			this->WeatherTrackBar->Name = L"WeatherTrackBar";
			this->WeatherTrackBar->Size = System::Drawing::Size(291, 22);
			this->WeatherTrackBar->TabIndex = 15;
			this->WeatherTrackBar->TickStyle = System::Windows::Forms::TickStyle::None;
			this->WeatherTrackBar->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &window_weather_editor::WeatherTrackBar_MouseDown);
			this->WeatherTrackBar->ValueChanged += gcnew System::EventHandler(this, &window_weather_editor::WeatherTrackBar_ValueChanged);
			this->WeatherTrackBar->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &window_weather_editor::WeatherTrackBar_MouseUp);
			// 
			// panel25
			// 
			this->panel25->Controls->Add(this->label8);
			this->panel25->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel25->Location = System::Drawing::Point(76, 0);
			this->panel25->Name = L"panel25";
			this->panel25->Size = System::Drawing::Size(291, 21);
			this->panel25->TabIndex = 14;
			// 
			// label8
			// 
			this->label8->AutoSize = true;
			this->label8->Location = System::Drawing::Point(6, 4);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(71, 13);
			this->label8->TabIndex = 5;
			this->label8->Text = L"total weather:";
			// 
			// panel24
			// 
			this->panel24->Controls->Add(this->CurrentTimeTextBox);
			this->panel24->Controls->Add(this->label7);
			this->panel24->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel24->Location = System::Drawing::Point(0, 0);
			this->panel24->Name = L"panel24";
			this->panel24->Size = System::Drawing::Size(76, 43);
			this->panel24->TabIndex = 0;
			// 
			// CurrentTimeTextBox
			// 
			this->CurrentTimeTextBox->Location = System::Drawing::Point(0, 22);
			this->CurrentTimeTextBox->Mask = L"00:00:00";
			this->CurrentTimeTextBox->Name = L"CurrentTimeTextBox";
			this->CurrentTimeTextBox->Size = System::Drawing::Size(70, 20);
			this->CurrentTimeTextBox->TabIndex = 5;
			this->CurrentTimeTextBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->CurrentTimeTextBox->TextChanged += gcnew System::EventHandler(this, &window_weather_editor::CurrentTimeTextBox_TextChanged);
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(0, 4);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(65, 13);
			this->label7->TabIndex = 4;
			this->label7->Text = L"current time:";
			// 
			// panel1
			// 
			this->panel1->Controls->Add(this->panel4);
			this->panel1->Controls->Add(this->panel3);
			this->panel1->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel1->Location = System::Drawing::Point(0, 43);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(367, 85);
			this->panel1->TabIndex = 3;
			// 
			// panel4
			// 
			this->panel4->Controls->Add(this->CurrentTimeTrackBar);
			this->panel4->Controls->Add(this->panel10);
			this->panel4->Controls->Add(this->panel9);
			this->panel4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel4->Location = System::Drawing::Point(0, 54);
			this->panel4->Name = L"panel4";
			this->panel4->Size = System::Drawing::Size(367, 31);
			this->panel4->TabIndex = 1;
			// 
			// CurrentTimeTrackBar
			// 
			this->CurrentTimeTrackBar->Dock = System::Windows::Forms::DockStyle::Top;
			this->CurrentTimeTrackBar->LargeChange = 1;
			this->CurrentTimeTrackBar->Location = System::Drawing::Point(113, 0);
			this->CurrentTimeTrackBar->Maximum = 1000;
			this->CurrentTimeTrackBar->Name = L"CurrentTimeTrackBar";
			this->CurrentTimeTrackBar->Size = System::Drawing::Size(254, 42);
			this->CurrentTimeTrackBar->TabIndex = 11;
			this->CurrentTimeTrackBar->TickStyle = System::Windows::Forms::TickStyle::None;
			this->CurrentTimeTrackBar->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &window_weather_editor::CurrentTimeTrackBar_MouseDown);
			this->CurrentTimeTrackBar->ValueChanged += gcnew System::EventHandler(this, &window_weather_editor::CurrentTimeTrackBar_ValueChanged);
			this->CurrentTimeTrackBar->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &window_weather_editor::CurrentTimeTrackBar_MouseUp);
			// 
			// panel10
			// 
			this->panel10->Controls->Add(this->NextFrameButton);
			this->panel10->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel10->Location = System::Drawing::Point(78, 0);
			this->panel10->Name = L"panel10";
			this->panel10->Size = System::Drawing::Size(35, 31);
			this->panel10->TabIndex = 9;
			// 
			// NextFrameButton
			// 
			this->NextFrameButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"NextFrameButton.Image")));
			this->NextFrameButton->Location = System::Drawing::Point(0, 0);
			this->NextFrameButton->Name = L"NextFrameButton";
			this->NextFrameButton->Size = System::Drawing::Size(32, 23);
			this->NextFrameButton->TabIndex = 7;
			this->NextFrameButton->UseVisualStyleBackColor = true;
			this->NextFrameButton->Click += gcnew System::EventHandler(this, &window_weather_editor::NextFrameButton_Click);
			// 
			// panel9
			// 
			this->panel9->Controls->Add(this->PauseButton);
			this->panel9->Controls->Add(this->PreviousFrameButton);
			this->panel9->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel9->Location = System::Drawing::Point(0, 0);
			this->panel9->Name = L"panel9";
			this->panel9->Size = System::Drawing::Size(78, 31);
			this->panel9->TabIndex = 8;
			// 
			// PauseButton
			// 
			this->PauseButton->BackgroundImageLayout = System::Windows::Forms::ImageLayout::Center;
			this->PauseButton->ImageIndex = 0;
			this->PauseButton->ImageList = this->imageList1;
			this->PauseButton->Location = System::Drawing::Point(0, 0);
			this->PauseButton->Name = L"PauseButton";
			this->PauseButton->Size = System::Drawing::Size(32, 23);
			this->PauseButton->TabIndex = 7;
			this->PauseButton->UseVisualStyleBackColor = true;
			this->PauseButton->Click += gcnew System::EventHandler(this, &window_weather_editor::PauseButton_Click);
			// 
			// PreviousFrameButton
			// 
			this->PreviousFrameButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"PreviousFrameButton.Image")));
			this->PreviousFrameButton->Location = System::Drawing::Point(46, 0);
			this->PreviousFrameButton->Name = L"PreviousFrameButton";
			this->PreviousFrameButton->Size = System::Drawing::Size(32, 23);
			this->PreviousFrameButton->TabIndex = 6;
			this->PreviousFrameButton->UseVisualStyleBackColor = true;
			this->PreviousFrameButton->Click += gcnew System::EventHandler(this, &window_weather_editor::PreviousFrameButton_Click);
			// 
			// panel3
			// 
			this->panel3->AutoSize = true;
			this->panel3->Controls->Add(this->panel8);
			this->panel3->Controls->Add(this->WeathersComboBox);
			this->panel3->Controls->Add(this->panel7);
			this->panel3->Controls->Add(this->panel6);
			this->panel3->Controls->Add(this->panel5);
			this->panel3->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel3->Location = System::Drawing::Point(0, 0);
			this->panel3->Name = L"panel3";
			this->panel3->Size = System::Drawing::Size(367, 54);
			this->panel3->TabIndex = 0;
			// 
			// panel8
			// 
			this->panel8->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panel8->Location = System::Drawing::Point(85, 46);
			this->panel8->Name = L"panel8";
			this->panel8->Size = System::Drawing::Size(196, 8);
			this->panel8->TabIndex = 10;
			// 
			// WeathersComboBox
			// 
			this->WeathersComboBox->Dock = System::Windows::Forms::DockStyle::Top;
			this->WeathersComboBox->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->WeathersComboBox->FormattingEnabled = true;
			this->WeathersComboBox->Location = System::Drawing::Point(85, 25);
			this->WeathersComboBox->MaxDropDownItems = 16;
			this->WeathersComboBox->Name = L"WeathersComboBox";
			this->WeathersComboBox->Size = System::Drawing::Size(196, 21);
			this->WeathersComboBox->TabIndex = 9;
			this->WeathersComboBox->SelectedIndexChanged += gcnew System::EventHandler(this, &window_weather_editor::WeathersComboBox_SelectedIndexChanged);
			// 
			// panel7
			// 
			this->panel7->Controls->Add(this->label2);
			this->panel7->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel7->Location = System::Drawing::Point(85, 0);
			this->panel7->Name = L"panel7";
			this->panel7->Size = System::Drawing::Size(196, 25);
			this->panel7->TabIndex = 2;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(3, 7);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(48, 13);
			this->label2->TabIndex = 2;
			this->label2->Text = L"weather:";
			// 
			// panel6
			// 
			this->panel6->Controls->Add(this->label1);
			this->panel6->Controls->Add(this->TimeFactorNumericUpDown);
			this->panel6->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel6->Location = System::Drawing::Point(0, 0);
			this->panel6->Name = L"panel6";
			this->panel6->Size = System::Drawing::Size(85, 54);
			this->panel6->TabIndex = 1;
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(0, 7);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(59, 13);
			this->label1->TabIndex = 1;
			this->label1->Text = L"time factor:";
			// 
			// TimeFactorNumericUpDown
			// 
			this->TimeFactorNumericUpDown->DecimalPlaces = 1;
			this->TimeFactorNumericUpDown->Increment = System::Decimal(gcnew cli::array< System::Int32 >(4) {10, 0, 0, 0});
			this->TimeFactorNumericUpDown->Location = System::Drawing::Point(0, 25);
			this->TimeFactorNumericUpDown->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {100000, 0, 0, 0});
			this->TimeFactorNumericUpDown->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1, 0, 0, 131072});
			this->TimeFactorNumericUpDown->Name = L"TimeFactorNumericUpDown";
			this->TimeFactorNumericUpDown->Size = System::Drawing::Size(70, 20);
			this->TimeFactorNumericUpDown->TabIndex = 0;
			this->TimeFactorNumericUpDown->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {1, 0, 0, 0});
			this->TimeFactorNumericUpDown->ValueChanged += gcnew System::EventHandler(this, &window_weather_editor::TimeFactorNumericUpDown_ValueChanged);
			// 
			// panel5
			// 
			this->panel5->Controls->Add(this->label3);
			this->panel5->Controls->Add(this->FramesComboBox);
			this->panel5->Dock = System::Windows::Forms::DockStyle::Right;
			this->panel5->Location = System::Drawing::Point(281, 0);
			this->panel5->Name = L"panel5";
			this->panel5->Size = System::Drawing::Size(86, 54);
			this->panel5->TabIndex = 0;
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(11, 7);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(58, 13);
			this->label3->TabIndex = 13;
			this->label3->Text = L"time frame:";
			// 
			// FramesComboBox
			// 
			this->FramesComboBox->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->FramesComboBox->FormattingEnabled = true;
			this->FramesComboBox->Location = System::Drawing::Point(11, 25);
			this->FramesComboBox->MaxDropDownItems = 16;
			this->FramesComboBox->Name = L"FramesComboBox";
			this->FramesComboBox->Size = System::Drawing::Size(75, 21);
			this->FramesComboBox->TabIndex = 12;
			this->FramesComboBox->SelectedIndexChanged += gcnew System::EventHandler(this, &window_weather_editor::FramesComboBox_SelectedIndexChanged);
			this->FramesComboBox->DropDownClosed += gcnew System::EventHandler(this, &window_weather_editor::FramesComboBox_DropDownClosed);
			this->FramesComboBox->DropDown += gcnew System::EventHandler(this, &window_weather_editor::FramesComboBox_DropDown);
			// 
			// panel2
			// 
			this->panel2->Controls->Add(this->panel13);
			this->panel2->Controls->Add(this->splitter2);
			this->panel2->Controls->Add(this->panel14);
			this->panel2->Controls->Add(this->splitter1);
			this->panel2->Controls->Add(this->panel12);
			this->panel2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel2->Location = System::Drawing::Point(0, 128);
			this->panel2->Name = L"panel2";
			this->panel2->Size = System::Drawing::Size(367, 248);
			this->panel2->TabIndex = 4;
			// 
			// panel13
			// 
			this->panel13->Controls->Add(this->blend);
			this->panel13->Controls->Add(this->panel15);
			this->panel13->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel13->Location = System::Drawing::Point(123, 0);
			this->panel13->Name = L"panel13";
			this->panel13->Size = System::Drawing::Size(131, 248);
			this->panel13->TabIndex = 24;
			// 
			// blend
			// 
			this->blend->Dock = System::Windows::Forms::DockStyle::Fill;
			this->blend->Location = System::Drawing::Point(0, 25);
			this->blend->Name = L"blend";
			this->blend->Size = System::Drawing::Size(131, 223);
			this->blend->TabIndex = 2;
			this->blend->ToolbarVisible = false;
			// 
			// panel15
			// 
			this->panel15->Controls->Add(this->label6);
			this->panel15->Controls->Add(this->panel22);
			this->panel15->Controls->Add(this->panel11);
			this->panel15->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel15->Location = System::Drawing::Point(0, 0);
			this->panel15->Name = L"panel15";
			this->panel15->Size = System::Drawing::Size(131, 25);
			this->panel15->TabIndex = 1;
			// 
			// label6
			// 
			this->label6->Dock = System::Windows::Forms::DockStyle::Fill;
			this->label6->Location = System::Drawing::Point(32, 0);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(66, 25);
			this->label6->TabIndex = 6;
			this->label6->Text = L"blend";
			this->label6->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// panel22
			// 
			this->panel22->Controls->Add(this->CopyButton);
			this->panel22->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel22->Location = System::Drawing::Point(0, 0);
			this->panel22->Name = L"panel22";
			this->panel22->Size = System::Drawing::Size(32, 25);
			this->panel22->TabIndex = 5;
			// 
			// CopyButton
			// 
			this->CopyButton->AutoSize = true;
			this->CopyButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"CopyButton.Image")));
			this->CopyButton->Location = System::Drawing::Point(0, 0);
			this->CopyButton->Name = L"CopyButton";
			this->CopyButton->Size = System::Drawing::Size(32, 23);
			this->CopyButton->TabIndex = 2;
			this->CopyButton->UseVisualStyleBackColor = true;
			this->CopyButton->Click += gcnew System::EventHandler(this, &window_weather_editor::CopyButton_Click);
			// 
			// panel11
			// 
			this->panel11->Controls->Add(this->CreateFromButton);
			this->panel11->Dock = System::Windows::Forms::DockStyle::Right;
			this->panel11->Location = System::Drawing::Point(98, 0);
			this->panel11->Name = L"panel11";
			this->panel11->Size = System::Drawing::Size(33, 25);
			this->panel11->TabIndex = 2;
			// 
			// CreateFromButton
			// 
			this->CreateFromButton->AutoSize = true;
			this->CreateFromButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"CreateFromButton.Image")));
			this->CreateFromButton->Location = System::Drawing::Point(0, 0);
			this->CreateFromButton->Name = L"CreateFromButton";
			this->CreateFromButton->Size = System::Drawing::Size(32, 23);
			this->CreateFromButton->TabIndex = 3;
			this->CreateFromButton->UseVisualStyleBackColor = true;
			this->CreateFromButton->Click += gcnew System::EventHandler(this, &window_weather_editor::CreateFromButton_Click);
			// 
			// splitter2
			// 
			this->splitter2->Dock = System::Windows::Forms::DockStyle::Right;
			this->splitter2->Location = System::Drawing::Point(254, 0);
			this->splitter2->Name = L"splitter2";
			this->splitter2->Size = System::Drawing::Size(3, 248);
			this->splitter2->TabIndex = 23;
			this->splitter2->TabStop = false;
			// 
			// panel14
			// 
			this->panel14->Controls->Add(this->target);
			this->panel14->Controls->Add(this->panel20);
			this->panel14->Controls->Add(this->panel17);
			this->panel14->Dock = System::Windows::Forms::DockStyle::Right;
			this->panel14->Location = System::Drawing::Point(257, 0);
			this->panel14->Name = L"panel14";
			this->panel14->Size = System::Drawing::Size(110, 248);
			this->panel14->TabIndex = 22;
			// 
			// target
			// 
			this->target->Dock = System::Windows::Forms::DockStyle::Fill;
			this->target->Location = System::Drawing::Point(0, 25);
			this->target->Name = L"target";
			this->target->Size = System::Drawing::Size(110, 223);
			this->target->TabIndex = 24;
			this->target->ToolbarVisible = false;
			// 
			// panel20
			// 
			this->panel20->Dock = System::Windows::Forms::DockStyle::Right;
			this->panel20->Location = System::Drawing::Point(110, 25);
			this->panel20->Name = L"panel20";
			this->panel20->Size = System::Drawing::Size(0, 223);
			this->panel20->TabIndex = 23;
			// 
			// panel17
			// 
			this->panel17->Controls->Add(this->label5);
			this->panel17->Controls->Add(this->panel18);
			this->panel17->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel17->Location = System::Drawing::Point(0, 0);
			this->panel17->Name = L"panel17";
			this->panel17->Size = System::Drawing::Size(110, 25);
			this->panel17->TabIndex = 22;
			// 
			// label5
			// 
			this->label5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->label5->Location = System::Drawing::Point(0, 0);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(46, 25);
			this->label5->TabIndex = 3;
			this->label5->Text = L"target";
			this->label5->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// panel18
			// 
			this->panel18->Controls->Add(this->ReloadTargetButton);
			this->panel18->Controls->Add(this->PasteTargetButton);
			this->panel18->Dock = System::Windows::Forms::DockStyle::Right;
			this->panel18->Location = System::Drawing::Point(46, 0);
			this->panel18->Name = L"panel18";
			this->panel18->Size = System::Drawing::Size(64, 25);
			this->panel18->TabIndex = 2;
			// 
			// PasteTargetButton
			// 
			this->PasteTargetButton->AutoSize = true;
			this->PasteTargetButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"PasteTargetButton.Image")));
			this->PasteTargetButton->Location = System::Drawing::Point(0, 0);
			this->PasteTargetButton->Name = L"PasteTargetButton";
			this->PasteTargetButton->Size = System::Drawing::Size(32, 23);
			this->PasteTargetButton->TabIndex = 2;
			this->PasteTargetButton->UseVisualStyleBackColor = true;
			this->PasteTargetButton->Click += gcnew System::EventHandler(this, &window_weather_editor::PasteTargetButton_Click);
			// 
			// splitter1
			// 
			this->splitter1->Location = System::Drawing::Point(120, 0);
			this->splitter1->Name = L"splitter1";
			this->splitter1->Size = System::Drawing::Size(3, 248);
			this->splitter1->TabIndex = 19;
			this->splitter1->TabStop = false;
			// 
			// panel12
			// 
			this->panel12->Controls->Add(this->current);
			this->panel12->Controls->Add(this->panel19);
			this->panel12->Controls->Add(this->panel16);
			this->panel12->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel12->Location = System::Drawing::Point(0, 0);
			this->panel12->Name = L"panel12";
			this->panel12->Size = System::Drawing::Size(120, 248);
			this->panel12->TabIndex = 18;
			// 
			// current
			// 
			this->current->Dock = System::Windows::Forms::DockStyle::Fill;
			this->current->Location = System::Drawing::Point(0, 25);
			this->current->Name = L"current";
			this->current->Size = System::Drawing::Size(120, 223);
			this->current->TabIndex = 19;
			this->current->ToolbarVisible = false;
			// 
			// panel19
			// 
			this->panel19->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel19->Location = System::Drawing::Point(0, 25);
			this->panel19->Name = L"panel19";
			this->panel19->Size = System::Drawing::Size(0, 223);
			this->panel19->TabIndex = 18;
			// 
			// panel16
			// 
			this->panel16->Controls->Add(this->label4);
			this->panel16->Controls->Add(this->panel21);
			this->panel16->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel16->Location = System::Drawing::Point(0, 0);
			this->panel16->Name = L"panel16";
			this->panel16->Size = System::Drawing::Size(120, 25);
			this->panel16->TabIndex = 17;
			// 
			// label4
			// 
			this->label4->Dock = System::Windows::Forms::DockStyle::Fill;
			this->label4->Location = System::Drawing::Point(67, 0);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(53, 25);
			this->label4->TabIndex = 4;
			this->label4->Text = L"current";
			this->label4->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// panel21
			// 
			this->panel21->AutoSize = true;
			this->panel21->Controls->Add(this->ReloadCurrentButton);
			this->panel21->Controls->Add(this->PasteCurrentButton);
			this->panel21->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel21->Location = System::Drawing::Point(0, 0);
			this->panel21->Name = L"panel21";
			this->panel21->Size = System::Drawing::Size(67, 25);
			this->panel21->TabIndex = 3;
			// 
			// PasteCurrentButton
			// 
			this->PasteCurrentButton->AutoSize = true;
			this->PasteCurrentButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"PasteCurrentButton.Image")));
			this->PasteCurrentButton->Location = System::Drawing::Point(0, 0);
			this->PasteCurrentButton->Name = L"PasteCurrentButton";
			this->PasteCurrentButton->Size = System::Drawing::Size(32, 23);
			this->PasteCurrentButton->TabIndex = 2;
			this->PasteCurrentButton->UseVisualStyleBackColor = true;
			this->PasteCurrentButton->Click += gcnew System::EventHandler(this, &window_weather_editor::PasteCurrentButton_Click);
			// 
			// ReloadCurrentButton
			// 
			this->ReloadCurrentButton->AutoSize = true;
			this->ReloadCurrentButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"ReloadCurrentButton.Image")));
			this->ReloadCurrentButton->Location = System::Drawing::Point(32, 0);
			this->ReloadCurrentButton->Name = L"ReloadCurrentButton";
			this->ReloadCurrentButton->Size = System::Drawing::Size(32, 23);
			this->ReloadCurrentButton->TabIndex = 3;
			this->ReloadCurrentButton->UseVisualStyleBackColor = true;
			this->ReloadCurrentButton->Click += gcnew System::EventHandler(this, &window_weather_editor::ReloadCurrentButton_Click);
			// 
			// ReloadTargetButton
			// 
			this->ReloadTargetButton->AutoSize = true;
			this->ReloadTargetButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"ReloadTargetButton.Image")));
			this->ReloadTargetButton->Location = System::Drawing::Point(32, 0);
			this->ReloadTargetButton->Name = L"ReloadTargetButton";
			this->ReloadTargetButton->Size = System::Drawing::Size(32, 23);
			this->ReloadTargetButton->TabIndex = 4;
			this->ReloadTargetButton->UseVisualStyleBackColor = true;
			this->ReloadTargetButton->Click += gcnew System::EventHandler(this, &window_weather_editor::ReloadTargetButton_Click);
			// 
			// window_weather_editor
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(367, 376);
			this->Controls->Add(this->panel2);
			this->Controls->Add(this->panel1);
			this->Controls->Add(this->panel23);
			this->Name = L"window_weather_editor";
			this->TabText = L"weather editor";
			this->Text = L"weather editor";
			this->SizeChanged += gcnew System::EventHandler(this, &window_weather_editor::window_weather_editor_SizeChanged);
			this->Enter += gcnew System::EventHandler(this, &window_weather_editor::window_weather_editor_Enter);
			this->panel23->ResumeLayout(false);
			this->panel23->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->WeatherTrackBar))->EndInit();
			this->panel25->ResumeLayout(false);
			this->panel25->PerformLayout();
			this->panel24->ResumeLayout(false);
			this->panel24->PerformLayout();
			this->panel1->ResumeLayout(false);
			this->panel1->PerformLayout();
			this->panel4->ResumeLayout(false);
			this->panel4->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->CurrentTimeTrackBar))->EndInit();
			this->panel10->ResumeLayout(false);
			this->panel9->ResumeLayout(false);
			this->panel3->ResumeLayout(false);
			this->panel7->ResumeLayout(false);
			this->panel7->PerformLayout();
			this->panel6->ResumeLayout(false);
			this->panel6->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->TimeFactorNumericUpDown))->EndInit();
			this->panel5->ResumeLayout(false);
			this->panel5->PerformLayout();
			this->panel2->ResumeLayout(false);
			this->panel13->ResumeLayout(false);
			this->panel15->ResumeLayout(false);
			this->panel22->ResumeLayout(false);
			this->panel22->PerformLayout();
			this->panel11->ResumeLayout(false);
			this->panel11->PerformLayout();
			this->panel14->ResumeLayout(false);
			this->panel17->ResumeLayout(false);
			this->panel18->ResumeLayout(false);
			this->panel18->PerformLayout();
			this->panel12->ResumeLayout(false);
			this->panel16->ResumeLayout(false);
			this->panel16->PerformLayout();
			this->panel21->ResumeLayout(false);
			this->panel21->PerformLayout();
			this->ResumeLayout(false);

		}
#pragma endregion
public:
	typedef ::editor::ide::weathers_getter_type			weathers_getter_type;
	typedef ::editor::ide::weathers_size_getter_type	weathers_size_getter_type;
	typedef ::editor::ide::frames_getter_type			frames_getter_type;
	typedef ::editor::ide::frames_size_getter_type		frames_size_getter_type;

public:
			void		weathers_ids				(
							weathers_getter_type const& weathers_getter,
							weathers_size_getter_type const& weathers_size_getter,
							frames_getter_type const& frames_getter,
							frames_size_getter_type const& frames_size_getter
						);
			void		on_load_finished			();
			void		on_idle						();
			void		save						(Microsoft::Win32::RegistryKey^ root);
			void		load						(Microsoft::Win32::RegistryKey^ root);
			void		fill_weathers				();

private:
			void		fill_frames					(LPCSTR current_weather_id);
			void		update_frame				();
			void		save						();
			void		load						();

private:
	window_ide^					m_ide;
	engine&						m_engine;
	weathers_getter_type*		m_weathers_getter;
	weathers_size_getter_type*	m_weathers_size_getter;
	frames_getter_type*			m_frames_getter;
	frames_size_getter_type*	m_frames_size_getter;
	bool						m_update_enabled;
	bool						m_load_finished;
	bool						m_mouse_down;
	bool						m_update_frames_combo_box;
	bool						m_update_frame_trackbar;
	bool						m_update_text_value;
	bool						m_update_weather_time;

private:
			Void		window_weather_editor_Enter				(Object^ sender, EventArgs^ e);
			Void		WeathersComboBox_SelectedIndexChanged	(Object^ sender, EventArgs^ e);
			Void		FramesComboBox_SelectedIndexChanged		(Object^ sender, EventArgs^ e);
			Void		PreviousFrameButton_Click				(Object^ sender, EventArgs^ e);
			Void		NextFrameButton_Click					(Object^ sender, EventArgs^ e);
			Void		CurrentTimeTrackBar_ValueChanged		(Object^ sender, EventArgs^ e);
			Void		FramesComboBox_DropDown					(Object^ sender, EventArgs^ e);
			Void		FramesComboBox_DropDownClosed			(Object^ sender, EventArgs^ e);
			Void		PauseButton_Click						(Object^ sender, EventArgs^ e);
			Void		TimeFactorNumericUpDown_ValueChanged	(Object^ sender, EventArgs^ e);
			
private:
			Void		CopyButton_Click						(Object^ sender, EventArgs^ e);
			Void		PasteCurrentButton_Click				(Object^ sender, EventArgs^ e);
			Void		PasteTargetButton_Click					(Object^ sender, EventArgs^ e);
			Void		CreateFromButton_Click					(Object^ sender, EventArgs^ e);
			Void		window_weather_editor_SizeChanged		(Object^ sender, EventArgs^ e);
			Void		CurrentTimeTrackBar_MouseDown			(Object^ sender, MouseEventArgs^ e);
			Void		CurrentTimeTrackBar_MouseUp				(Object^ sender, MouseEventArgs^ e);
			Void		current_Leave							(Object^ sender, EventArgs^ e);
			Void		target_Leave							(Object^ sender, EventArgs^ e);
			Void		current_Enter							(Object^ sender, EventArgs^ e);
			Void		target_Enter							(Object^ sender, EventArgs^ e);
			Void		CurrentTimeTextBox_TextChanged			(Object^ sender, EventArgs^ e);
			Void		WeatherTrackBar_ValueChanged			(Object^ sender, EventArgs^ e);
			Void		WeatherTrackBar_MouseDown				(Object^ sender, MouseEventArgs^ e);
			Void		WeatherTrackBar_MouseUp					(Object^ sender, MouseEventArgs^ e);
			Void		ReloadCurrentButton_Click				(Object^ sender, EventArgs^ e);
			Void		ReloadTargetButton_Click				(Object^ sender, EventArgs^ e);
};

}