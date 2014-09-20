//---------------------------------------------------------------------------

#ifndef AppendObjectInfoFormH
#define AppendObjectInfoFormH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class CCustomObject;

class TfrmAppendObjectInfo : public TForm
{
__published:	// IDE-managed Components
	TStaticText *StaticText1;
	TButton *btOverwrite;
	TButton *btSkip;
	TCheckBox *CheckBox1;
	TButton *Button1;
	TStaticText *StaticText2;
	void __fastcall btOverwriteClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TfrmAppendObjectInfo(TComponent* Owner);

    void					Prepare				();
    CCustomObject*			m_existing_object;
    CCustomObject*			m_new_object;
	int 					m_result;
};

extern    TfrmAppendObjectInfo* 					g_frmConflictLoadObject;
//---------------------------------------------------------------------------
#endif
