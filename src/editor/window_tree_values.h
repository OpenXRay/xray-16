#pragma once

#include "property_string_values_value_base.hpp"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

namespace editor {

	/// <summary>
	/// Summary for window_tree_values
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class window_tree_values : public System::Windows::Forms::Form
	{
	public:
		window_tree_values(void)
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
		~window_tree_values()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Panel^  panel2;
	protected: 
	private: System::Windows::Forms::Panel^  panel3;
	private: System::Windows::Forms::Panel^  panel4;
	private: System::Windows::Forms::Panel^  panel1;
	private: System::Windows::Forms::Panel^  panel5;
	private: System::Windows::Forms::Panel^  panel6;





	private: System::Windows::Forms::TreeView^  TreeView;


	private: System::Windows::Forms::Panel^  panel7;
	private: System::Windows::Forms::ImageList^  Images;
	private: System::Windows::Forms::Panel^  panel10;
	private: System::Windows::Forms::TextBox^  TextBox;

	private: System::Windows::Forms::Panel^  panel8;
	private: System::Windows::Forms::Button^  OK_Button;
	private: System::Windows::Forms::Button^  Cancel_Button;

	private: System::ComponentModel::IContainer^  components;

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
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(window_tree_values::typeid));
			this->panel2 = (gcnew System::Windows::Forms::Panel());
			this->panel3 = (gcnew System::Windows::Forms::Panel());
			this->panel4 = (gcnew System::Windows::Forms::Panel());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->panel5 = (gcnew System::Windows::Forms::Panel());
			this->TreeView = (gcnew System::Windows::Forms::TreeView());
			this->Images = (gcnew System::Windows::Forms::ImageList(this->components));
			this->panel7 = (gcnew System::Windows::Forms::Panel());
			this->panel6 = (gcnew System::Windows::Forms::Panel());
			this->panel10 = (gcnew System::Windows::Forms::Panel());
			this->TextBox = (gcnew System::Windows::Forms::TextBox());
			this->panel8 = (gcnew System::Windows::Forms::Panel());
			this->OK_Button = (gcnew System::Windows::Forms::Button());
			this->Cancel_Button = (gcnew System::Windows::Forms::Button());
			this->panel5->SuspendLayout();
			this->panel6->SuspendLayout();
			this->panel10->SuspendLayout();
			this->panel8->SuspendLayout();
			this->SuspendLayout();
			// 
			// panel2
			// 
			this->panel2->Dock = System::Windows::Forms::DockStyle::Left;
			this->panel2->Location = System::Drawing::Point(0, 0);
			this->panel2->Name = L"panel2";
			this->panel2->Size = System::Drawing::Size(10, 383);
			this->panel2->TabIndex = 1;
			// 
			// panel3
			// 
			this->panel3->Dock = System::Windows::Forms::DockStyle::Right;
			this->panel3->Location = System::Drawing::Point(489, 0);
			this->panel3->Name = L"panel3";
			this->panel3->Size = System::Drawing::Size(10, 383);
			this->panel3->TabIndex = 2;
			// 
			// panel4
			// 
			this->panel4->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panel4->Location = System::Drawing::Point(10, 373);
			this->panel4->Name = L"panel4";
			this->panel4->Size = System::Drawing::Size(479, 10);
			this->panel4->TabIndex = 3;
			// 
			// panel1
			// 
			this->panel1->Dock = System::Windows::Forms::DockStyle::Top;
			this->panel1->Location = System::Drawing::Point(10, 0);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(479, 10);
			this->panel1->TabIndex = 4;
			// 
			// panel5
			// 
			this->panel5->Controls->Add(this->TreeView);
			this->panel5->Controls->Add(this->panel7);
			this->panel5->Controls->Add(this->panel6);
			this->panel5->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel5->Location = System::Drawing::Point(10, 10);
			this->panel5->Name = L"panel5";
			this->panel5->Size = System::Drawing::Size(479, 363);
			this->panel5->TabIndex = 5;
			// 
			// TreeView
			// 
			this->TreeView->Dock = System::Windows::Forms::DockStyle::Fill;
			this->TreeView->HideSelection = false;
			this->TreeView->ImageIndex = 0;
			this->TreeView->ImageList = this->Images;
			this->TreeView->Location = System::Drawing::Point(0, 0);
			this->TreeView->Name = L"TreeView";
			this->TreeView->SelectedImageIndex = 0;
			this->TreeView->Size = System::Drawing::Size(479, 327);
			this->TreeView->TabIndex = 0;
			this->TreeView->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &window_tree_values::TreeView_MouseDoubleClick);
			this->TreeView->AfterCollapse += gcnew System::Windows::Forms::TreeViewEventHandler(this, &window_tree_values::TreeView_AfterCollapse);
			this->TreeView->MouseClick += gcnew System::Windows::Forms::MouseEventHandler(this, &window_tree_values::TreeView_MouseClick);
			this->TreeView->AfterExpand += gcnew System::Windows::Forms::TreeViewEventHandler(this, &window_tree_values::TreeView_AfterExpand);
			// 
			// Images
			// 
			this->Images->ImageStream = (cli::safe_cast<System::Windows::Forms::ImageListStreamer^  >(resources->GetObject(L"Images.ImageStream")));
			this->Images->TransparentColor = System::Drawing::Color::Fuchsia;
			this->Images->Images->SetKeyName(0, L"filter_closed.bmp");
			this->Images->Images->SetKeyName(1, L"filter_opened.bmp");
			this->Images->Images->SetKeyName(2, L"Document.bmp");
			// 
			// panel7
			// 
			this->panel7->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panel7->Location = System::Drawing::Point(0, 327);
			this->panel7->Name = L"panel7";
			this->panel7->Size = System::Drawing::Size(479, 10);
			this->panel7->TabIndex = 1;
			// 
			// panel6
			// 
			this->panel6->Controls->Add(this->panel10);
			this->panel6->Controls->Add(this->panel8);
			this->panel6->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panel6->Location = System::Drawing::Point(0, 337);
			this->panel6->Name = L"panel6";
			this->panel6->Size = System::Drawing::Size(479, 26);
			this->panel6->TabIndex = 0;
			// 
			// panel10
			// 
			this->panel10->Controls->Add(this->TextBox);
			this->panel10->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel10->Location = System::Drawing::Point(0, 0);
			this->panel10->Name = L"panel10";
			this->panel10->Size = System::Drawing::Size(313, 26);
			this->panel10->TabIndex = 6;
			// 
			// TextBox
			// 
			this->TextBox->Dock = System::Windows::Forms::DockStyle::Fill;
			this->TextBox->Location = System::Drawing::Point(0, 0);
			this->TextBox->Name = L"TextBox";
			this->TextBox->ReadOnly = true;
			this->TextBox->Size = System::Drawing::Size(313, 21);
			this->TextBox->TabIndex = 6;
			// 
			// panel8
			// 
			this->panel8->Controls->Add(this->OK_Button);
			this->panel8->Controls->Add(this->Cancel_Button);
			this->panel8->Dock = System::Windows::Forms::DockStyle::Right;
			this->panel8->Location = System::Drawing::Point(313, 0);
			this->panel8->Name = L"panel8";
			this->panel8->Size = System::Drawing::Size(166, 26);
			this->panel8->TabIndex = 4;
			// 
			// OK_Button
			// 
			this->OK_Button->DialogResult = System::Windows::Forms::DialogResult::OK;
			this->OK_Button->Location = System::Drawing::Point(8, 0);
			this->OK_Button->Name = L"OK_Button";
			this->OK_Button->Size = System::Drawing::Size(75, 23);
			this->OK_Button->TabIndex = 3;
			this->OK_Button->Text = L"&OK";
			this->OK_Button->UseVisualStyleBackColor = true;
			// 
			// Cancel_Button
			// 
			this->Cancel_Button->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->Cancel_Button->Location = System::Drawing::Point(91, 0);
			this->Cancel_Button->Name = L"Cancel_Button";
			this->Cancel_Button->Size = System::Drawing::Size(75, 23);
			this->Cancel_Button->TabIndex = 4;
			this->Cancel_Button->Text = L"&Cancel";
			this->Cancel_Button->UseVisualStyleBackColor = true;
			// 
			// window_tree_values
			// 
			this->AcceptButton = this->OK_Button;
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->CancelButton = this->Cancel_Button;
			this->ClientSize = System::Drawing::Size(499, 383);
			this->Controls->Add(this->panel5);
			this->Controls->Add(this->panel1);
			this->Controls->Add(this->panel4);
			this->Controls->Add(this->panel3);
			this->Controls->Add(this->panel2);
			this->Font = (gcnew System::Drawing::Font(L"Tahoma", 8.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(204)));
			this->MaximizeBox = false;
			this->MinimizeBox = false;
			this->Name = L"window_tree_values";
			this->ShowIcon = false;
			this->ShowInTaskbar = false;
			this->StartPosition = System::Windows::Forms::FormStartPosition::CenterParent;
			this->Text = L"Select from the list...";
			this->panel5->ResumeLayout(false);
			this->panel6->ResumeLayout(false);
			this->panel10->ResumeLayout(false);
			this->panel10->PerformLayout();
			this->panel8->ResumeLayout(false);
			this->ResumeLayout(false);

		}
#pragma endregion
public:
	typedef property_string_values_value_base::collection_type	collection_type;

public:
			void		values						(
							collection_type^ values,
							String^ current_value
						);

public:
	System::String^		Result;

private:
			Void		TreeView_AfterCollapse		(Object^ sender, TreeViewEventArgs^ e);
			Void		TreeView_AfterExpand		(Object^ sender, TreeViewEventArgs^ e);
			Void		TreeView_MouseClick			(Object^ sender, MouseEventArgs^ e);
			Void		TreeView_MouseDoubleClick	(Object^ sender, MouseEventArgs^ e);
};
}
