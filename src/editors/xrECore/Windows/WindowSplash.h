#pragma once

namespace XRay
{
namespace Editor
{
namespace Windows
{
public ref class WindowSplash : public System::Windows::Forms::Form
{
public:
    WindowSplash(void)
    {
        InitializeComponent();
    }

    void SetStatus(System::String^ status)
    {
        label1->Text = status;
        Refresh();
    }

    void Show()
    {
        System::Windows::Forms::Form::Show();
        Refresh();
    }

protected:
    ~WindowSplash()
    {
        if (components)
        {
            delete components;
        }
    }

private: System::Windows::Forms::PictureBox^ pictureBox1;
private: System::Windows::Forms::Label^ label1;

private:
    System::ComponentModel::Container^ components;

#pragma region Windows Form Designer generated code
    void InitializeComponent(void)
    {
        System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(WindowSplash::typeid));
        this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
        this->label1 = (gcnew System::Windows::Forms::Label());
        (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
        this->SuspendLayout();
        // 
        // pictureBox1
        // 
        this->pictureBox1->Dock = System::Windows::Forms::DockStyle::Fill;
        this->pictureBox1->Image = (cli::safe_cast<System::Drawing::Image^>(resources->GetObject(L"pictureBox1.Image")));
        this->pictureBox1->Location = System::Drawing::Point(0, 0);
        this->pictureBox1->Name = L"pictureBox1";
        this->pictureBox1->Size = System::Drawing::Size(500, 285);
        this->pictureBox1->TabIndex = 0;
        this->pictureBox1->TabStop = false;
        // 
        // label1
        // 
        this->label1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left)
            | System::Windows::Forms::AnchorStyles::Right));
        this->label1->BackColor = System::Drawing::Color::Black;
        this->label1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
            static_cast<System::Byte>(204)));
        this->label1->ForeColor = System::Drawing::Color::White;
        this->label1->Location = System::Drawing::Point(0, 267);
        this->label1->Name = L"label1";
        this->label1->Size = System::Drawing::Size(500, 18);
        this->label1->TabIndex = 1;
        this->label1->Text = L"Loading...";
        this->label1->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
        // 
        // WindowSplash
        // 
        this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
        this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        this->ClientSize = System::Drawing::Size(500, 285);
        this->Controls->Add(this->label1);
        this->Controls->Add(this->pictureBox1);
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::None;
        this->Name = L"WindowSplash";
        this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
        this->Text = L"Loading...";
        (cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
        this->ResumeLayout(false);

    }
#pragma endregion
};
} // namespace Windows
} // namespace Editor
} // namespace XRay
