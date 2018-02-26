#pragma once

namespace XRay
{
namespace ECore
{
namespace Props
{
ref class GameType;
}
}
}

struct GameTypeChooser;

namespace XRay
{
namespace ECore
{
namespace Props
{
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

public ref class GameType : public System::Windows::Forms::Form
{
public:
	GameType(void)
	{
		InitializeComponent();
	}

protected:
	~GameType()
	{
		if (components)
		{
			delete components;
		}
	}

public:
    bool Run(pcstr title, GameTypeChooser* data);

private:
    GameTypeChooser* gameTypes;

private: System::Void buttonOk_Click(System::Object^ sender, System::EventArgs^ e);

private: System::Windows::Forms::CheckBox^ checkSingle;
private: System::Windows::Forms::CheckBox^ checkDM;
private: System::Windows::Forms::CheckBox^ checkTDM;
private: System::Windows::Forms::CheckBox^ checkAfHunt;
private: System::Windows::Forms::CheckBox^ checkCTA;

private: System::Windows::Forms::Button^ buttonOk;
private: System::Windows::Forms::Button^ buttonCancel;

private: System::Windows::Forms::Panel^ panel1;
private:
	System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
	void InitializeComponent(void)
	{
        this->checkSingle = (gcnew System::Windows::Forms::CheckBox());
        this->checkDM = (gcnew System::Windows::Forms::CheckBox());
        this->checkTDM = (gcnew System::Windows::Forms::CheckBox());
        this->checkAfHunt = (gcnew System::Windows::Forms::CheckBox());
        this->checkCTA = (gcnew System::Windows::Forms::CheckBox());
        this->buttonOk = (gcnew System::Windows::Forms::Button());
        this->buttonCancel = (gcnew System::Windows::Forms::Button());
        this->panel1 = (gcnew System::Windows::Forms::Panel());
        this->panel1->SuspendLayout();
        this->SuspendLayout();
        this->checkSingle->AutoSize = true;
        this->checkSingle->Location = System::Drawing::Point(3, 3);
        this->checkSingle->Name = L"checkSingle";
        this->checkSingle->Size = System::Drawing::Size(55, 17);
        this->checkSingle->TabIndex = 0;
        this->checkSingle->Text = L"Single";
        this->checkSingle->UseVisualStyleBackColor = true;
        this->checkDM->AutoSize = true;
        this->checkDM->Location = System::Drawing::Point(3, 26);
        this->checkDM->Name = L"checkDM";
        this->checkDM->Size = System::Drawing::Size(78, 17);
        this->checkDM->TabIndex = 1;
        this->checkDM->Text = L"Deathmatch";
        this->checkDM->UseVisualStyleBackColor = true;
        this->checkTDM->AutoSize = true;
        this->checkTDM->Location = System::Drawing::Point(3, 49);
        this->checkTDM->Name = L"checkTDM";
        this->checkTDM->Size = System::Drawing::Size(108, 17);
        this->checkTDM->TabIndex = 2;
        this->checkTDM->Text = L"Team Deathmatch";
        this->checkTDM->UseVisualStyleBackColor = true;
        this->checkAfHunt->AutoSize = true;
        this->checkAfHunt->Location = System::Drawing::Point(3, 72);
        this->checkAfHunt->Name = L"checkAfHunt";
        this->checkAfHunt->Size = System::Drawing::Size(89, 17);
        this->checkAfHunt->TabIndex = 3;
        this->checkAfHunt->Text = L"Artefact Hunt";
        this->checkAfHunt->UseVisualStyleBackColor = true;
        this->checkCTA->AutoSize = true;
        this->checkCTA->Location = System::Drawing::Point(3, 95);
        this->checkCTA->Name = L"checkCTA";
        this->checkCTA->Size = System::Drawing::Size(121, 17);
        this->checkCTA->TabIndex = 4;
        this->checkCTA->Text = L"Capture the Artefact";
        this->checkCTA->UseVisualStyleBackColor = true;
        this->buttonOk->DialogResult = System::Windows::Forms::DialogResult::OK;
        this->buttonOk->Location = System::Drawing::Point(0, 115);
        this->buttonOk->Name = L"buttonOk";
        this->buttonOk->Size = System::Drawing::Size(64, 23);
        this->buttonOk->TabIndex = 5;
        this->buttonOk->Text = L"Ok";
        this->buttonOk->UseVisualStyleBackColor = true;
        this->buttonOk->Click += gcnew System::EventHandler(this, &GameType::buttonOk_Click);
        this->buttonCancel->DialogResult = System::Windows::Forms::DialogResult::Cancel;
        this->buttonCancel->Location = System::Drawing::Point(63, 115);
        this->buttonCancel->Name = L"buttonCancel";
        this->buttonCancel->Size = System::Drawing::Size(64, 23);
        this->buttonCancel->TabIndex = 6;
        this->buttonCancel->Text = L"Cancel";
        this->buttonCancel->UseVisualStyleBackColor = true;
        this->panel1->Controls->Add(this->checkSingle);
        this->panel1->Controls->Add(this->checkDM);
        this->panel1->Controls->Add(this->checkTDM);
        this->panel1->Controls->Add(this->checkCTA);
        this->panel1->Controls->Add(this->checkAfHunt);
        this->panel1->Location = System::Drawing::Point(0, 0);
        this->panel1->Name = L"panel1";
        this->panel1->Size = System::Drawing::Size(127, 115);
        this->panel1->TabIndex = 7;
        this->AcceptButton = this->buttonOk;
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->CancelButton = this->buttonCancel;
        this->ClientSize = System::Drawing::Size(127, 138);
        this->Controls->Add(this->panel1);
        this->Controls->Add(this->buttonOk);
        this->Controls->Add(this->buttonCancel);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedToolWindow;
        this->Name = L"GameType";
        this->Text = L"GameType";
        this->panel1->ResumeLayout(false);
        this->panel1->PerformLayout();
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Props
} // namespace ECore
} // namespace XRay
