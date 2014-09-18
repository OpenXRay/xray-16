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
	/// Summary for window_weather
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class window_weather : public WeifenLuo::WinFormsUI::DockContent
	{
	public:
		window_weather(window_ide^ ide) :
			m_ide	(ide)
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
		~window_weather()
		{
			if (components)
			{
				delete components;
			}
		}

	private: System::Windows::Forms::ToolStrip^  toolStrip1;
	private: System::Windows::Forms::ToolStripButton^  SaveButton;
	private: editor::controls::property_grid^  PropertyGrid;
	private: System::Windows::Forms::ToolStripButton^  ReloadWeatherButton;
	private: System::Windows::Forms::ToolStripButton^  ReloadAllWeathersButton;
	protected: 

	protected: 

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
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(window_weather::typeid));
			this->toolStrip1 = (gcnew System::Windows::Forms::ToolStrip());
			this->SaveButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->ReloadWeatherButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->ReloadAllWeathersButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->PropertyGrid = (gcnew editor::controls::property_grid());
			this->toolStrip1->SuspendLayout();
			this->SuspendLayout();
			// 
			// toolStrip1
			// 
			this->toolStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {this->SaveButton, this->ReloadWeatherButton, 
				this->ReloadAllWeathersButton});
			this->toolStrip1->Location = System::Drawing::Point(0, 0);
			this->toolStrip1->Name = L"toolStrip1";
			this->toolStrip1->Size = System::Drawing::Size(292, 25);
			this->toolStrip1->TabIndex = 1;
			this->toolStrip1->Text = L"toolStrip1";
			// 
			// SaveButton
			// 
			this->SaveButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Image;
			this->SaveButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"SaveButton.Image")));
			this->SaveButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->SaveButton->Name = L"SaveButton";
			this->SaveButton->Size = System::Drawing::Size(23, 22);
			this->SaveButton->Text = L"save weathers";
			this->SaveButton->Click += gcnew System::EventHandler(this, &window_weather::SaveButton_Click);
			// 
			// ReloadWeatherButton
			// 
			this->ReloadWeatherButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Image;
			this->ReloadWeatherButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"ReloadWeatherButton.Image")));
			this->ReloadWeatherButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->ReloadWeatherButton->Name = L"ReloadWeatherButton";
			this->ReloadWeatherButton->Size = System::Drawing::Size(23, 22);
			this->ReloadWeatherButton->Text = L"Reload current weather only";
			this->ReloadWeatherButton->Click += gcnew System::EventHandler(this, &window_weather::ReloadWeatherButton_Click);
			// 
			// ReloadAllWeathersButton
			// 
			this->ReloadAllWeathersButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Image;
			this->ReloadAllWeathersButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"ReloadAllWeathersButton.Image")));
			this->ReloadAllWeathersButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->ReloadAllWeathersButton->Name = L"ReloadAllWeathersButton";
			this->ReloadAllWeathersButton->Size = System::Drawing::Size(23, 22);
			this->ReloadAllWeathersButton->Text = L"Reload all the weathers";
			this->ReloadAllWeathersButton->Click += gcnew System::EventHandler(this, &window_weather::ReloadAllWeathersButton_Click);
			// 
			// PropertyGrid
			// 
			this->PropertyGrid->Dock = System::Windows::Forms::DockStyle::Fill;
			this->PropertyGrid->Location = System::Drawing::Point(0, 25);
			this->PropertyGrid->Name = L"PropertyGrid";
			this->PropertyGrid->Size = System::Drawing::Size(292, 248);
			this->PropertyGrid->TabIndex = 2;
			this->PropertyGrid->ToolbarVisible = false;
			// 
			// window_weather
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(292, 273);
			this->Controls->Add(this->PropertyGrid);
			this->Controls->Add(this->toolStrip1);
			this->Name = L"window_weather";
			this->TabText = L"weather";
			this->Text = L"weather";
			this->Leave += gcnew System::EventHandler(this, &window_weather::window_weather_Leave);
			this->toolStrip1->ResumeLayout(false);
			this->toolStrip1->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
private:
	window_ide^					m_ide;

public:
	inline	::PropertyGrid^		property_grid		() {return this->PropertyGrid;}

private:
			Void				window_weather_Leave			(Object^ sender, EventArgs^ e);
			Void				SaveButton_Click				(Object^ sender, EventArgs^ e);
			Void				ReloadWeatherButton_Click		(Object^ sender, EventArgs^ e);
			Void				ReloadAllWeathersButton_Click	(Object^ sender, EventArgs^ e);
}; // ref class window_weather

} // namespace editor