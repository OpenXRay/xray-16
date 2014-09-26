//---------------------------------------------------------------------------

#ifndef SearchH
#define SearchH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TSearchForm : public TForm
{
__published:	// IDE-managed Components
        TLabel *Label1;
        TComboBox *FieldCombo;
        TComboBox *ActionCombo;
        TEdit *DataEdit;
        TButton *SearchBtn;
        TButton *CloseBtn;
        void __fastcall CloseBtnClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TSearchForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TSearchForm *SearchForm;
//---------------------------------------------------------------------------
#endif
