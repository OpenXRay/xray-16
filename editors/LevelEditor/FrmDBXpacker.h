//---------------------------------------------------------------------------

#ifndef FrmDBXpackerH
#define FrmDBXpackerH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "ElShellCtl.hpp"
#include "ElTree.hpp"
#include "ElXPThemedControl.hpp"
#include <ExtCtrls.hpp>
#include "ElListBox.hpp"
#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
//---------------------------------------------------------------------------
class TDB_packer : public TForm
{
__published:	// IDE-managed Components
        TPanel *Panel1;
        TPanel *Panel2;
        TExtBtn *btnLoad;
        TExtBtn *btnSave;
	TExtBtn *ExtBtn5;
	TPanel *Panel5;
	TElShellTree *shellTree;
	TPanel *Panel6;
	TPanel *Panel3;
	TMxLabel *MxLabel2;
	TExtBtn *ExtBtn3;
	TExtBtn *ExtBtn4;
	TElListBox *lbIncludeFiles;
	TPanel *Panel4;
	TMxLabel *MxLabel1;
	TExtBtn *ExtBtn1;
	TExtBtn *ExtBtn2;
	TElListBox *lbIncludeFolders;
	TSplitter *Splitter1;
	void __fastcall btnLoadClick(TObject *Sender);
	void __fastcall ExtBtn2Click(TObject *Sender);
	void __fastcall ExtBtn4Click(TObject *Sender);
	void __fastcall ExtBtn1Click(TObject *Sender);
	void __fastcall ExtBtn3Click(TObject *Sender);
	void __fastcall btnSaveClick(TObject *Sender);
	void __fastcall ExtBtn5Click(TObject *Sender);
private:	// User declarations
        static TDB_packer*	m_form;
        xr_string 			m_cfgFileName;
        string_path			m_root_folder;
        void				_load_(const xr_string& fn);
public:		// User declarations
        __fastcall TDB_packer(TComponent* Owner);
        void __fastcall prepare			();
        static void __fastcall ActivatePacker ();
};
//---------------------------------------------------------------------------
extern PACKAGE TDB_packer *DB_packer;
//---------------------------------------------------------------------------
#endif
